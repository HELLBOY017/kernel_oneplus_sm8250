#include "tmemory.h"

#include <linux/migrate.h>
#include <linux/bio.h>
#include <linux/blk_types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/random.h>

int g_submit_times_per_tran;

extern struct kmem_cache *transaction_entry_slab;
extern struct kmem_cache *discard_context_slab;
extern struct kmem_cache *read_context_slab;
extern struct kmem_cache *encrypt_context_slab;
extern struct kmem_cache *bypass_context_slab;

struct tmemory_transaction *latest_trans(struct tmemory_device *tm)
{
	return container_of(READ_ONCE(tm->transactions.prev),
			struct tmemory_transaction, list);
}

/* latest transaction in the tail of transaction list */
struct tmemory_transaction *latest_transaction(struct tmemory_device *tm)
{
	return list_last_entry(&tm->transactions,
			struct tmemory_transaction, list);
}

int do_discard_bio(struct tmemory_device *tm, struct bio *bio,
			pgoff_t blkaddr, unsigned int blkofs,
			bool *need_endio)
{
	struct radix_tree_root *root = &tm->global_space;
	unsigned int n = bio->bi_iter.bi_size;
	int err;

	if (blkofs) {
		TMEMORY_BUG_ON(1, tm, "blkofs: %u", blkofs);
		if (n <= PAGE_SIZE - blkofs)
			return -EIO;
		n -= (PAGE_SIZE - blkofs);
		blkaddr++;
	}

	down_read(&tm->commit_rwsem);
	if (time_to_inject(tm, FAULT_PANIC_COMMIT_RWSEM)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_COMMIT_RWSEM);
		TMEMORY_BUG_ON(1, tm, "");
	}
	while (n >= PAGE_SIZE) {
		struct page *dst;
		void *slot;
		unsigned long flags;

		err = radix_tree_preload(GFP_NOIO);
		if (err) {
			tmemory_error(TMEMORY_ERR_RADIX_PRELOAD);
			goto err;
		}

		spin_lock_irqsave(&tm->update_lock, flags);
		if (time_to_inject(tm, FAULT_PANIC_UPDATE_LOCK)) {
			tmemory_show_injection_info(tm, FAULT_PANIC_UPDATE_LOCK);
			TMEMORY_BUG_ON(1, tm, "");
		}
retry:
		slot = radix_tree_lookup_slot(root, blkaddr);
		if (!slot)
			goto insert;
		dst = radix_tree_deref_slot_protected(slot, &tm->update_lock);

		if (!dst) {
insert:
			err = radix_tree_insert(root, blkaddr,
						TMEMORY_DISCARD_MARKED);
			TMEMORY_BUG_ON(err, tm, "");
		} else if (radix_tree_exception(dst)) {
			if (radix_tree_deref_retry(dst))
				goto retry;
		} else if (dst != TMEMORY_DISCARD_MARKED) {
			radix_tree_replace_slot(root, slot, TMEMORY_DISCARD_MARKED);
			TMEMORY_BUG_ON(page_ref_count(dst) < 2, tm, "");
			// inject: keep discared page for a while
			if (time_to_inject(tm, FAULT_DELAY)) {
				tmemory_show_injection_info(tm, FAULT_DELAY);
				mdelay(get_random_u32() & 0x7f); // 128ms
			}
			put_page(dst);
			atomic_dec(&tm->page_cnt);
		}

		spin_unlock_irqrestore(&tm->update_lock, flags);
		blkaddr++;
		n -= PAGE_SIZE;
		radix_tree_preload_end();
	}
err:
	up_read(&tm->commit_rwsem);
	*need_endio = true;
	return err;
}

void migrate_lock(struct tmemory_device *tm)
{
#ifdef CONFIG_TMEMORY_MIGRATION
	down_read(&tm->migrate_lock);
	if (time_to_inject(tm, FAULT_PANIC_MIGRATE_LOCK)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_MIGRATE_LOCK);
		TMEMORY_BUG_ON(1, tm, "");
	}
#endif
}

void migrate_unlock(struct tmemory_device *tm)
{
#ifdef CONFIG_TMEMORY_MIGRATION
	up_read(&tm->migrate_lock);
#endif
}

void tmemory_submit_bio(struct bio *bio, int op, unsigned int op_flags, struct tmemory_device *tm)
{
	if (op == REQ_OP_DISCARD || op == REQ_OP_SECURE_ERASE) {
		atomic_inc(&tm->inflight_discard_bio);
	} else if (op == REQ_OP_WRITE) {
		if (tm->config & TMEMORY_CONFIG_FORCE_SYNC_WRITE)
			bio->bi_opf |= REQ_SYNC;
		atomic_inc(&tm->inflight_write_bio);
	} else if (op == REQ_OP_READ) {
		atomic_inc(&tm->inflight_read_bio);
	}

	bio_set_op_attrs(bio, op, bio->bi_opf);
	if (op == REQ_OP_WRITE && !atomic_read(&tm->w_count_fg) &&
#if 1
		(current->flags & PF_KTHREAD))
#else //OPLUS_FEATURE_SCHED_ASSIST
		(current->flags & PF_KTHREAD)) && (op_flags & REQ_FG))
#endif
		tmemory_wait(tm);
	tmemory_debug(tm, "tmemory_submit_bio, op:%d, opf:%d, idx:%llu, segs:%u",
		op, bio->bi_opf, bio->bi_iter.bi_sector >> SECTOR_PER_PAGE_SHIFT,
		bio_segments(bio));
	submit_bio(bio);
}

void inc_tmemory_remaining(struct tmemory_device *tm)
{
	unsigned long flags1;
	spin_lock_irqsave(&tm->state_lock, flags1);
	tm->tmemory_remaining++;
	spin_unlock_irqrestore(&tm->state_lock, flags1);
}

#ifdef CONFIG_TMEMORY_DEBUG
static void detect_memleak(struct tmemory_device *tm)
{
	bool bugon = false;

	if (atomic_read(&tm->trans_slab) != 1)
		bugon = true;
	if (atomic_read(&tm->discard_slab))
		bugon = true;
	if (atomic_read(&tm->read_slab))
		bugon = true;
	if (atomic_read(&tm->encrypt_slab))
		bugon = true;
	if (atomic_read(&tm->crypto_slab))
		bugon = true;
	if (atomic_read(&tm->bio_cnt))
		bugon = true;
	if (atomic_read(&tm->page_cnt))
		bugon = true;

	if (!bugon) {
		tmemory_info(tm, "all slab counters are valid");
		return;
	}

	tmemory_err(tm, "trans_slab: %d\ndiscard_slab: %d\nread_slab: %d\n"
		    "encrypt_slab: %d\ncrypto_slab: %d\nbypass_cnt_slab: %d\n"
		    "bio_cnt: %d\npage_cnt: %d\n",
		    atomic_read(&tm->trans_slab), atomic_read(&tm->discard_slab),
		    atomic_read(&tm->read_slab), atomic_read(&tm->encrypt_slab),
		    atomic_read(&tm->crypto_slab), atomic_read(&tm->bypass_ctx_slab),
		    atomic_read(&tm->bio_cnt), atomic_read(&tm->page_cnt));
}
#endif

void dec_tmemory_remaining(struct tmemory_device *tm)
{
	unsigned long flags1, flags2, flags3;
	bool wakeup = false;
	spin_lock_irqsave(&tm->state_lock, flags1);
	tm->tmemory_remaining--;
	if (tm->tmemory_remaining == 0 &&
	    tm->state == TM_STATE_STOPPING) {
		spin_lock_irqsave(&tm->discard_lock, flags2);
		spin_lock_irqsave(&tm->update_lock, flags3);

		if (time_to_inject(tm, FAULT_PANIC_STATE_LOCK)) {
			tmemory_show_injection_info(tm, FAULT_PANIC_STATE_LOCK);
			TMEMORY_BUG_ON(1, tm, "");
		}
		if (time_to_inject(tm, FAULT_PANIC_DISCARD_LOCK)) {
			tmemory_show_injection_info(tm, FAULT_PANIC_DISCARD_LOCK);
			TMEMORY_BUG_ON(1, tm, "");
		}
		if (time_to_inject(tm, FAULT_PANIC_UPDATE_LOCK)) {
			tmemory_show_injection_info(tm, FAULT_PANIC_UPDATE_LOCK);
			TMEMORY_BUG_ON(1, tm, "");
		}

		if (radix_tree_empty(&tm->global_space) &&
		    radix_tree_empty(&tm->discard_space)) {
			tm->switch_jiffies = jiffies + SWITCH_FROZEN_TIME;
			tm->state = TM_STATE_STOPPED;
#ifdef CONFIG_TMEMORY_DEBUG
			detect_memleak(tm);
			tmemory_info(tm, "state changed to stopped! Time from last submit is %lldms",
				ktime_to_ms(ktime_sub(ktime_get_real(), tm->last_submit_time)));
#endif
			if (tm->need_block) {
				tm->need_block = false;
				wakeup = true;
			}
			wake_up_all(&tm->switch_wait);
		}
		spin_unlock_irqrestore(&tm->update_lock, flags3);
		spin_unlock_irqrestore(&tm->discard_lock, flags2);
	}
	spin_unlock_irqrestore(&tm->state_lock, flags1);
	if (wakeup)
		wake_up_all(&tm->block_wait);
}

void update_position(pgoff_t *blkaddr, unsigned int *blkofs, struct bio_vec *bvec)
{
	*blkaddr += (*blkofs + bvec->bv_len) >> PAGE_SHIFT;
	*blkofs = (*blkofs + bvec->bv_len) & (PAGE_SIZE - 1);
}

void tmemory_handle_read_endio(struct tmemory_read_context *irc, unsigned int bios)
{
	struct tmemory_device *tm = irc->tm;
	struct bio *bio;
	unsigned int bi_status = irc->bi_status;

	if (atomic_add_return(bios, &irc->bio_remaining))
		return;

	if (time_to_inject(tm, FAULT_PANIC_RENDIO)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_RENDIO);
		TMEMORY_BUG_ON(1, tm, "");
	}

	bio = irc->orig_bio;
	kmem_cache_free(read_context_slab, irc);
	atomic_dec(&tm->read_slab);

	if (time_to_inject(tm, FAULT_READ_IO)) {
		tmemory_show_injection_info(tm, FAULT_READ_IO);
		bi_status = BLK_STS_IOERR;
	}

	if (bi_status) {
		tmemory_err(tm, "tmemory_handle_read_endio: bi_status:%u",
						bi_status);
		tmemory_error(TMEMORY_ERR_ENDREAD);
		bio_io_error(bio);
		dec_tmemory_remaining(tm);
		return;
	}
	bio_endio(bio);
	dec_tmemory_remaining(tm);
}

void tmemory_read_endio(struct bio *bio)
{
	struct tmemory_read_context *irc = bio->bi_private;
	struct tmemory_device *tm = irc->tm;
#if LINUX_KERNEL_515 || LINUX_KERNEL_510 || LINUX_KERNEL_504
	struct bvec_iter_all iter;
#elif LINUX_KERNEL_414  || LINUX_KERNEL_419
	int iter;
#endif
	struct bio_vec *bvec;
	unsigned int readed = 0;

	bio_for_each_segment_all(bvec, bio, iter)
		readed++;

	if (time_to_inject(tm, FAULT_PANIC_RENDIO)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_RENDIO);
		TMEMORY_BUG_ON(1, tm, "");
	}

	atomic_sub(readed, &tm->inflight_read_page);
	atomic_dec(&tm->inflight_read_bio);

	if (bio->bi_status)
		irc->bi_status = bio->bi_status;
	tmemory_handle_read_endio(irc, -1);
	bio_put(bio);
	atomic_dec(&tm->bio_cnt);
}


void *tmemory_kmem_cache_alloc(struct tmemory_device *tm,
			struct kmem_cache *cachep, gfp_t flags, bool nofail)
{
	void *obj;

	if (nofail || (flags & __GFP_NOFAIL))
		return kmem_cache_alloc(cachep, flags | __GFP_NOFAIL);

	if (time_to_inject(tm, FAULT_SLAB_ALLOC)) {
		tmemory_show_injection_info(tm, FAULT_SLAB_ALLOC);
		obj = NULL;
		goto out;
	}

	obj = kmem_cache_alloc(cachep, flags);
out:
	if (!obj)
		tmemory_error(TMEMORY_ERR_ALLOC_SLAB);
	return obj;
}

