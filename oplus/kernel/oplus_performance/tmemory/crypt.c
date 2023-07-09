 #include <linux/pagemap.h>

#include "tmemory.h"

#ifdef CONFIG_TMEMORY_CRYPTO

extern struct kmem_cache *crypto_context_slab;

bool bio_is_dio(struct bio *bio)
{
#if LINUX_KERNEL_515 || LINUX_KERNEL_510 || LINUX_KERNEL_504
	return PageAnon(bio_first_page_all(bio));
#elif LINUX_KERNEL_414 || LINUX_KERNEL_419
	return PageAnon(bio_page(bio));
#endif
}

void save_bio_key_to_page(struct page *page, struct bio *bio, unsigned int ofs)
{
	struct bio_crypt_ctx *ctx = bio->bi_crypt_context;
	struct tmemory_crypt_info *ici = &tmemory_page_crypt_info(page);

	if (!ctx) {
		ici->ctx.bc_key = NULL;
	} else {
		ici->ctx = *ctx;
		ici->key = *ctx->bc_key;
		ici->ctx.bc_key = &ici->key;
		ici->ctx.bc_dun[0] += ofs;
	}

#ifdef CONFIG_DM_DEFAULT_KEY
	ici->skip_dm_default_key = bio->bi_skip_dm_default_key;
#endif
}

void save_page_key_to_page(struct page *dst, struct page *src)
{
	struct tmemory_crypt_info *dst_ici = &tmemory_page_crypt_info(dst);
	struct tmemory_crypt_info *src_ici = &tmemory_page_crypt_info(src);

	if (!src_ici->ctx.bc_key) {
		dst_ici->ctx.bc_key = NULL;
	} else {
		dst_ici->ctx = src_ici->ctx;
		dst_ici->key = src_ici->key;
		dst_ici->ctx.bc_key = &dst_ici->key;
	}

#ifdef CONFIG_DM_DEFAULT_KEY
	dst_ici->skip_dm_default_key = src_ici->skip_dm_default_key;
#endif
}

void clean_page_key(struct page *page)
{
	struct tmemory_encrypt_context *ec = tmemory_page_crypt(page);

	ec->lblk = TMEMORY_INVALID_IDX;
	memset(&ec->crypt_info, 0, sizeof(struct tmemory_crypt_info));
}

bool page_contain_key(struct page *page)
{
	struct tmemory_crypt_info *ici = &tmemory_page_crypt_info(page);

	return ici->ctx.bc_key;
}

bool key_same(struct bio_crypt_ctx *key1, struct bio_crypt_ctx *key2)
{
#if LINUX_KERNEL_515 || LINUX_KERNEL_510
	return !memcmp(key1->bc_key, key2->bc_key, sizeof(struct blk_crypto_key));
#elif LINUX_KERNEL_414 || LINUX_KERNEL_419 || LINUX_KERNEL_504
	return !memcmp(key1->bc_key, key2->bc_key, sizeof(struct blk_crypto_key)) &&
		key1->bc_ksm == key2->bc_ksm &&
		key1->bc_keyslot == key2->bc_keyslot;
#endif
}

bool page_key_same(struct page *dst, struct page *src)
{
	return key_same(&tmemory_page_crypt_info_ctx(dst),
			&tmemory_page_crypt_info_ctx(src));
}

void dup_key_from_bio(struct bio *dst, struct bio *src, pgoff_t lblk)
{
	/* bio_crypt_clone() will call bio_clone_skip_dm_default_key() anyway */
	BUG_ON(dst->bi_crypt_context);
	bio_crypt_clone(dst, src, GFP_NOIO);

	if (!src->bi_crypt_context) {
		BUG_ON(dst->bi_crypt_context);
		return;
	}

	BUG_ON(lblk > U32_MAX);

	BUG_ON(!dst->bi_crypt_context);

	dst->bi_crypt_context->bc_dun[0] =
		(src->bi_crypt_context->bc_dun[0] >> 32 << 32) | lblk;
}

void dup_key_from_page(struct bio *dst, struct page *src)
{
#ifdef CONFIG_DM_DEFAULT_KEY
	struct tmemory_crypt_info *ici = &tmemory_page_crypt_info(src);

	dst->bi_skip_dm_default_key = ici->skip_dm_default_key;
#endif
}