struct tmemory_transaction *tmemory_start_transaction(struct tmemory_device *tm,
							int gfp_flag)
{
	struct tmemory_transaction *new_trans;

	new_trans = tmemory_kmem_cache_alloc(tm, transaction_entry_slab,
							gfp_flag, false);
	if (!new_trans) {
		tmemory_err(tm, "failed to alloc slab for new trans");
		return ERR_PTR(-ENOMEM);
	}

	atomic_inc(&tm->trans_slab);

	INIT_LIST_HEAD(&new_trans->list);
	INIT_RADIX_TREE(&new_trans->space, GFP_ATOMIC);
	new_trans->start_time = jiffies;
	new_trans->commit_time = new_trans->start_time - 1;
	atomic_set(&new_trans->nr_pages, 0);
	new_trans->sync_trans = false;
	new_trans->freed = false;

	tmemory_debug(tm, "new_transation, %p, start:%lu, commit:%lu\n",
		new_trans, new_trans->start_time, new_trans->commit_time);

	return new_trans;
}

int tmemory_stop_transaction(struct tmemory_device *tm, bool sync, bool fua)
{
	struct tmemory_transaction *old_trans, *new_trans;
	unsigned int flushmerge_no;
	bool empty;

	down_read(&tm->commit_rwsem);
	if (time_to_inject(tm, FAULT_PANIC_COMMIT_RWSEM)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_COMMIT_RWSEM);
		TMEMORY_BUG_ON(1, tm, "");
	}
	flushmerge_no = atomic_read(&tm->flushmerge_no);
	old_trans = latest_trans(tm);
	empty = radix_tree_empty(&old_trans->space);
	up_read(&tm->commit_rwsem);
	if (empty)
		goto out;

	new_trans = tmemory_start_transaction(tm, GFP_NOIO);
	if (IS_ERR(new_trans))
		return PTR_ERR(new_trans);

	if (atomic_inc_return(&tm->flushmerge_waiters) >
					TMEMORY_FLUSH_MERGE_WAITERS) {
		int ret;
		int retry = TMEMORY_FLUSH_MERGE_RETRIES;

		do {
			io_schedule();
			down_read(&tm->commit_rwsem);
			ret = atomic_read(&tm->flushmerge_no) != flushmerge_no;
			up_read(&tm->commit_rwsem);

			// inject: multiple FUA, other tasks increase flushmerge_waiters
			if (time_to_inject(tm, FAULT_DELAY)) {
				tmemory_show_injection_info(tm, FAULT_DELAY);
				might_sleep();
				msleep(get_random_u32() & 0x3ff);
			}

			if (ret) {
				kmem_cache_free(transaction_entry_slab, new_trans);
				atomic_dec(&tm->trans_slab);
				goto out;
			}
		} while (--retry);
	}

	down_write(&tm->commit_rwsem);
	if (time_to_inject(tm, FAULT_PANIC_COMMIT_RWSEM)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_COMMIT_RWSEM);
		TMEMORY_BUG_ON(1, tm, "");
	}
	old_trans = latest_trans(tm);
	old_trans->commit_time = jiffies;
	old_trans->sync_trans = sync;
	old_trans->is_fua = fua;
	list_add(&new_trans->list, &old_trans->list);
	up_write(&tm->commit_rwsem);

	if (time_to_inject(tm, FAULT_DELAY)) {
		tmemory_show_injection_info(tm, FAULT_DELAY);
		might_sleep();
		msleep(get_random_u32() & 0x3ff);
	}

	atomic_set(&tm->flushmerge_waiters, 0);
	atomic_inc(&tm->flushmerge_no);
out:
	return 0;
}

static inline void tmemory_copy_page(struct page *dst, struct page *src,
					unsigned int ofs, unsigned int len)
{
	void *ksrc, *kdst;

	kdst = kmap_atomic(dst);
	ksrc = kmap_atomic(src);

	memcpy(kdst + ofs, ksrc, len);

	kunmap_atomic(kdst);
	kunmap_atomic(ksrc);
}

static bool bio_is_encrypted(struct bio *bio)
{
	return bio_has_crypt_ctx(bio);
}

int do_read_bio(struct tmemory_device *tm, struct bio *bio,
			pgoff_t blkaddr, unsigned int blkofs,
			bool *need_endio)
{
	struct tmemory_read_context *irc;
	struct bio *rbio = NULL;
	struct bvec_iter iter;
	struct bio_vec bvec;
	unsigned int nrsegs = bio_segments(bio);
	unsigned int nrbios = 0;
	struct radix_tree_root *root = &tm->global_space;
#ifdef CONFIG_TMEMORY_CRYPTO
	bool is_encrypted_dio = false;
	pgoff_t page_lba = 0;
#endif

	irc = tmemory_kmem_cache_alloc(tm, read_context_slab, GFP_NOIO, false);
	if (!irc) {
		tmemory_err(tm, "failed to alloc slab for read context");
		return -ENOMEM;
	}

	atomic_inc(&tm->read_slab);
	irc->orig_bio = bio;
	atomic_set(&irc->bio_remaining, 0);
	irc->bi_status = 0;
	irc->tm = tm;

#ifdef CONFIG_TMEMORY_CRYPTO
	if (bio_is_encrypted(bio) && bio_is_dio(bio)) {
		is_encrypted_dio = true;
		page_lba = bio->bi_crypt_context->bc_dun[0] & TMEMORY_IV_MASK;
	}
#endif

	tmemory_debug(tm, "do_read_bio, blkaddr: %lu", blkaddr);

	bio_for_each_segment(bvec, bio, iter) {
		struct page *dst = bvec.bv_page;
		struct page *src;
		unsigned long flags;
		pgoff_t pageidx;
		int ret;

		migrate_lock(tm);
		spin_lock_irqsave(&tm->update_lock, flags);
		rcu_read_lock();

		if (time_to_inject(tm, FAULT_PANIC_UPDATE_LOCK)) {
			tmemory_show_injection_info(tm, FAULT_PANIC_UPDATE_LOCK);
			TMEMORY_BUG_ON(1, tm, "");
		}

		src = radix_tree_lookup(root, blkaddr);
		if (src && src != TMEMORY_DISCARD_MARKED) {
#ifdef CONFIG_TMEMORY_CRYPTO
			int dir;
			bool need_cyrpto = false;

			if (is_encrypted_dio)
				pageidx = page_lba;
			else
				pageidx = dst->index;

			/*
			 * case 1:
			 * - GC write encrypted page into tmemory
			 * - Non-GC read decrypted page
			 */
			if (bio_is_encrypted(bio) && !page_contain_key(src)) {
				dir = TMEMORY_DECRYPT;
				need_cyrpto = true;
			}

			/*
			 * case 2:
			 * - Non-GC write decrypted page into tmemory
			 * - GC read encrypted page
			 */
			if (!bio_is_encrypted(bio) && page_contain_key(src)) {
				dir = TMEMORY_ENCRYPT;
				need_cyrpto = true;
			}

			if (need_cyrpto) {
				rcu_read_unlock();

				/* should add reference w/ update_lock */
				get_page(src);
				atomic_inc(&tm->page_cnt);
				tmemory_attach_page_key(src);

				spin_unlock_irqrestore(&tm->update_lock, flags);

				// inject: outside of updata_lock, wait discard to put page
				if (time_to_inject(tm, FAULT_DELAY)) {
					tmemory_show_injection_info(tm, FAULT_DELAY);
					might_sleep();
					msleep(get_random_u32() & 0x3ff);
				}
				ret = tmemory_read_crypted_data(tm, bio, src, dst,
						pageidx, dir, irc);
				/* should retry if fails */
				migrate_unlock(tm);
				++nrbios;
				goto next;
			}

			/*
			 * case 3:
			 * - bio needs to skip dm-crypt & page doesn't;
			 * - bio doesn't need to skip dm-crypt & page does
			 */
#ifdef CONFIG_DM_DEFAULT_KEY
			if (bio->bi_skip_dm_default_key !=
				tmemory_page_crypt_info(src).skip_dm_default_key)
				TMEMORY_BUG_ON(1, tm, "");
#endif
#endif
			tmemory_copy_page(dst, src, bvec.bv_offset, bvec.bv_len);

			flush_dcache_page(dst);

			rcu_read_unlock();
			spin_unlock_irqrestore(&tm->update_lock, flags);
			migrate_unlock(tm);

			/*
			 * once read hit the cache, it needs to submit last bio,
			 * as next page should not be merged in bio due to
			 * iv or pba is not contiguous.
			 */
			if (rbio) {
				tmemory_submit_bio(rbio, REQ_OP_READ, bio->bi_opf, tm);
				rbio = NULL;
			}
		} else {
			rcu_read_unlock();
			spin_unlock_irqrestore(&tm->update_lock, flags);
			migrate_unlock(tm);

			if (!rbio) {
				unsigned int maxpages;
repeat:
#if LINUX_KERNEL_515
				maxpages = min(nrsegs, BIO_MAX_VECS);
#elif LINUX_KERNEL_414 || LINUX_KERNEL_419 || LINUX_KERNEL_504 || LINUX_KERNEL_510
				maxpages = min_t(unsigned int, nrsegs, BIO_MAX_PAGES);
#endif
				rbio = bio_alloc(GFP_NOIO, maxpages);
				atomic_inc(&tm->bio_cnt);
				rbio->bi_opf = bio->bi_opf;
				bio_set_dev(rbio, tm->rbd);
				rbio->bi_iter.bi_sector =
						blkaddr << SECTOR_PER_PAGE_SHIFT;
				rbio->bi_end_io = tmemory_read_endio;
				rbio->bi_private = irc;

#ifdef CONFIG_TMEMORY_CRYPTO
				if (is_encrypted_dio)
					pageidx = page_lba;
				else
					pageidx = dst->index;
				dup_key_from_bio(rbio, bio, pageidx);
#endif
				++nrbios;
			}
			if (bio_add_page(rbio, dst, bvec.bv_len,
					bvec.bv_offset) < bvec.bv_len) {
				tmemory_submit_bio(rbio, REQ_OP_READ, bio->bi_opf, tm);
				goto repeat;
			}

			atomic_inc(&tm->inflight_read_page);
		}
#ifdef CONFIG_TMEMORY_CRYPTO
next:
#endif
		--nrsegs;
		update_position(&blkaddr, &blkofs, &bvec);
#ifdef CONFIG_TMEMORY_CRYPTO
		if (is_encrypted_dio)
			page_lba++;
#endif
	}

	if (IS_ENABLED(CONFIG_DEBUG_LOCK_ALLOC))
		WARN_ON(0);

	if (!nrbios) {
		*need_endio = true;
		kmem_cache_free(read_context_slab, irc);
		atomic_dec(&tm->read_slab);
		tmemory_debug(tm, "all read hit cache");
		return 0;
	}
	if (rbio)
		tmemory_submit_bio(rbio, REQ_OP_READ, bio->bi_opf, tm);

	tmemory_handle_read_endio(irc, nrbios);
	tmemory_debug(tm, "do_read_bio end");
	return 0;
}

struct tmemory_encrypt_context *tmemory_alloc_encrypt_context(struct tmemory_device *tm)
{
	struct tmemory_encrypt_context *ec;
	int count = TMEMORY_ALLOC_RETRY_NUM;

	while (count--) {
		ec = tmemory_kmem_cache_alloc(tm, encrypt_context_slab,
				__GFP_NOWARN | GFP_NOWAIT | __GFP_NORETRY,
				false);
		if (ec)
			goto initialize;
		congestion_wait(BLK_RW_ASYNC, TMEMORY_DEFAULT_IO_TIMEOUT);
		cond_resched();
	}

	ec = tmemory_kmem_cache_alloc(tm, encrypt_context_slab,
						GFP_NOIO, false);
	if (!ec) {
		tmemory_err(tm, "failed to alloc slab for encrypt context");
		return ERR_PTR(-ENOMEM);
	}

initialize:
	memset(ec, 0, sizeof(struct tmemory_encrypt_context));
	spin_lock_init(&ec->key_lock);
	atomic_set(&ec->ref, 0);
	atomic_inc(&tm->encrypt_slab);

	return ec;
}

void tmemory_free_encrypt_key(struct tmemory_device *tm,
				struct tmemory_encrypt_context *ec)
{
	if (!ec)
		return;
	kmem_cache_free(encrypt_context_slab, ec);
	atomic_dec(&tm->encrypt_slab);
}

static struct page *__tmemory_alloc_page(struct tmemory_device *tm,
							gfp_t gfp_flags)
{
	if (time_to_inject(tm, FAULT_PAGE_ALLOC)) {
		tmemory_show_injection_info(tm, FAULT_PAGE_ALLOC);
		return NULL;
	}

	return alloc_page(gfp_flags);
}

struct page *tmemory_alloc_page(struct tmemory_device *tm)
{
	struct page *page;
	int count = TMEMORY_ALLOC_RETRY_NUM;

	while (count--) {
		page = __tmemory_alloc_page(tm,
				__GFP_NOWARN | GFP_NOWAIT | __GFP_NORETRY);
		if (page)
			return page;
		congestion_wait(BLK_RW_ASYNC, TMEMORY_DEFAULT_IO_TIMEOUT);
		cond_resched();
	}

	page = __tmemory_alloc_page(tm, GFP_NOIO);
	if (!page) {
		tmemory_error(TMEMORY_ERR_ALLOC_PAGE);
		return ERR_PTR(-ENOMEM);
	}
	return page;
}

static int tmemory_radix_tree_insert(struct tmemory_device *tm,
					struct tmemory_transaction *trans,
					pgoff_t pblk, struct page *page)
{
	if (time_to_inject(tm, FAULT_RADIX_INSERT)) {
		tmemory_show_injection_info(tm, FAULT_RADIX_INSERT);
		return -EEXIST;
	}
	return radix_tree_insert(&trans->space, pblk, page);
}

struct page *register_page(struct tmemory_device *tm,
			pgoff_t lblk, pgoff_t pblk,
			void **slot, struct tmemory_transaction *trans, unsigned long *flags)
{
	struct tmemory_encrypt_context *encrypt_context;
	struct page *page, *old;
	bool released = false;
	int ret;
	unsigned int max_dirty_pages, dirty_pages;

	migrate_unlock(tm);

	if (atomic_read(&tm->dirty_pages) >=
			atomic_read(&tm->tmemory_capacity)) {
		unsigned long start_jiffies;

		up_read(&tm->commit_rwsem);
		released = true;

		start_jiffies = jiffies;

		if (!wait_event_interruptible_timeout(tm->alloc_pages_wait,
				atomic_read(&tm->dirty_pages) <
					atomic_read(&tm->tmemory_capacity),
				msecs_to_jiffies(TMEMORY_REGISTER_TIMEOUT))) {
			tmemory_info(tm, "register_page high memory usage of tmemory, pages:%d",
				atomic_read(&tm->dirty_pages));
		}

		tmemory_update_latency_stat(TMEMORY_REGISTER_PAGE_OP,
						jiffies - start_jiffies);
	}

	encrypt_context = tmemory_alloc_encrypt_context(tm);
	if (IS_ERR(encrypt_context)) {
		migrate_lock(tm);
		return ERR_PTR(-ENOMEM);
	}

	page = tmemory_alloc_page(tm);
	if (IS_ERR(page)) {
		tmemory_free_encrypt_key(tm, encrypt_context);
		migrate_lock(tm);
		return ERR_PTR(-ENOMEM);
	}
	atomic_inc(&tm->page_cnt);

	encrypt_context->lblk = lblk;

	if (released) {
		down_read(&tm->commit_rwsem);
		if (time_to_inject(tm, FAULT_PANIC_COMMIT_RWSEM)) {
			tmemory_show_injection_info(tm, FAULT_PANIC_COMMIT_RWSEM);
			TMEMORY_BUG_ON(1, tm, "");
		}
	}
	migrate_lock(tm);

	trans = latest_transaction(tm);
	spin_lock_irqsave(&tm->update_lock, *flags);
	page->index = pblk;
#ifdef CONFIG_TMEMORY_MIGRATION
	__SetPageMovable(page, &tm->mapping);
#endif
	get_page(page);
	atomic_inc(&tm->page_cnt);
	page->private = (long unsigned int)encrypt_context;
	ret = tmemory_radix_tree_insert(tm, trans, pblk, page);
	tmemory_debug(tm, "register_page, index:%ld, ret:%d", pblk, ret);
	if (ret) {
		spin_unlock_irqrestore(&tm->update_lock, *flags);
		tmemory_free_encrypt_key(tm, encrypt_context);
		page->private = 0;
		put_page(page);
		atomic_dec(&tm->page_cnt);
		put_page(page);
		atomic_dec(&tm->page_cnt);
		return ERR_PTR(-EAGAIN);
	}
#ifdef CONFIG_TMEMORY_MIGRATION
	encrypt_context->trans = trans;
#endif

	tmemory_attach_page_key(page);

	atomic_inc(&trans->nr_pages);
	atomic_inc(&tm->dirty_pages);

	max_dirty_pages = atomic_read(&tm->max_dirty_pages);
	dirty_pages = atomic_read(&tm->dirty_pages);

	if (dirty_pages > max_dirty_pages)
		atomic_set(&tm->max_dirty_pages, dirty_pages);

	atomic_inc(&tm->total_cached_pages);

	if (atomic_read(&tm->dirty_pages) >=
			atomic_read(&tm->tmemory_capacity) *
				tm->high_watermark_ratio / 100) {
		tm->need_commit = true;
		smp_mb();
		wake_up_interruptible_all(&tm->commit_trans_wait);
	}

	/* update last space as well */
	get_page(page);
	atomic_inc(&tm->page_cnt);
	slot = radix_tree_lookup_slot(&tm->global_space, pblk);
	if (!slot) {
		if (radix_tree_insert(&tm->global_space, pblk, page))
			BUG();
		goto out;
	}

retry:
	old = radix_tree_deref_slot_protected(slot, &tm->update_lock);
	if (radix_tree_exception(old)) {
		if (radix_tree_deref_retry(old))
			goto retry;
		BUG();
	} else {
		radix_tree_replace_slot(&tm->global_space, slot, page);
		if (old && old != TMEMORY_DISCARD_MARKED) {
			put_page(old);
			atomic_dec(&tm->page_cnt);
		}
	}
out:
	tmemory_debug(tm, "register_page success, pblk:%lu", pblk);
	return page;
}

/* must held rcu_read_lock() */
void hook_page(struct tmemory_device *tm, pgoff_t index, struct page *page, unsigned long *flags)
{
	struct page *old;
	void *slot;

	spin_lock_irqsave(&tm->update_lock, *flags);

	slot = radix_tree_lookup_slot(&tm->global_space, index);
	TMEMORY_BUG_ON(!slot, tm, "page %lu:%lu", page->index, index);
	old = radix_tree_deref_slot(slot);
	if (radix_tree_exception(old)) {
		TMEMORY_BUG_ON(1, tm, "");
	} else if (old == page) {
		return;
	} else {
		/*
		 * old != page, will this really happen:
		 * 1. write vs write
		 * 2. write vs discard
		 */
		TMEMORY_BUG_ON(1, tm, "");
		get_page(page);
		atomic_inc(&tm->page_cnt);
		radix_tree_replace_slot(&tm->global_space, slot, page);
		/* last page was replaced by discard */
		TMEMORY_BUG_ON(old != TMEMORY_DISCARD_MARKED, tm, "");
	}
}

int do_write_bio(struct tmemory_device *tm, struct bio *bio,
			pgoff_t blkaddr, unsigned int blkofs,
			bool *need_endio)
{
	struct tmemory_transaction *trans;
	struct radix_tree_root *space;
	struct bio_vec bvec;
	struct bvec_iter iter;
	unsigned int ofs = 0;
	int ret = 0;
#ifdef CONFIG_TMEMORY_CRYPTO
	bool is_encrypted_dio = false;
	pgoff_t page_lba = 0;
#endif

	if (!bio_segments(bio)) {
		TMEMORY_BUG_ON(*need_endio == false, tm, "need_endio is false may cause deadlock!");
		return 0;
	}

#ifdef CONFIG_TMEMORY_CRYPTO
	if (bio_is_encrypted(bio) && bio_is_dio(bio)) {
		is_encrypted_dio = true;
		page_lba = bio->bi_crypt_context->bc_dun[0] & TMEMORY_IV_MASK;
	}
#endif

	down_read(&tm->commit_rwsem);

#if 0 //OPLUS_FEATURE_SCHED_ASSIST
	if (bio->bi_opf & REQ_FG)
		atomic_inc(&tm->w_count_fg);
#endif

	if (time_to_inject(tm, FAULT_PANIC_COMMIT_RWSEM)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_COMMIT_RWSEM);
		TMEMORY_BUG_ON(1, tm, "");
	}
	trans = latest_transaction(tm);

	tmemory_debug(tm, "do_write_bio, trans:%p, time[%lu, %lu], index:%lu, segs:%u",
		trans, trans->start_time, trans->commit_time, blkaddr, bio_segments(bio));

	bio_for_each_segment(bvec, bio, iter) {
		struct page *src = bvec.bv_page;
		unsigned long flags, update_flags;
		int retry = TMEMORY_IO_RETRY_COUNT;
		pgoff_t pageidx;
		void **slot;
		struct page *dst = NULL;

		migrate_lock(tm);
repeat:
		rcu_read_lock();

		space = &trans->space;

		TMEMORY_BUG_ON(trans->freed, tm, "use after free on transaction");

		slot = radix_tree_lookup_slot(space, blkaddr);
		if (!slot)
			goto insert_page;

		dst = radix_tree_deref_slot(slot);
		if (!dst) {
insert_page:
			rcu_read_unlock();
			dst = register_page(tm, src->index,
					blkaddr, slot, trans, &update_flags);
			if (PTR_ERR(dst) == -EAGAIN) {
				trans = latest_transaction(tm);
				goto repeat;
			} else if (PTR_ERR(dst) == -ENOMEM) {
				unsigned long start;
				up_read(&tm->commit_rwsem);

				mutex_lock(&tm->nomem_lock);
				if (time_to_inject(tm, FAULT_PANIC_NOMEM_LOCK)) {
					tmemory_show_injection_info(tm, FAULT_PANIC_NOMEM_LOCK);
					TMEMORY_BUG_ON(1, tm, "");
				}
				tm->no_mem = true;
				smp_mb();
				wake_up_interruptible_all(&tm->commit_trans_wait);
				start = jiffies;
				wait_event_interruptible(tm->nomem_wait, !tm->no_mem);
				tmemory_update_latency_stat(TMEMORY_WAIT_MEM_OP, jiffies - start);
				mutex_unlock(&tm->nomem_lock);

				down_read(&tm->commit_rwsem);
				if (retry--) {
					trans = latest_transaction(tm);
					goto repeat;
				}
				TMEMORY_BUG_ON(1, tm, "register_page fails");
				/* should fallback to error handling */
				tmemory_warn(tm, "do_write_bio, no memory");
			} else {
				/*
				 * get latest transaction again since
				 * register_page() may unlock and re-lock
				 * commit_rwsem, then trans may get freed
				 * by flush thread
				 */
				trans = latest_transaction(tm);
			}

			if (IS_ERR(dst)) {
#if 0 //OPLUS_FEATURE_SCHED_ASSIST
				if (bio->bi_opf & REQ_FG)//move to out_unlock?
					atomic_dec(&tm->w_count_fg);
#endif
				migrate_unlock(tm);
				ret = PTR_ERR(dst);
				dst = NULL;
				goto out_unlock;
			}
		} else if (radix_tree_exception(dst)) {
			if (radix_tree_deref_retry(dst)) {
				rcu_read_unlock();
				goto repeat;
			}
			TMEMORY_BUG_ON(1, tm, "");
		} else {
			/* why shouldn't @to be a discard entry ? */
			TMEMORY_BUG_ON(dst == TMEMORY_DISCARD_ISSUED, tm, "");
			TMEMORY_BUG_ON(dst == TMEMORY_DISCARD_MARKED, tm, "");

			hook_page(tm, blkaddr, dst, &update_flags);
			rcu_read_unlock();
			tmemory_debug(tm, "hook_page, blkaddr:%lu", blkaddr);
		}

		// inject: outside of updata_lock, wait discard to put page
		if (time_to_inject(tm, FAULT_DELAY)) {
			tmemory_show_injection_info(tm, FAULT_DELAY);
			might_sleep();
			msleep(get_random_u32() & 0x3ff);
		}

		if (IS_ENABLED(CONFIG_DEBUG_LOCK_ALLOC))
			WARN_ON(0);

		TMEMORY_BUG_ON(trans->freed, tm, "use after free on transaction");

		/* no need to consider read while write */

		tmemory_copy_page(dst, src, bvec.bv_offset, bvec.bv_len);

#ifdef CONFIG_TMEMORY_CRYPTO
		spin_lock_irqsave(&tmemory_page_crypt_lock(dst), flags);
		if (time_to_inject(tm, FAULT_PANIC_PAGECRYPT_LOCK)) {
			tmemory_show_injection_info(tm, FAULT_PANIC_PAGECRYPT_LOCK);
			TMEMORY_BUG_ON(1, tm, "");
		}

		if (page_contain_key(dst))
			clean_page_key(dst);
		save_bio_key_to_page(dst, bio, ofs);
		if (is_encrypted_dio)
			pageidx = page_lba;
		else
			pageidx = page_index(src);
		tmemory_page_crypt_lblk(dst) = pageidx;
		spin_unlock_irqrestore(&tmemory_page_crypt_lock(dst), flags);
		spin_unlock_irqrestore(&tm->update_lock, update_flags);
		if (is_encrypted_dio)
			page_lba++;
#endif
		migrate_unlock(tm);

		update_position(&blkaddr, &blkofs, &bvec);
		tmemory_debug(tm, "do_write_bio next blkaddr:%lu, blkofs:%u", blkaddr, blkofs);

		ofs++;
	}