void tmemory_set_bc(struct bio *dst, struct page *page)
{
	struct tmemory_crypt_info *ici = &tmemory_page_crypt_info(page);

	if (!ici->ctx.bc_key) {
		dst->bi_crypt_context = NULL;
	} else {
		bio_crypt_set_ctx(dst, ici->ctx.bc_key, ici->ctx.bc_dun, GFP_NOIO);
#if LINUX_KERNEL_414 || LINUX_KERNEL_419 || LINUX_KERNEL_504
		dst->bi_crypt_context->bc_ksm = ici->ctx.bc_ksm;
		dst->bi_crypt_context->bc_keyslot = ici->ctx.bc_keyslot;
#endif
	}
}

void tmemory_attach_page_key(struct page *page)
{
	atomic_inc(&tmemory_page_crypt(page)->ref);
}

void tmemory_detach_page_key(struct tmemory_device *tm, struct page *page)
{
	if (atomic_dec_and_test(&tmemory_page_crypt(page)->ref)) {
		tmemory_free_encrypt_key(tm, tmemory_page_crypt(page));
		set_page_private(page, 0);
	}
}

void write_endio(struct bio *bio)
{
	struct tmemory_device *tm = bio->bi_private;
#if LINUX_KERNEL_515 || LINUX_KERNEL_510 || LINUX_KERNEL_504
	struct bvec_iter_all iter;
#elif LINUX_KERNEL_414 || LINUX_KERNEL_419
	int iter;
#endif
	struct bio_vec *bvec;

	if (bio->bi_status) {
		tmemory_err(tm, "write_endio bi_status:%d", bio->bi_status);
		tmemory_error(TMEMORY_ERR_ENDWRITE);
	}

	if (time_to_inject(tm, FAULT_PANIC_WENDIO)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_WENDIO);
		TMEMORY_BUG_ON(1, tm, "");
	}

	bio_for_each_segment_all(bvec, bio, iter) {
		struct page *page = bvec->bv_page;

		end_page_writeback(page);
	}

	bio_put(bio);
	atomic_dec(&tm->bio_cnt);
}

void read_endio(struct bio *bio)
{
	struct tmemory_device *tm = bio->bi_private;
#if LINUX_KERNEL_515 || LINUX_KERNEL_510 || LINUX_KERNEL_504
	struct bvec_iter_all iter;
#elif LINUX_KERNEL_414 || LINUX_KERNEL_419
	int iter;
#endif
	struct bio_vec *bvec;

	if (bio->bi_status) {
		tmemory_err(tm, "read_endio bi_status:%d", bio->bi_status);
		tmemory_error(TMEMORY_ERR_ENDREAD);
	}

	if (time_to_inject(tm, FAULT_PANIC_RENDIO)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_RENDIO);
		TMEMORY_BUG_ON(1, tm, "");
	}

	bio_for_each_segment_all(bvec, bio, iter) {
		struct page *page = bvec->bv_page;

		unlock_page(page);
	}

	bio_put(bio);
	atomic_dec(&tm->bio_cnt);
}

unsigned int crypto_lock(struct tmemory_device *tm)
{
	unsigned int cid;

	spin_lock(&tm->crypto_bitmap_lock);
	cid = find_next_zero_bit_le(tm->crypto_bitmap,
			TMEMORY_CRYPTO_BITMAP_SIZE,
			TMEMORY_CRYPTO_START_BLKADDR);
	__set_bit_le(cid, tm->crypto_bitmap);
	spin_unlock(&tm->crypto_bitmap_lock);

	TMEMORY_BUG_ON(cid >= TMEMORY_CRYPTO_BITMAP_SIZE, tm, "");

	return cid;
}

void crypto_unlock(struct tmemory_device *tm, unsigned int cid)
{
	TMEMORY_BUG_ON(!test_bit_le(cid, tm->crypto_bitmap), tm, "");

	spin_lock(&tm->crypto_bitmap_lock);
	__clear_bit_le(cid, tm->crypto_bitmap);
	spin_unlock(&tm->crypto_bitmap_lock);
}