#if 0  //OPLUS_FEATURE_SCHED_ASSIST
	if (bio->bi_opf & REQ_FG)
		atomic_dec(&tm->w_count_fg);
#endif
out_unlock:
	up_read(&tm->commit_rwsem);
	*need_endio = true;
	return ret;
}

#ifdef TMEMORY_XCOPY_SUPPORT
struct tmemory_xcopy_read_context {
	struct work_struct work;
	struct tmemory_device *tm;
	struct page *page;
	pgoff_t pblk;
};

static void tmemory_xcopy_read_work(struct work_struct *work)
{
	struct tmemory_xcopy_read_context *ixrc =
		container_of(work, struct tmemory_xcopy_read_context, work);
	struct tmemory_device *tm = ixrc->tm;
	struct bio *bio;

	/* read from disk */
	bio = bio_alloc(GFP_NOIO, 1);
	atomic_inc(&tm->bio_cnt);
	bio->bi_opf = REQ_SYNC;
	bio_set_dev(bio, tm->rbd);
	bio->bi_iter.bi_sector =
			ixrc->pblk << SECTOR_PER_PAGE_SHIFT;
	bio->bi_end_io = read_endio;
	bio->bi_private = tm;

	bio->bi_crypt_context = NULL;
#ifdef CONFIG_DM_DEFAULT_KEY
	bio->bi_skip_dm_default_key = true;
#endif

	if (bio_add_page(bio, ixrc->page, PAGE_SIZE, 0) < PAGE_SIZE)
		TMEMORY_BUG_ON(1, tm, "");

	atomic_inc(&tm->inflight_xcopy_read);

	tmemory_submit_bio(bio, REQ_OP_READ, bio->bi_opf, tm);
}

int do_xcopy_bio(struct tmemory_device *tm, struct bio *bio,
			pgoff_t blkaddr, unsigned int blkofs,
			bool *need_endio)
{
	struct blk_copy_payload *payload = bio->bi_private;
	struct tmemory_transaction *trans;
	struct radix_tree_root *space;
	int size = 0;
	int i;

	TMEMORY_BUG_ON(bio_is_encrypted(bio), tm, "");

	while (size < BLK_MAX_COPY_RANGE) {
		if (!payload->src_addr[size])
			break;
		size++;
	}

	TMEMORY_BUG_ON(!size, tm, "");
	TMEMORY_BUG_ON(size > BLK_MAX_COPY_RANGE, tm, "");

	down_read(&tm->commit_rwsem);

	if (time_to_inject(tm, FAULT_PANIC_COMMIT_RWSEM)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_COMMIT_RWSEM);
		TMEMORY_BUG_ON(1, tm, "");
	}

	trans = latest_transaction(tm);

	for (i = 0; i < size; i++) {
		pgoff_t lblk = payload->src_addr[i];
		pgoff_t pblk = payload->dst_addr[i];
		struct page *orig_page = payload->pages[i];
		struct page *src, *dst;
		void **slot;
		int retry = TMEMORY_IO_RETRY_COUNT;
		unsigned long update_flags;

		migrate_lock(tm);
repeat:
		rcu_read_lock();

		space = &trans->space;

		slot = radix_tree_lookup_slot(space, pblk);
		TMEMORY_BUG_ON(slot, tm, "");

		rcu_read_unlock();

		//should never fail
		dst = register_page(tm, orig_page->index, pblk, slot, trans, &update_flags);
		if (PTR_ERR(dst) == -ENOMEM) {
			unsigned long start;

			up_read(&tm->commit_rwsem);

			mutex_lock(&tm->nomem_lock);
			tm->no_mem = true;
			smp_mb();
			wake_up_interruptible_all(&tm->commit_trans_wait);
			start = jiffies;
			wait_event_interruptible(tm->nomem_wait, !tm->no_mem);
			tmemory_update_latency_stat(TMEMORY_WAIT_MEM_OP, jiffies - start);
			mutex_unlock(&tm->nomem_lock);

			down_read(&tm->commit_rwsem);
			if (retry--) {
				trans = latest_transaction(tm);
				goto repeat;
			}
			/* should fallback to error handling */
			tmemory_warn(tm, "do_xcopy_bio() no memory");
		} else if (PTR_ERR(dst) == -EAGAIN) {
			tmemory_err(tm, "do_xcopy_bio() register_page fail err:%d",
				PTR_ERR(dst));
			trans = latest_transaction(tm);
			goto repeat;
		} else {
			/*
			 * get latest transaction again since register_page()
			 * may unlock and re-lock commit_rwsem, then trans may
			 * get freed by flush thread
			 */
			trans = latest_transaction(tm);
		}

		get_page(dst);
		atomic_inc(&tm->page_cnt);

		rcu_read_lock();

		space = &tm->global_space;

		src = radix_tree_lookup(space, blkaddr);
		if (src) {
			unsigned long src_flags, dst_flags;

			tmemory_copy_page(dst, src, 0, PAGE_SIZE);

#ifdef CONFIG_TMEMORY_CRYPTO
			spin_lock_irqsave(&tmemory_page_crypt_lock(dst), dst_flags);
			spin_lock_irqsave(&tmemory_page_crypt_lock(src), src_flags);
			if (time_to_inject(tm, FAULT_PANIC_PAGECRYPT_LOCK)) {
				tmemory_show_injection_info(tm, FAULT_PANIC_PAGECRYPT_LOCK);
				TMEMORY_BUG_ON(1, tm, "");
			}
			save_page_key_to_page(dst, src);
			tmemory_page_crypt_lblk(dst) = tmemory_page_crypt_lblk(src);
			spin_unlock_irqrestore(&tmemory_page_crypt_lock(src), src_flags);
			spin_unlock_irqrestore(&tmemory_page_crypt_lock(dst), dst_flags);
#endif
			spin_unlock_irqrestore(&tm->update_lock, update_flags);

			rcu_read_unlock();
		} else {
			struct tmemory_xcopy_read_context ixrc = { 0 };

			rcu_read_unlock();
			spin_unlock_irqrestore(&tm->update_lock, update_flags);

			ixrc.tm = tm;
			ixrc.page = dst;
			ixrc.pblk = lblk;

			lock_page(dst);

			INIT_WORK(&ixrc.work, tmemory_xcopy_read_work);
			queue_work(tm->crypto_wq, &ixrc.work);

			lock_page(dst);
			atomic_dec(&tm->inflight_xcopy_read);
			unlock_page(dst);
		}

		put_page(dst);
		atomic_dec(&tm->page_cnt);

		migrate_unlock(tm);
	}

	up_read(&tm->commit_rwsem);
	*need_endio = true;
	return 0;
}
#else
int do_xcopy_bio(struct tmemory_device *tm, struct bio *bio,
			pgoff_t blkaddr, unsigned int blkofs,
			bool *need_endio)
{
	return 0;
}
#endif

void tmemory_enable_switch(struct tmemory_device *tm)
{
	unsigned long flags;
	bool wakeup = false;
	tmemory_notice(tm, "enable the switch");
	spin_lock_irqsave(&tm->state_lock, flags);
	if (time_to_inject(tm, FAULT_PANIC_STATE_LOCK)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_STATE_LOCK);
		TMEMORY_BUG_ON(1, tm, "");
	}
	if (tm->state == TM_STATE_STOPPED) {
		if (tm->bypass_remaining == 0) {
			tm->state = TM_STATE_STARTED;

#ifdef CONFIG_TMEMORY_DEBUG
			tm->last_submit_time = ktime_get_real();
#endif
			wakeup = true;
			tmemory_info(tm, "state changed to started!");
		} else {
			tm->state = TM_STATE_STARTING;
			tmemory_info(tm, "state changed to starting!");
		}
	}
	spin_unlock_irqrestore(&tm->state_lock, flags);
	if (wakeup)
		wake_up(&tm->exec_wait); // Wake up flush thread.
}

bool need_flush(struct tmemory_device *tm);
void tmemory_sync_close(struct tmemory_device *tm, bool sync)
{
	unsigned long flags;
	tmemory_notice(tm, "wait to stop state");

	if (sync) {
		int ret;
		unsigned long start;
		tm->need_commit = true;
		smp_mb();
		wake_up_interruptible_all(&tm->commit_trans_wait);
		start = jiffies;
		ret = wait_event_interruptible_timeout(tm->switch_wait,
						       !need_flush(tm),
						       msecs_to_jiffies(atomic_read(&tm->switch_time)));

		tmemory_update_latency_stat(TMEMORY_WAIT_SWITCH_OP, jiffies - start);
		if (ret == 0) { // Timeout and not completed.
			spin_lock_irqsave(&tm->state_lock, flags);
			tm->need_block = true;
			spin_unlock_irqrestore(&tm->state_lock, flags);
			wait_event_interruptible(tm->switch_wait, !need_flush(tm));
		}
	}
}

static bool tmemory_switch_syncflag(struct tmemory_device *tm, int value)
{
	bool flag = true;
	switch (value) {
		case TMEMORY_SWITCH_FLAG_LOWBETTERY:
		case TMEMORY_SWITCH_FLAG_POWER:
		case TMEMORY_SWITCH_FLAG_PANIC:
		case TMEMORY_SWITCH_FLAG_UNREG:
		case TMEMORY_SWITCH_FLAG_MULTIUSER:
		case TMEMORY_SWITCH_FLAG_FORCE:
		case TMEMORY_SWITCH_FLAG_SHELL:
			flag = true;
			break;
		case TMEMORY_SWITCH_FLAG_RUS:
		case TMEMORY_SWITCH_FLAG_OSENSE:
			flag = false;
			break;
		default:
			flag = false;
			break;
	}

	tmemory_info(tm, "switch_syncflag:%d value:%d", flag, value);
	return flag;
}

void check_lazy_switch(struct tmemory_device *tm)
{
	int lazy_switch = 0;
	unsigned long flags1,flags2;
	tmemory_debug(tm, "check_lazy_switch:%d, jiffies:%lu", tm->lazy_switch, jiffies - tm->switch_jiffies);

	if (!tm->openedflag)
		return;

	// trylock
	if (!spin_trylock_irqsave(&tm->switch_set, flags1))
		return;

	if (!spin_trylock_irqsave(&tm->state_lock, flags2)) {
		spin_unlock_irqrestore(&tm->switch_set, flags1);
		return;
	}

	if (time_to_inject(tm, FAULT_PANIC_STATE_LOCK)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_STATE_LOCK);
		TMEMORY_BUG_ON(1, tm, "");
	}

	if (tm->state != TM_STATE_STOPPED)
		goto exit;

	if ((tm->switch_flag & TMEMORY_SWITCH_FLAG_OSENSE) != 0) {
		// auto switch when available memory
		if (tmemory_availablemem_check(tm)) {
			spin_unlock_irqrestore(&tm->state_lock, flags2);
			spin_unlock_irqrestore(&tm->switch_set, flags1);
			// 1 for enable the osense case
			tmemory_notice(tm, "tmemory_availablemem_check ok!");
			tmemory_switch_set(tm, TMEMORY_SWITCH_FLAG_OSENSE_SET, false);
			return;
		}
	}

	if (tm->lazy_switch) {
		if (jiffies >= tm->switch_jiffies) {
			tmemory_notice(tm, "check_lazy_switch timeout:%lu", jiffies - tm->switch_jiffies);
			if (tm->state == TM_STATE_STOPPED || tm->state == TM_STATE_STOPPING)
				lazy_switch = tm->lazy_switch;
			tm->lazy_switch = 0;
		}
	}

exit:
	spin_unlock_irqrestore(&tm->state_lock, flags2);
	spin_unlock_irqrestore(&tm->switch_set, flags1);

	if (lazy_switch)
		tmemory_switch_set(tm, lazy_switch|TMEMORY_SWITCH_LASTBIT, false);
}

void tmemory_switch_set(struct tmemory_device *tm, int value, bool die)
{
	unsigned long flags1,flags2;
	bool syncflag;
	bool set_disable = false;
	bool wakeup = false;

	spin_lock_irqsave(&tm->switch_set, flags1);
#ifdef CONFIG_TMEMORY_FAULT_INJECTION
	if (value == TMEMORY_SWITCH_FLAG_PANIC)
		tm->fault_info.inject_rate = 0;
#endif

	tmemory_notice(tm, "state:%d, switch_flag:0x%x, value:%d",
				tm->state, tm->switch_flag , value);
	spin_lock_irqsave(&tm->state_lock, flags2);
	if (time_to_inject(tm, FAULT_PANIC_STATE_LOCK)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_STATE_LOCK);
		TMEMORY_BUG_ON(1, tm, "");
	}

	// clear lazy switch
	tm->lazy_switch = 0;

	if (value == 1) {
		set_disable = false;
		tm->openedflag = true;
		value = TMEMORY_SWITCH_FLAG_SHELL;
		 // clear all switch_flag and switch_jiffies
		tm->switch_flag = 0;
		tm->switch_jiffies = 0;
	} else if (value == 0) {
		set_disable = true;
		value = TMEMORY_SWITCH_FLAG_SHELL;
	} else {
		set_disable = value & TMEMORY_SWITCH_LASTBIT ? false : true;
		value = (value >> 1) << 1; // drop last bit
	}

	syncflag = tmemory_switch_syncflag(tm, value);

	switch(value) {
		case TMEMORY_SWITCH_FLAG_FORCE:
			if (set_disable && die)
				tm->die = true;
			break;
		case TMEMORY_SWITCH_FLAG_RUS:
			if (!set_disable)
				tm->openedflag = true;
			break;
		case TMEMORY_SWITCH_FLAG_MULTIUSER:
		case TMEMORY_SWITCH_FLAG_LOWBETTERY:
		case TMEMORY_SWITCH_FLAG_POWER:
		case TMEMORY_SWITCH_FLAG_PANIC:
		case TMEMORY_SWITCH_FLAG_UNREG:
		case TMEMORY_SWITCH_FLAG_OSENSE:
		case TMEMORY_SWITCH_FLAG_SHELL:
			break;
		default:
			TMEMORY_BUG_ON(1, tm, "tmemory value err:%d", value);
			goto exit;
	}

	if (!tm->openedflag)
		goto exit;

	if (set_disable)
		tm->switch_flag |= value;
	else
		tm->switch_flag &= ~(value & TMEMORY_SWITCH_ALLMASK);

	if (set_disable)
	{
		switch (tm->state) {
			case TM_STATE_STARTING:
				tm->state = TM_STATE_STOPPED;
				tmemory_info(tm, "state STARTING changed to STOPPED!");
				goto exit;
			case TM_STATE_STOPPED:
				tmemory_info(tm, "state STOPPED return!");
				goto exit;
			case TM_STATE_STARTED:
				tm->state = TM_STATE_STOPPING;
				tmemory_info(tm, "state TM_STATE_STARTED changed to STOPPING!");
				fallthrough;
			case TM_STATE_STOPPING:
				tmemory_info(tm, "state STOPPING!");
				spin_unlock_irqrestore(&tm->state_lock, flags2);
				spin_unlock_irqrestore(&tm->switch_set, flags1);
				tmemory_sync_close(tm, syncflag);
				tmemory_info(tm, "state STOPPING changed to STOPPED!");
				return;
			default:
				TMEMORY_BUG_ON(1, tm, "unknown state!");
				goto exit;
		}
	} else {
		if (tm->switch_flag & TMEMORY_SWITCH_ALLMASK)
			goto exit;

		switch (tm->state) {
			case TM_STATE_STARTING:
				tmemory_info(tm, "state STARTING and return!");
				goto exit;
			case TM_STATE_STARTED:
				tmemory_info(tm, "state STARTED and return");
				goto exit;
			case TM_STATE_STOPPED:
				tmemory_info(tm, "state STOPPED!");
				if (jiffies < tm->switch_jiffies) {
					tmemory_info(tm, "jiffies no reach and lazy switch!");
					tm->lazy_switch = value;
					goto exit;
				}

				if (tm->bypass_remaining == 0) {
					tm->state = TM_STATE_STARTED;
#ifdef CONFIG_TMEMORY_DEBUG
					tm->last_submit_time = ktime_get_real();
#endif
					wakeup = true;
					tmemory_info(tm, "state STOPPED  changed to started!");
				} else {
					tm->state = TM_STATE_STARTING;
					tmemory_info(tm, "state STOPPED changed to starting!");
				}
				goto exit;
			case TM_STATE_STOPPING:
				tmemory_info(tm, "state STOPPING and set lazy_switch!");
				tm->lazy_switch = value;
				break;
			default:
				TMEMORY_BUG_ON(1, tm, "unknown state!");
				goto exit;
		}
	}

exit:
    spin_unlock_irqrestore(&tm->state_lock, flags2);
	spin_unlock_irqrestore(&tm->switch_set, flags1);
	if (wakeup)
		wake_up(&tm->exec_wait); // Wake up flush thread.
}

void tmemory_close(void)
{
	struct tmemory_device *tm;

#ifdef TMEMORY_PANIC_NOCLOSE
	return;
#endif
	tm = get_tmemory_device();
	if (!tm)
		return;
	if (tm->panic_no_close)
		return;

	tmemory_switch_set(tm, TMEMORY_SWITCH_FLAG_PANIC, false);
}
EXPORT_SYMBOL(tmemory_close);

static inline int bio_op_to_tmemory_op(unsigned int op, unsigned int opf)
{
	if (op_is_flush(opf)) {
		if (opf & REQ_FUA)
			return TMEMORY_POST_FLUSH_OP;
		if (opf & REQ_PREFLUSH)
			return TMEMORY_PRE_FLUSH_OP;
	}

	if (op == REQ_OP_DISCARD || op == REQ_OP_SECURE_ERASE)
		return TMEMORY_DISCARD_OP;
	if (op == REQ_OP_WRITE)
		return TMEMORY_WRITE_OP;
	if (op == REQ_OP_READ)
		return TMEMORY_READ_OP;
#ifdef TMEMORY_XCOPY_SUPPORT
	if (op == REQ_OP_DEVICE_COPY)
		return TMEMORY_DEVICE_COPY_OP;
#endif
#ifdef CONFIG_OPLUS_FEATURE_FBARRIER
	if (op == REQ_OP_BARRIER)
		return TMEMORY_BARRIER_OP;
#endif
	return -1;
}

static void bypass_endio(struct bio *bio)
{
	struct tmemory_bypass_context *bypass_context = bio->bi_private;
	struct tmemory_device *tm = bypass_context->tm;
	unsigned long flags;
	bool wakeup = false;

	bio->bi_private = bypass_context->origin_private;
	bio->bi_end_io = bypass_context->origin_endio;
	if (bio->bi_end_io) {
		bio->bi_end_io(bio);
	}

	spin_lock_irqsave(&tm->state_lock, flags);
	if (time_to_inject(tm, FAULT_PANIC_STATE_LOCK)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_STATE_LOCK);
		TMEMORY_BUG_ON(1, tm, "");
	}
	tm->bypass_remaining--;
	if (tm->state == TM_STATE_STARTING && tm->bypass_remaining == 0) {
		tm->state = TM_STATE_STARTED;
#ifdef CONFIG_TMEMORY_DEBUG
		tm->last_submit_time = ktime_get_real();
#endif
		wakeup = true;
		tmemory_info(tm, "state changed to started!");
	}
	spin_unlock_irqrestore(&tm->state_lock, flags);
	kmem_cache_free(bypass_context_slab, bypass_context);
	atomic_dec(&tm->bypass_ctx_slab);
	if (wakeup)
		wake_up(&tm->exec_wait); // Wake up flush thread.
}

bool is_need_block(struct tmemory_device *tm)
{
	unsigned long flags;
	bool ret;
	spin_lock_irqsave(&tm->state_lock, flags);
	ret = tm->need_block;
	spin_unlock_irqrestore(&tm->state_lock, flags);
	return ret;
}

struct process_timer {
	struct timer_list timer;
	struct task_struct *task;
};

static void process_timeout(struct timer_list *t)
{
	struct process_timer *timeout = from_timer(timeout, t, timer);

	wake_up_process(timeout->task);
}

void tmemory_wait_swith_off(struct tmemory_device *tm)
{
	struct process_timer timer;
	unsigned int expire;
	DEFINE_WAIT(wait);
	long ret;
	unsigned long long delay_inqueue;
	bool need_finish = false;

	if (!is_need_block(tm))
		return;

	delay_inqueue = current->sched_info.run_delay;
	expire = (unsigned int)atomic_read(&tm->max_block_time);
	timer.task = current;
	timer_setup_on_stack(&timer.timer, process_timeout, 0);
	timer.timer.expires = msecs_to_jiffies(expire) + jiffies;
	add_timer(&timer.timer);

	smp_mb();

	if (!is_need_block(tm))
		goto out_del;

	need_finish = true;
	ret = prepare_to_wait_event(&tm->block_wait, &wait,
					TASK_INTERRUPTIBLE);
	if (!is_need_block(tm))
		goto out_del;

	if (ret && ___wait_is_interruptible(TASK_INTERRUPTIBLE))
		goto out_del;

	io_schedule();
out_del:
	del_singleshot_timer_sync(&timer.timer);
	destroy_timer_on_stack(&timer.timer);
	if (need_finish)
		finish_wait(&tm->block_wait, &wait);
	delay_inqueue = current->sched_info.run_delay - delay_inqueue;
	tmemory_update_latency_stat(TMEMORY_SCHED_OP, delay_inqueue / NSEC_PER_MSEC);
}

#if LINUX_KERNEL_515 || LINUX_KERNEL_510
unsigned int tmemory_make_request(struct bio *bio)
#elif LINUX_KERNEL_414 || LINUX_KERNEL_419 || LINUX_KERNEL_504
blk_qc_t tmemory_make_request(struct request_queue *queue, struct bio *bio)
#endif
{
	struct tmemory_device *tm;
	struct tmemory_bypass_context *bypass_context;
	pgoff_t blkaddr;
	unsigned int blkofs;
	bool post_flush = false;
	bool need_endio = false;
	unsigned int op = bio_op(bio);
	unsigned int opf = bio->bi_opf;
	int iop;
	int err;
	unsigned long start_jiffies = jiffies;
	bool is_barrier = false;
	int state;
	unsigned long flags;

#if LINUX_KERNEL_515
	tm = (struct tmemory_device *)bio->bi_bdev->bd_disk->private_data;
#elif LINUX_KERNEL_414 || LINUX_KERNEL_419 || LINUX_KERNEL_504 || LINUX_KERNEL_510
	tm = (struct tmemory_device *)bio->bi_disk->private_data;
#endif

	tmemory_debug(tm, "make_request [op:%d] flush:%d, endidx:%llu, seg:%u",
		op, opf, bio_end_sector(bio) >> SECTOR_PER_PAGE_SHIFT,
		bio_segments(bio));

	tmemory_wait_swith_off(tm);
	tmemory_update_latency_stat(TMEMORY_BLOCK_WAIT_OP, jiffies - start_jiffies);

	spin_lock_irqsave(&tm->state_lock, flags);
	state = tm->state;
	if (state == TM_STATE_STOPPED || state == TM_STATE_STARTING) {
		tm->bypass_remaining++; // This bio will be submitted directly.
	} else {
		tm->tmemory_remaining++;
#ifdef CONFIG_TMEMORY_DEBUG
		tm->last_submit_time = ktime_get_real();
#endif
	}
	if (time_to_inject(tm, FAULT_PANIC_STATE_LOCK)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_STATE_LOCK);
		TMEMORY_BUG_ON(1, tm, "");
	}
	spin_unlock_irqrestore(&tm->state_lock, flags);

	if (state == TM_STATE_STOPPED || state == TM_STATE_STARTING) {
		bypass_context = kmem_cache_alloc(bypass_context_slab, GFP_NOIO | __GFP_NOFAIL);
		bypass_context->origin_endio = bio->bi_end_io;
		bypass_context->origin_private = bio->bi_private;
		bypass_context->tm = tm;
		atomic_inc(&tm->bypass_ctx_slab);

		bio->bi_private = bypass_context;
		bio->bi_end_io = bypass_endio;
		bio_set_dev(bio, tm->rbd);

		submit_bio(bio);

		tmemory_debug(tm, "make_request [swith off] bio: op:%d, flush:%d",
			bio_op(bio), op_is_flush(bio->bi_opf));
		goto ret;
	}

	/* check boundary between bio and device */
	if (bio_end_sector(bio) > get_capacity(tm->rbd->bd_disk)) {
		tmemory_error(TMEMORY_ERR_CHECKBOUND);
		dec_tmemory_remaining(tm);
		goto out;
	}