static void tmemory_crypto_work(struct work_struct *work)
{
	struct tmemory_crypto_context *icc =
		container_of(work, struct tmemory_crypto_context, work);
	struct page *page;
	struct bio *bio;
	blk_qc_t ret;
	struct tmemory_read_context *irc = icc->irc;
	struct tmemory_device *tm = irc->tm;
	pgoff_t pageidx = icc->pageidx;
	unsigned int blkaddr;
	unsigned long flags;
#ifdef CONFIG_TMEMORY_DEBUG
	int cur = atomic_add_return(1, &tm->crypt_parallels);
	if (cur > atomic_read(&tm->crypt_parallels_max))
		atomic_set(&tm->crypt_parallels_max, cur);
#endif /* CONFIG_TMEMORY_DEBUG */

	blkaddr = crypto_lock(tm);

	page = alloc_page(GFP_NOIO | __GFP_ZERO | __GFP_NOFAIL);
	TMEMORY_BUG_ON(!page, tm, "");
	atomic_inc(&tm->page_cnt);

	memcpy(page_address(page), page_address(icc->src), PAGE_SIZE);
	set_page_writeback(page);
	
	bio = bio_alloc(GFP_NOIO, 1);
	atomic_inc(&tm->bio_cnt);
	bio_set_dev(bio, tm->rbd);
	bio->bi_iter.bi_sector = blkaddr << SECTOR_PER_PAGE_SHIFT;
	bio->bi_private = tm;
	bio->bi_end_io = write_endio;

#ifdef CONFIG_DM_DEFAULT_KEY
	bio->bi_skip_dm_default_key = true;
#endif

	if (icc->dir == TMEMORY_DECRYPT) {
		/* step #1: write encrypted page directly to magic blkaddr */
		bio->bi_crypt_context = NULL;
	} else {
		/* step #1: write raw page by encryption w/ inilne encrypt engine */

		dup_key_from_page(bio, icc->src);
		tmemory_set_bc(bio, icc->src);
	}

	atomic_inc(&tm->inflight_write_page);
	atomic_inc(&tm->inflight_write_bio);

	if (bio_add_page(bio, page, PAGE_SIZE, 0) < PAGE_SIZE)
		TMEMORY_BUG_ON(1, tm, "");
	bio_set_op_attrs(bio, REQ_OP_WRITE, REQ_SYNC);
	ret = submit_bio(bio);

	wait_on_page_writeback(page);

	if (!atomic_dec_return(&tm->inflight_write_bio))
		wake_up(&tm->inflight_write_wait);
	atomic_dec(&tm->inflight_write_page);

	/* step #2: read the raw one by decryption w/ inline encrypt engine */
	bio = bio_alloc(GFP_NOIO, 1);
	atomic_inc(&tm->bio_cnt);
	bio_set_dev(bio, tm->rbd);
	bio->bi_iter.bi_sector = blkaddr << SECTOR_PER_PAGE_SHIFT;
	bio->bi_private = tm;
		bio->bi_end_io = read_endio;

	if (icc->dir == TMEMORY_DECRYPT)
		dup_key_from_bio(bio, icc->orig_bio, pageidx);
	else
		bio->bi_crypt_context = NULL;

#ifdef CONFIG_DM_DEFAULT_KEY
	bio->bi_skip_dm_default_key = true;
#endif

	lock_page(page);

	if (bio_add_page(bio, page, PAGE_SIZE, 0) < PAGE_SIZE)
		TMEMORY_BUG_ON(1, tm, "");

	atomic_inc(&tm->inflight_read_page);
	atomic_inc(&tm->inflight_read_bio);
	bio_set_op_attrs(bio, REQ_OP_READ, REQ_SYNC);
	ret = submit_bio(bio);

	lock_page(page);

	atomic_dec(&tm->inflight_read_bio);
	atomic_dec(&tm->inflight_read_page);

	unlock_page(page);

	memcpy(page_address(icc->dst), page_address(page), PAGE_SIZE);

	put_page(page);
	atomic_dec(&tm->page_cnt);

	spin_lock_irqsave(&tm->update_lock, flags);
	tmemory_detach_page_key(tm, icc->src);
	put_page(icc->src);
	atomic_dec(&tm->page_cnt);
	spin_unlock_irqrestore(&tm->update_lock, flags);

	crypto_unlock(tm, blkaddr);

	tmemory_handle_read_endio(irc, -1);

	kmem_cache_free(crypto_context_slab, icc);
	atomic_dec(&tm->crypto_slab);
#ifdef CONFIG_TMEMORY_DEBUG
	atomic_dec(&tm->crypt_parallels);
#endif /* CONFIG_TMEMORY_DEBUG */
}

int tmemory_read_crypted_data(struct tmemory_device *tm, struct bio *orig_bio,
				struct page *src_page, struct page *dst_page,
				pgoff_t pageidx, int dir,
				struct tmemory_read_context *irc)
{
	struct tmemory_crypto_context *icc;

	icc = tmemory_kmem_cache_alloc(tm, crypto_context_slab, GFP_NOIO, true);

	TMEMORY_BUG_ON(!icc, tm, "");

	atomic_inc(&tm->crypto_slab);
	icc->src = src_page;
	icc->dst = dst_page;
	icc->dir = dir;
	icc->pageidx = pageidx;
	icc->irc = irc;
	icc->orig_bio = orig_bio;

	INIT_WORK(&icc->work, tmemory_crypto_work);
	queue_work(tm->crypto_wq, &icc->work);

	return 0;
}
#endif