#ifdef CONFIG_OPLUS_FEATURE_FBARRIER
	is_barrier = bio_op(bio) == REQ_OP_BARRIER;
#endif

	if (op_is_flush(bio->bi_opf) || is_barrier) {
		if ((bio->bi_opf & REQ_PREFLUSH) || is_barrier) {
			err = tmemory_stop_transaction(tm, true, false);
			tmemory_debug(tm, "make_request [preflush], err:%d", err);
			if (err) {
				tmemory_error(TMEMORY_ERR_STOPTRANS);
				dec_tmemory_remaining(tm);
				goto out;
			}
			need_endio = true;
		}
		if (bio->bi_opf & REQ_FUA)
			post_flush = true;
	}

	blkaddr = bio->bi_iter.bi_sector >> SECTOR_PER_PAGE_SHIFT; /* byte unit */
	blkofs = (bio->bi_iter.bi_sector & (SECTOR_PER_PAGE - 1)) << SECTOR_SHIFT; /* page unit */

	tmemory_debug(tm, "make_request [op:%d] flush:%d, startidx:%lu, ofs:%u, endidx:%llu, seg:%u",
		op, op_is_flush(bio->bi_opf),
		blkaddr, blkofs, bio_end_sector(bio) >> SECTOR_PER_PAGE_SHIFT,
		bio_segments(bio));

	if (op == REQ_OP_DISCARD || op == REQ_OP_SECURE_ERASE) {
		err = do_discard_bio(tm, bio, blkaddr, blkofs, &need_endio);
		dec_tmemory_remaining(tm);
	} else if (op == REQ_OP_WRITE) {
		err = do_write_bio(tm, bio, blkaddr, blkofs, &need_endio);
		dec_tmemory_remaining(tm);
	} else if (op == REQ_OP_READ) {
		err = do_read_bio(tm, bio, blkaddr, blkofs, &need_endio);
		if (err)
			dec_tmemory_remaining(tm);
#ifdef TMEMORY_XCOPY_SUPPORT
	} else if (op == REQ_OP_DEVICE_COPY) {
		err = do_xcopy_bio(tm, bio, blkaddr, blkofs, &need_endio);
		dec_tmemory_remaining(tm);
#endif
#ifdef CONFIG_OPLUS_FEATURE_FBARRIER
	} else if (op == REQ_OP_BARRIER) {
#endif
	} else {
		tmemory_err(tm, "tmemory bypass bio op(%u)", op);
		tmemory_error(TMEMORY_ERR_NOTSUPP);
		TMEMORY_BUG_ON(1, tm, "");
		goto out;
	}

	if (!err) {
		if (post_flush) {
			err = tmemory_stop_transaction(tm, true, true);
			if (err) {
				tmemory_error(TMEMORY_ERR_STOPTRANS);
				bio_io_error(bio);
			}
		}

		if (need_endio) {
			bio_endio(bio);
			if (op == REQ_OP_READ) {
				dec_tmemory_remaining(tm);
			}
		}
		goto ret;
	}
	tmemory_error(TMEMORY_ERR_OP);
out:
	bio_io_error(bio);
ret:
	iop = bio_op_to_tmemory_op(op, opf);
	if (iop >= 0)
		tmemory_update_latency_stat(iop, jiffies - start_jiffies);
	return BLK_QC_T_NONE;
}

/* find contiguous logic pages */
int radix_tree_gang_lookup_contig(struct radix_tree_root *root,
				pgoff_t *last_index, unsigned int nrpages,
				struct page **pages)
{
	struct radix_tree_iter iter;
	struct page *page, *pre_page = NULL;
	void **slot;
	int num = 0;

	rcu_read_lock();

	radix_tree_for_each_slot(slot, root, &iter, *last_index) {
		if (!slot && *last_index != iter.index)
			break;
repeat:
		page = radix_tree_deref_slot(slot);
		if (!page)
			continue;
		if (radix_tree_exception(page)) {
			if (radix_tree_deref_retry(page)) {
				slot = radix_tree_iter_retry(&iter);
				goto repeat;
			}
			BUG();
		}

#ifdef CONFIG_TMEMORY_CRYPTO
		/*
		 * previous page is valid, it needs to check whether latter
		 * one can be merged with previous one.
		 */
		if (pre_page) {
#ifdef CONFIG_DM_DEFAULT_KEY
			/* do not merge dm-crypted page and non-dm-crypted page */
			if (tmemory_page_crypt_info(pre_page).skip_dm_default_key !=
				tmemory_page_crypt_info(page).skip_dm_default_key)
				break;
#endif

			/* do not merge encrypted page and non-encrypted one */
			if (page_contain_key(pre_page) ^ page_contain_key(page))
				break;

			/* physical block address should always be contiguous */
			if (pre_page->index + 1 != page->index)
				break;

			if (page_contain_key(pre_page) && page_contain_key(page)) {
				/* for encrypted page, LUN should be contiguous */
				if (!page_key_same(pre_page, page))
					break;
				if (tmemory_page_crypt_lblk(pre_page) + 1 !=
					tmemory_page_crypt_lblk(page))
					break;
			}
		}

#ifdef CONFIG_TMEMORY_MIGRATION
		tmemory_page_crypt(page)->trans = NULL;
#endif

#else
		/* physical block address should always be contiguous */
		if (pre_page && pre_page->index + 1 != page->index)
			break;
#endif
		pre_page = page;

		pages[num++] = page;
		*last_index = iter.index + 1;
		if (num >= nrpages)
			break;
	}
	rcu_read_unlock();

	if (IS_ENABLED(CONFIG_DEBUG_LOCK_ALLOC))
		WARN_ON(0);

	return num;
}

void wait_on_discard_page(struct tmemory_device *tm, unsigned int idx)
{
	struct page *page;
	void **slot;
	DEFINE_WAIT(wait);

	prepare_to_wait(&tm->discard_wq, &wait, TASK_UNINTERRUPTIBLE);
repeat:
	rcu_read_lock();
	slot = radix_tree_lookup_slot(&tm->discard_space, idx);
	if (!slot)
		goto out;
	page = radix_tree_deref_slot(slot);
	if (!page)
		goto out;
	if (radix_tree_exception(page)) {
		if (radix_tree_deref_retry(page)) {
			rcu_read_unlock();
			goto repeat;
		}
		BUG();
	}

	if (page != TMEMORY_DISCARD_ISSUED) {
		tmemory_err(tm, "page is not TMEMORY_DISCARD_ISSUED");
		goto out;
	}
	rcu_read_unlock();
	tmemory_debug(tm, "wait_on_discard_page discarding...wait idx:%u\n", idx);
	io_schedule();
	if (IS_ENABLED(CONFIG_DEBUG_LOCK_ALLOC))
		WARN_ON(0);
	goto repeat;
out:
	rcu_read_unlock();
	if (IS_ENABLED(CONFIG_DEBUG_LOCK_ALLOC))
		WARN_ON(0);
	finish_wait(&tm->discard_wq, &wait);
}

void tmemory_write_endio(struct bio *bio)
{
	struct tmemory_device *tm = bio->bi_private;
#if LINUX_KERNEL_515 || LINUX_KERNEL_510 || LINUX_KERNEL_504
	struct bvec_iter_all iter;
#elif LINUX_KERNEL_414 || LINUX_KERNEL_419
	int iter;
#endif
	struct bio_vec *bvec;
	int written = 0;

	if (time_to_inject(tm, FAULT_WRITE_IO)) {
		tmemory_show_injection_info(tm, FAULT_WRITE_IO);
		bio->bi_status = BLK_STS_IOERR;
	}

	if (bio->bi_status) {
		tmemory_err(tm, "tmemory_write_endio bi_status:%d", bio->bi_status);
		tmemory_error(TMEMORY_ERR_ENDWRITE);
	}

	if (time_to_inject(tm, FAULT_PANIC_WENDIO)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_WENDIO);
		TMEMORY_BUG_ON(1, tm, "");
	}

	bio_for_each_segment_all(bvec, bio, iter) {
		struct page *page = bvec->bv_page;
		unsigned long flags, page_flags;

		spin_lock_irqsave(&tm->update_lock, flags);

		if (time_to_inject(tm, FAULT_PANIC_UPDATE_LOCK)) {
			tmemory_show_injection_info(tm, FAULT_PANIC_UPDATE_LOCK);
			TMEMORY_BUG_ON(1, tm, "");
		}

#ifdef CONFIG_TMEMORY_CRYPTO
		spin_lock_irqsave(&tmemory_page_crypt_lock(page), page_flags);
		if (time_to_inject(tm, FAULT_PANIC_PAGECRYPT_LOCK)) {
			tmemory_show_injection_info(tm, FAULT_PANIC_PAGECRYPT_LOCK);
			TMEMORY_BUG_ON(1, tm, "");
		}
		clean_page_key(page);
		spin_unlock_irqrestore(&tmemory_page_crypt_lock(page), page_flags);

		tmemory_detach_page_key(tm, page);
#endif
		// inject: delay after page crypt info freed
		if (time_to_inject(tm, FAULT_DELAY)) {
			tmemory_show_injection_info(tm, FAULT_DELAY);
			mdelay(get_random_u32() & 0x7f); // 128ms
		}

		if (radix_tree_delete_item(&tm->global_space,
					page->index, page) == page) {
			put_page(page);
			atomic_dec(&tm->page_cnt);
		}
		put_page(page);
		atomic_dec(&tm->page_cnt);

		spin_unlock_irqrestore(&tm->update_lock, flags);
		written++;
	}

	atomic_sub(written, &tm->inflight_write_page);

	if (atomic_sub_return(written, &tm->dirty_pages) <
		atomic_read(&tm->tmemory_capacity))
		wake_up_all(&tm->alloc_pages_wait);

	if (!atomic_dec_return(&tm->inflight_write_bio))
		wake_up(&tm->inflight_write_wait);

	wake_up_all(&tm->wait);

	if (is_idle(tm)) {
		tm->need_commit = true;
		wake_up_interruptible_all(&tm->commit_trans_wait);
	}

	bio_put(bio);
	atomic_dec(&tm->bio_cnt);
}

int commit_transaction_page(struct tmemory_device *tm, struct bio **bio_ret,
			struct page *page, pgoff_t *pre_page_pba,
			unsigned int maxpages, pgoff_t *pre_page_lba)
{
	struct bio *bio = *bio_ret;
	unsigned long flags;
	pgoff_t cur_page_pba = page->index;
#ifdef CONFIG_TMEMORY_CRYPTO
	pgoff_t cur_page_lba = tmemory_page_crypt_lblk(page);
#endif

	TMEMORY_BUG_ON(page == TMEMORY_DISCARD_MARKED, tm, "");
	TMEMORY_BUG_ON(page == TMEMORY_DISCARD_ISSUED, tm, "");

	if (bio) {
#ifdef CONFIG_TMEMORY_CRYPTO

#ifdef CONFIG_DM_DEFAULT_KEY
		/* don't merge dm-crypt and non-dm-crypt IO */
		if (bio->bi_skip_dm_default_key !=
			tmemory_page_crypt_info(page).skip_dm_default_key)
			goto submit_bio;
#endif
		/*
		 * encrypt type mismatch:
		 * a) bio is encrypted && page is not
		 * b) page is encrypted && bio is not
		 */
		if ((bio_is_encrypted(bio) ^ page_contain_key(page)))
			goto submit_bio;

		/* bio and page are both encrypted */
		if (bio_is_encrypted(bio) && page_contain_key(page)) {
			/* key is not the same */
			if (!key_same(bio->bi_crypt_context,
					&tmemory_page_crypt_info_ctx(page)))
				goto submit_bio;

			/* logical block index is not contiguous */
			if (*pre_page_lba + 1 != cur_page_lba)
				goto submit_bio;
		}

		/* bio and page are both not encrypted */
		if (!bio_is_encrypted(bio) && !page_contain_key(page)) {
			;//merge if physical blocks are contiguous
		}
#endif

		/* physical block address is not contiguous */
		if (*pre_page_pba + 1 != cur_page_pba)
			goto submit_bio;

		goto do_merge;
	}

grab_bio:
	if (!bio) {
#ifdef CONFIG_TMEMORY_CRYPTO
		struct tmemory_crypt_info ici;
#endif
#if LINUX_KERNEL_515
		maxpages = BIO_MAX_VECS;
#elif LINUX_KERNEL_414|| LINUX_KERNEL_419 || LINUX_KERNEL_504 || LINUX_KERNEL_510
		maxpages = BIO_MAX_PAGES;
#endif
		if (!is_idle(tm))
			maxpages /= 2;
		bio = bio_alloc(GFP_NOIO, maxpages);
		atomic_inc(&tm->bio_cnt);
		bio_set_dev(bio, tm->rbd);
		//bio->bi_bdev = tm->rbd;
		bio->bi_end_io = tmemory_write_endio;
		bio->bi_iter.bi_sector = cur_page_pba << SECTOR_PER_PAGE_SHIFT;
		bio->bi_private = tm;

#ifdef CONFIG_TMEMORY_CRYPTO
		spin_lock_irqsave(&tmemory_page_crypt_lock(page), flags);
		if (time_to_inject(tm, FAULT_PANIC_PAGECRYPT_LOCK)) {
			tmemory_show_injection_info(tm, FAULT_PANIC_PAGECRYPT_LOCK);
			TMEMORY_BUG_ON(1, tm, "");
		}
		dup_key_from_page(bio, page);

		/* save crypt_info to temp variable */
		memcpy(&ici, &tmemory_page_crypt_info(page),
				sizeof(struct tmemory_crypt_info));
		//ici.ctx.bc_key = &ici.key;
		spin_unlock_irqrestore(&tmemory_page_crypt_lock(page), flags);

		// inject: use page->crypt info outside of crypt lock
		if (time_to_inject(tm, FAULT_DELAY)) {
			tmemory_show_injection_info(tm, FAULT_DELAY);
			might_sleep();
			msleep(get_random_u32() & 0x3ff);
		}

		tmemory_set_bc(bio, page);
#endif
	}

do_merge:
	if (bio_add_page(bio, page, PAGE_SIZE, 0) < PAGE_SIZE) {
submit_bio:
		tmemory_submit_bio(bio, REQ_OP_WRITE, 0, tm);
		bio = NULL;
		g_submit_times_per_tran++;

		goto grab_bio;
	}

	atomic_inc(&tm->inflight_write_page);

	*bio_ret = bio;
	*pre_page_pba = cur_page_pba;
#ifdef CONFIG_TMEMORY_CRYPTO
	*pre_page_lba = cur_page_lba;
#endif
	return 0;
}

void tmemory_delete_trans_pages(struct tmemory_device *tm,
				struct tmemory_transaction *trans,
				pgoff_t start_index, unsigned nrpages,
				struct page **pages)
{
	unsigned long flags;
	int i;

	spin_lock_irqsave(&tm->update_lock, flags);
	if (time_to_inject(tm, FAULT_PANIC_UPDATE_LOCK)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_UPDATE_LOCK);
		TMEMORY_BUG_ON(1, tm, "");
	}
	for (i = 0; i < nrpages; i++) {
		struct page *page = pages[i];
		struct page *entry;

		entry = radix_tree_delete(&trans->space, page->index);
		if (entry != page) {
			tmemory_err(tm, "tmemory tmemory_delete_trans_pages entry=%p page=%p i=%d idx=%ld, start:%lu, nrpages:%u",
					entry, page, i, page->index,
					start_index, nrpages);
		}
		TMEMORY_BUG_ON(entry != page, tm, "");
		TMEMORY_BUG_ON(entry == TMEMORY_DISCARD_MARKED, tm, "");
		TMEMORY_BUG_ON(entry == TMEMORY_DISCARD_ISSUED, tm, "");

		put_page(page);
		atomic_dec(&tm->page_cnt);
		atomic_dec(&trans->nr_pages);
	}
	spin_unlock_irqrestore(&tm->update_lock, flags);
}

int tmemory_gang_discard_replace(struct tmemory_device *tm,
				pgoff_t *index,
				pgoff_t *head,
				unsigned int maxpages)
{
	struct radix_tree_iter iter;
	unsigned long flags, update_flags;
	void **slot;
	int i = 0;
	bool found = false;

	if (radix_tree_preload(GFP_KERNEL))
		return -ENOMEM;

	spin_lock_irqsave(&tm->discard_lock, flags);
	spin_lock_irqsave(&tm->update_lock, update_flags);

	if (time_to_inject(tm, FAULT_PANIC_DISCARD_LOCK)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_DISCARD_LOCK);
		TMEMORY_BUG_ON(1, tm, "");
	}
	if (time_to_inject(tm, FAULT_PANIC_UPDATE_LOCK)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_UPDATE_LOCK);
		TMEMORY_BUG_ON(1, tm, "");
	}
	radix_tree_for_each_slot(slot, &tm->global_space, &iter, *index) {
		struct page *page = NULL;
		int err;

		if (i && *index != iter.index)
			break;
repeat:
		page = radix_tree_deref_slot_protected(slot, &tm->update_lock);
		if (unlikely(!page))
			continue;
		if (radix_tree_exception(page)) {
			if (radix_tree_deref_retry(page)) {
				slot = radix_tree_iter_retry(&iter);
				goto repeat;
			}
			BUG();
		}

		*index = iter.index + 1;
		found = true;

		if (page != TMEMORY_DISCARD_MARKED)
			break;
		if (!i)
			*head = iter.index;

		err = radix_tree_insert(&tm->discard_space, iter.index,
						TMEMORY_DISCARD_ISSUED);
		if (err == -EEXIST) {
			tmemory_info(tm, "discard entry exists, idx:%lu", iter.index);
			radix_tree_delete(&tm->global_space, iter.index);
			break;
		} else if (err) {
			tmemory_err(tm, "fail to insert discard entry, idx:%lu", iter.index);
			*index = iter.index;
		}

		radix_tree_delete(&tm->global_space, iter.index);
		if (++i >= maxpages)
			break;
	}

	spin_unlock_irqrestore(&tm->update_lock, update_flags);
	spin_unlock_irqrestore(&tm->discard_lock, flags);

	radix_tree_preload_end();
	return found ? i : -1;
}

void tmemory_discard_endio(struct bio *bio)
{
	struct tmemory_discard_context *dc = bio->bi_private;
	struct tmemory_device *tm = dc->tm;
	unsigned long flags;
	pgoff_t idx;
	void *entry;

	if (time_to_inject(tm, FAULT_COMPLETE_DISCARD)) {
		tmemory_show_injection_info(tm, FAULT_COMPLETE_DISCARD);
		bio->bi_status = BLK_STS_IOERR;
	}

	if (bio->bi_status) {
		tmemory_err(tm, "tmemory_discard_endio: bi_status:%d",
						bio->bi_status);
		tmemory_error(TMEMORY_ERR_DISCARD_COMPLETE);
	}

	spin_lock_irqsave(&tm->discard_lock, flags);
	if (time_to_inject(tm, FAULT_PANIC_DISCARD_LOCK)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_DISCARD_LOCK);
		TMEMORY_BUG_ON(1, tm, "");
	}
	if (time_to_inject(tm, FAULT_PANIC_WENDIO)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_WENDIO);
		TMEMORY_BUG_ON(1, tm, "");
	}
	for (idx = dc->head; idx < dc->head + dc->nrpages; idx++) {
		entry = radix_tree_delete_item(&tm->discard_space, idx,
						TMEMORY_DISCARD_ISSUED);
		if (entry != TMEMORY_DISCARD_ISSUED)
			BUG();
	}
	spin_unlock_irqrestore(&tm->discard_lock, flags);
	tmemory_debug(tm, "tmemory_discard_endio: %lu, %u",
					dc->head, dc->nrpages);

	atomic_dec(&tm->inflight_discard_bio);

	if (atomic_sub_return(dc->nrpages, &tm->inflight_discard_page) <=
					TMEMORY_MAX_INFLIGHT_DISCARD_PAGES)
		wake_up(&tm->inflight_discard_wait);

	wake_up_all(&tm->discard_wq);
	kmem_cache_free(discard_context_slab, dc);
	atomic_dec(&tm->discard_slab);
	bio_put(bio);
	dec_tmemory_remaining(tm);
}

int tmemory_issue_discard(struct tmemory_device *tm, pgoff_t head,
				unsigned int nrpages, gfp_t gfp_mask, int flags)
{
	struct tmemory_discard_context *dc;
	struct bio *bio = NULL;
	int err;

	dc = tmemory_kmem_cache_alloc(tm, discard_context_slab, gfp_mask, false);
	if (!dc) {
		tmemory_err(tm, "failed to alloc slab for discard context");
		err = -ENOMEM;
		goto out;
	}

	atomic_inc(&tm->discard_slab);

	dc->tm = tm;
	dc->head = head;
	dc->nrpages = nrpages;

	if (time_to_inject(tm, FAULT_ISSUE_DISCARD)) {
		tmemory_show_injection_info(tm, FAULT_ISSUE_DISCARD);
		err = -ENOMEM;
		goto fail;
	}

	err = __blkdev_issue_discard(tm->rbd, head << SECTOR_PER_PAGE_SHIFT,
					nrpages << SECTOR_PER_PAGE_SHIFT,
					gfp_mask, flags, &bio);
	if (err)
		goto fail;

	tmemory_debug(tm, "tmemory_issue_discard: %lu, %u\n",
					dc->head, dc->nrpages);

	TMEMORY_BUG_ON(!bio, tm, "");

	atomic_add(nrpages, &tm->inflight_discard_page);
	bio->bi_private = dc;
	bio->bi_end_io = tmemory_discard_endio;
	bio->bi_opf |= REQ_SYNC;
	inc_tmemory_remaining(tm);
	tmemory_submit_bio(bio, REQ_OP_DISCARD, bio->bi_opf, tm);

	return 0;
fail:
	kmem_cache_free(discard_context_slab, dc);
	atomic_dec(&tm->discard_slab);
	tmemory_err(tm, "tmemory_issue_discard: %lu, %u, err:%d",
				head, nrpages, err);
out:
	tmemory_error(TMEMORY_ERR_DISCARD_ISSUE);
	return err;

}

void do_issue_discard(struct tmemory_device *tm)
{
	int total_nr = 0;
	pgoff_t idx = 0;
	int err;

	do {
		unsigned long cur_jiffies;
		bool reach_limit;
		bool idle;
		int nrpages;
		pgoff_t head;

		if (atomic_read(&tm->inflight_discard_page) >
				TMEMORY_MAX_INFLIGHT_DISCARD_PAGES) {
			up_read(&tm->commit_rwsem);
			down_read(&tm->commit_rwsem);

			if (time_to_inject(tm, FAULT_PANIC_COMMIT_RWSEM)) {
				tmemory_show_injection_info(tm, FAULT_PANIC_COMMIT_RWSEM);
				TMEMORY_BUG_ON(1, tm, "");
			}
			idle = is_idle(tm);
			reach_limit = atomic_read(&tm->dirty_pages) >=
				atomic_read(&tm->tmemory_capacity);

			cur_jiffies = jiffies;
			if (!list_is_singular(&tm->transactions) ||
				reach_limit ||
				cur_jiffies - tm->jiffies_interval
				>= tm->threshold[idle]) {
				break;
			} else {
				unsigned long start = jiffies;
				wait_event_interruptible_timeout(
					tm->inflight_discard_wait,
					atomic_read(&tm->inflight_discard_page) <=
					TMEMORY_MAX_INFLIGHT_DISCARD_PAGES,
					tm->threshold[idle] - cur_jiffies +
					tm->jiffies_interval);
				tmemory_update_latency_stat(TMEMORY_WAIT_UNMAP_OP, jiffies - start);
				if (atomic_read(&tm->inflight_discard_page) >
					TMEMORY_MAX_INFLIGHT_DISCARD_PAGES)
					break;
			}
		}

		nrpages = tmemory_gang_discard_replace(tm, &idx, &head,
					TMEMORY_MAX_INBATCH_DISCARD_PAGES);
		if (nrpages < 0)
			break;
		if (!nrpages)
			continue;

		err = tmemory_issue_discard(tm, head, nrpages, GFP_NOIO, 0);

		total_nr += nrpages;

		if (total_nr > TMEMORY_MAX_INFLIGHT_DISCARD_PAGES) {
			tmemory_info(tm, "issue too much discard bio, %u", total_nr);
			break;
		}

		up_read(&tm->commit_rwsem);
		down_read(&tm->commit_rwsem);
	} while (1);

	up_read(&tm->commit_rwsem);
}

void putback_isolate_trans(struct tmemory_device *tm,
				struct list_head *trans, bool locked)
{
	if (!locked)
		down_write(&tm->commit_rwsem);

	list_add(trans, &tm->transactions);

	up_write(&tm->commit_rwsem);
}

static int tmemory_commit_transaction(struct tmemory_device *tm,
			struct tmemory_transaction *trans,
			unsigned int *nr, bool *one_bio_per_trans)
{
	struct page *pages[TMEMORY_PAGE_ARRAY_SIZE];
	struct bio *bio = NULL;
	pgoff_t last_index = 0;
	unsigned int nrpages;
	unsigned int submitted = 0;
	pgoff_t pre_page_idx;
	pgoff_t idx = 0;
	int i;

	g_submit_times_per_tran = 0;

	do {
		pgoff_t start_index;

		migrate_lock(tm);
		nrpages = radix_tree_gang_lookup_contig(&trans->space,
				&last_index, TMEMORY_PAGE_ARRAY_SIZE, pages);
		migrate_unlock(tm);

		if (!nrpages) {
			tmemory_debug(tm,
				"tmemory_commit_transaction, no more entries, nr:%u",
				atomic_read(&trans->nr_pages));
			break;
		}

		start_index = last_index - nrpages;

		tmemory_debug(tm, "tmemory_commit_transaction, start:%lu, entries: %u",
							start_index, nrpages);

		if (*nr < nrpages)
			nrpages = *nr;

		for (i = 0; i < nrpages; i++) {
			wait_on_discard_page(tm, start_index + i);

			commit_transaction_page(tm, &bio, pages[i], &idx, *nr,
								&pre_page_idx);
			--*nr;
			submitted++;
		}

		tmemory_delete_trans_pages(tm, trans, start_index, nrpages, pages);
	} while (*nr);

	if (bio) {
		if (g_submit_times_per_tran == 0) {
			bio->bi_opf |= REQ_FUA;
			*one_bio_per_trans = true;
		}
		tmemory_submit_bio(bio, REQ_OP_WRITE, 0, tm);
	}

	return submitted;
}

static int tmemory_blkdev_issue_flush(struct tmemory_device *tm)
{
	int ret;
	int retry = TMEMORY_FLUSH_RETRY_COUNT;

flush_again:
	if (time_to_inject(tm, FAULT_FLUSH)) {
		tmemory_show_injection_info(tm, FAULT_FLUSH);
		ret = -EIO;
		goto next;
	}

	{
#if LINUX_KERNEL_515
		ret = blkdev_issue_flush(tm->rbd);
#elif LINUX_KERNEL_510
		ret = blkdev_issue_flush(tm->rbd, GFP_KERNEL);
#elif LINUX_KERNEL_414 || LINUX_KERNEL_419 || LINUX_KERNEL_504
		sector_t sector;

		ret = blkdev_issue_flush(tm->rbd, GFP_KERNEL, &sector);
#endif
	}

next:
	if (ret) {
		tmemory_err(tm, "blkdev_issue_flush failed");
		tmemory_error(TMEMORY_ERR_BLKFLUSH);
	}

	if (retry--) {
		cond_resched();
		goto flush_again;
	}

	return 0;
}

static void tmemory_update_io_stat(struct tmemory_device *tm, unsigned int count,
					bool flush)
{
	int power = 1;
	int i = 0;

	do {
		if (count <= power)
			break;
		power <<= 1;
	} while (++i < TMEMORY_STAT_SIZE - 1);

	/*
	 * i == TMEMORY_STAT_SIZE - 1 means count > power, record to
	 * last slot of array.
	 */

	if (flush) {
		tm->pages_per_flush[i]++;
		if (count > tm->max_flush_pages)
			tm->max_flush_pages = count;
	} else {
		tm->pages_per_trans[i]++;
		if (count > tm->max_trans_pages)
			tm->max_trans_pages = count;
	}
}

int tmemory_commit_transactions(struct tmemory_device *tm, unsigned int nr,
			bool reclaim, bool trim, bool nowait, bool is_stopping)
{
	struct tmemory_transaction *trans = NULL;
	struct list_head isolated_trans;
	unsigned int submitted = 0;
	bool locked;
	int ret;
	bool one_bio_per_trans = false;
	unsigned int tot_uppages;

	if (nowait) {
		if (!mutex_trylock(&tm->commit_lock))
			return -EAGAIN;
	} else {
		mutex_lock(&tm->commit_lock);
	}

	if (time_to_inject(tm, FAULT_PANIC_COMMIT_LOCK)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_COMMIT_LOCK);
		TMEMORY_BUG_ON(1, tm, "");
	}

	inc_tmemory_remaining(tm);

	if (reclaim || is_stopping) {
		trans = tmemory_start_transaction(tm, GFP_NOIO);
		if (IS_ERR(trans))
			trans = NULL;
	}

	locked = false;

	down_write(&tm->commit_rwsem);
	if (time_to_inject(tm, FAULT_PANIC_COMMIT_RWSEM)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_COMMIT_RWSEM);
		TMEMORY_BUG_ON(1, tm, "");
	}
	tmemory_update_io_stat(tm, atomic_read(&tm->dirty_pages), true);

	if ((reclaim && (trans || list_is_singular(&tm->transactions))) ||
								is_stopping) {
		isolated_trans = tm->transactions;
		isolated_trans.prev->next = &isolated_trans;
		isolated_trans.next->prev = &isolated_trans;
		INIT_LIST_HEAD(&tm->transactions);

		if (trans) {
			list_add(&trans->list, &tm->transactions);
			up_write(&tm->commit_rwsem);
		} else {
			locked = true;
		}
	} else {
		/* isolate trans except last transaction */
		trans = latest_transaction(tm);
		list_cut_position(&isolated_trans, &tm->transactions,
							trans->list.prev);
		up_write(&tm->commit_rwsem);
	}

	tot_uppages = atomic_read(&tm->dirty_pages);

	while (!list_empty(&isolated_trans)) {
		struct blk_plug plug;
		unsigned long _start_jiffies;

		if (tm->config & TMEMORY_CONFIG_FORCE_REORDER_IO)
			trans = list_last_entry(&isolated_trans,
					struct tmemory_transaction, list);
		else
			trans = list_first_entry(&isolated_trans,
					struct tmemory_transaction, list);

		list_del(&trans->list);

		tmemory_debug(tm, "trans:%p, time[%lu, %lu] space nrpages:%d",
			trans,
			trans->start_time, trans->commit_time,
			atomic_read(&trans->nr_pages));

		if (radix_tree_empty(&trans->space)) {
			tmemory_debug(tm, "trans is empty, free it, %ld, %ld",
				trans->start_time, trans->commit_time);
			kmem_cache_free(transaction_entry_slab, trans);
			atomic_dec(&tm->trans_slab);
			continue;
		}

		tmemory_update_io_stat(tm, atomic_read(&trans->nr_pages), false);

		blk_start_plug(&plug);
		ret = tmemory_commit_transaction(tm, trans, &nr,
						&one_bio_per_trans);
		blk_finish_plug(&plug);
		tmemory_info(tm, "commit transation, submitted:%d, ios:%d",
						ret, atomic_read(&tm->inflight_write_bio));
		if (ret < 0 || !radix_tree_empty(&trans->space)) {
			list_add(&trans->list, &isolated_trans);
			tmemory_err(tm, "tmemory_commit_transaction readd, ret:%d", ret);
			tmemory_error(TMEMORY_ERR_TRANSACTION);
			goto err;
		}

		_start_jiffies = jiffies;
		io_wait_event(tm->inflight_write_wait,
				!atomic_read(&tm->inflight_write_bio));
		tmemory_update_latency_stat(TMEMORY_TRANS_WAIT_OP,
						jiffies - _start_jiffies);
		tmemory_info(tm, "io_wait_event:%u", atomic_read(&tm->inflight_write_bio));

		/* issue flush only if transaction was committed */
		if (time_after(trans->commit_time, trans->start_time)) {
			if (trans->sync_trans && !one_bio_per_trans) {
				if (time_to_inject(tm, FAULT_DELAY)) {
					tmemory_show_injection_info(tm, FAULT_DELAY);
					might_sleep();
					msleep(get_random_u32() & 0x3ff);
				}
				tmemory_blkdev_issue_flush(tm);
			}
		}

		submitted += ret;
		if (atomic_read(&trans->nr_pages))
			BUG();

		trans->freed = true;

		kmem_cache_free(transaction_entry_slab, trans);
		atomic_dec(&tm->trans_slab);

		if (!nr) {
			tmemory_err(tm, "no more nr, should never happen");
			tmemory_error(TMEMORY_ERR_BLKFLUSH);
			goto err;
		}
	}

	if (locked) {
		TMEMORY_BUG_ON(!reclaim, tm, "");
		TMEMORY_BUG_ON(!list_empty(&tm->transactions), tm, "");

		trans = tmemory_start_transaction(tm, GFP_NOIO | __GFP_NOFAIL);
		list_add(&trans->list, &tm->transactions);
		if (trim)
			downgrade_write(&tm->commit_rwsem);
		else
			up_write(&tm->commit_rwsem);
	} else if (trim) {
		down_read(&tm->commit_rwsem);
		if (!list_is_singular(&tm->transactions)) {
			up_read(&tm->commit_rwsem);
			trim = false;
		}
	}

	if (trim)
		do_issue_discard(tm);
	mutex_unlock(&tm->commit_lock);

	if (reclaim && tm->no_mem) {
		tm->no_mem = false;
		smp_mb();
		wake_up_all(&tm->nomem_wait);
	}

	dec_tmemory_remaining(tm);
	return submitted;
err:
	TMEMORY_BUG_ON(1, tm, "tmemory_commit_transactions() fails");
	putback_isolate_trans(tm, &isolated_trans, locked);
	mutex_unlock(&tm->commit_lock);
	dec_tmemory_remaining(tm);
	return ret;
}

static int get_state(struct tmemory_device *tm)
{
	int state;
	unsigned long flags;
	spin_lock_irqsave(&tm->state_lock, flags);
	state = tm->state;
	spin_unlock_irqrestore(&tm->state_lock, flags);
	return state;
}

bool need_flush(struct tmemory_device *tm)
{
	int state = get_state(tm);
	return state == TM_STATE_STARTED || state == TM_STATE_STOPPING;
}

bool is_die(struct tmemory_device *tm)
{
	bool die;
	unsigned long flags;
	spin_lock_irqsave(&tm->state_lock, flags);
	die = tm->die;
	spin_unlock_irqrestore(&tm->state_lock, flags);
	return die;
}

int tmemory_flush_thread_func(void *data)
{
	struct tmemory_device *tm = data;
	bool idle = true, reach_limit = true;

	while (!is_die(tm) || need_flush(tm)) {
		bool is_stopping;

		if (tm->config & TMEMORY_CONFIG_FORCE_STOP_FLUSH) {
			msleep(500);
			continue;
		}

		check_lazy_switch(tm);

		if (!need_flush(tm)) {
			tmemory_debug(tm, "flush_thread don't need flush now.");
			wait_event_interruptible_timeout(tm->exec_wait,
				need_flush(tm) || is_die(tm),
				msecs_to_jiffies(TMEMORY_WAIT_BYPASS_IO_TIMEOUT));
			continue;
		}

		tmemory_info(tm, "flush_thread_func start");
		is_stopping = get_state(tm) == TM_STATE_STOPPING;

		tm->jiffies_interval = jiffies;
		tmemory_info(tm, "flush_thread_func transactions start dirty_pages:%d", atomic_read(&tm->dirty_pages));
		if (reach_limit || tm->no_mem)
			tmemory_commit_transactions(tm, UINT_MAX,
						true, false, false, false);
		else
			tmemory_commit_transactions(tm, UINT_MAX,
						false, idle || is_stopping, false, is_stopping);
		tmemory_info(tm, "flush_thread_func transactions end dirty_pages:%d", atomic_read(&tm->dirty_pages));
		tm->jiffies_interval = jiffies - tm->jiffies_interval;

		idle = is_idle(tm);
		reach_limit = atomic_read(&tm->dirty_pages) >=
				atomic_read(&tm->tmemory_capacity);
		if (is_stopping)
			continue;

		if (tm->jiffies_interval < tm->threshold[idle] &&
							!reach_limit) {
			unsigned long start = jiffies;
			long wait_time = TMEMORY_FLUSH_TIMEOUT *
				(tm->threshold[idle] -
				tm->jiffies_interval);

			tmemory_info(tm, "wait_event start time:%ld dirty_pages:%d", wait_time, atomic_read(&tm->dirty_pages));
			wait_event_interruptible_timeout(
				tm->commit_trans_wait,
				tm->need_commit | tm->no_mem,
				wait_time);
			tmemory_update_latency_stat(TMEMORY_BG_TIMEOUT_OP, jiffies - start);
			tmemory_info(tm, "wait_event end dirty_pages:%d", atomic_read(&tm->dirty_pages));
			if (tm->need_commit)
				tm->need_commit = false;
		}
	}
	tmemory_info(tm, "flush_thread exited!");
	return 0;
}
