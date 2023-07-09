#include "tmemory.h"

#include <linux/reboot.h>
#include <linux/module.h>

int tmemory_major = 0;
int max_part = 1;
int devices_num = 0;

struct block_device_operations tmemory_fops = {
#if LINUX_KERNEL_515 || LINUX_KERNEL_510
	.submit_bio = tmemory_make_request,
#endif
	.owner = THIS_MODULE,
};

struct delayed_work commit_dwork;

/* tmemory shrinker */
static struct shrinker tmemory_shrinker_info = {
	.scan_objects = tmemory_shrink_scan,
	.count_objects = tmemory_shrink_count,
	.seeks = DEFAULT_SEEKS,
};

struct kmem_cache *transaction_entry_slab;
struct kmem_cache *discard_context_slab;
struct kmem_cache *read_context_slab;
struct kmem_cache *encrypt_context_slab;
struct kmem_cache *crypto_context_slab;
struct kmem_cache *bypass_context_slab;

static inline struct kmem_cache *tmemory_kmem_cache_create(
				const char *name, size_t size)
{
	return kmem_cache_create(name, size, 0, SLAB_RECLAIM_ACCOUNT, NULL);
}

static int __init tmemory_init_transaction_cache(void)
{
	transaction_entry_slab =
		tmemory_kmem_cache_create("tmemory_transaction_entry",
					sizeof(struct tmemory_transaction));
	if (!transaction_entry_slab)
		return -ENOMEM;
	return 0;
}

static void tmemory_destroy_transaction_cache(void)
{
	kmem_cache_destroy(transaction_entry_slab);
}

static int __init tmemory_init_discard_cache(void)
{
	discard_context_slab =
		tmemory_kmem_cache_create("tmemory_discard_context",
					sizeof(struct tmemory_discard_context));
	if (!discard_context_slab)
		return -ENOMEM;
	return 0;
}

static void tmemory_destroy_discard_cache(void)
{
	kmem_cache_destroy(discard_context_slab);
}

static int __init tmemory_init_read_cache(void)
{
	read_context_slab =
		tmemory_kmem_cache_create("tmemory_read_context",
					sizeof(struct tmemory_read_context));
	if (!read_context_slab)
		return -ENOMEM;
	return 0;
}

static void tmemory_destroy_read_cache(void)
{
	kmem_cache_destroy(read_context_slab);
}

static int __init tmemory_init_encrypt_cache(void)
{
	encrypt_context_slab =
		tmemory_kmem_cache_create("tmemory_encrypt_context",
					sizeof(struct tmemory_encrypt_context));
	if (!encrypt_context_slab)
		return -ENOMEM;
	return 0;
}

static void tmemory_destroy_encrypt_cache(void)
{
	kmem_cache_destroy(encrypt_context_slab);
}

static int __init tmemory_init_crypto_cache(void)
{
	crypto_context_slab =
		tmemory_kmem_cache_create("tmemory_crypto_context",
					sizeof(struct tmemory_crypto_context));
	if (!crypto_context_slab)
		return -ENOMEM;
	return 0;
}

static int __init tmemory_init_bypass_cache(void)
{
	bypass_context_slab =
		tmemory_kmem_cache_create("tmemory_bypass_context",
					sizeof(struct tmemory_bypass_context));
	if (!bypass_context_slab)
		return -ENOMEM;
	return 0;
}

static void tmemory_destroy_crypto_cache(void)
{
	kmem_cache_destroy(crypto_context_slab);
}

static void tmemory_destroy_bypass_cache(void)
{
	kmem_cache_destroy(bypass_context_slab);
}

struct tmemory_device devices[TMEMORY_MAX_DEVICES];

struct tmemory_device *get_tmemory_device(void)
{
	int i;

	for (i = 0; i < TMEMORY_MAX_DEVICES; i++) {
		if (devices[i].queue)
			return &devices[i];
	}
	return NULL;
}

bool tmemory_enabled(struct tmemory_device *tm)
{
	bool ret;
	unsigned long flags;
	if (!tm)
		return false;
	spin_lock_irqsave(&tm->state_lock, flags);
	ret = (tm->state == TM_STATE_STARTED || tm->state == TM_STATE_STOPPING);
	spin_unlock_irqrestore(&tm->state_lock, flags);
	return ret;
}

int tmemory_schedule_delayed_work(struct delayed_work *dwork,
						unsigned long delay)
{
	/*
	 * works can be handleding in queue simutanuously
	 * queue can be freezed while userspace app was freezed for pm
	 */
	return queue_delayed_work(system_freezable_wq, dwork, delay);
}

int tmemory_commit_notify(struct notifier_block *nb,
			unsigned long action, void *data)
{
	return tmemory_schedule_delayed_work(&commit_dwork, 0);
}

struct notifier_block on_reboot_nb = {
	.notifier_call = tmemory_commit_notify,
};

bool is_tmemory_emergency(void)
{
	struct tmemory_device *tm = get_tmemory_device();
	if (!tm)
		return false;

	return tm->emergency_flag;
}

bool is_tmemory_enough(struct tmemory_device *tm)
{
	return atomic_read(&tm->dirty_pages) >
			atomic_read(&tm->high_watermark_pages);
}

unsigned int tmemory_get_used_pages(struct tmemory_device *tm)
{
	return atomic_read(&tm->dirty_pages);
}

extern struct address_space_operations tmemory_aops;

int tmemory_alloc(struct tmemory_device *tm, int devno)
{
	struct request_queue *q;
	struct gendisk *disk;

#if LINUX_KERNEL_414 || LINUX_KERNEL_510 || LINUX_KERNEL_419 || LINUX_KERNEL_504
	q = blk_alloc_queue(GFP_KERNEL);
	if (!q) {
		tmemory_err(tm, "blk_alloc_queue failed");
		return -ENOMEM;
	}
#endif

#if LINUX_KERNEL_515
	disk = blk_alloc_disk(1);
#elif LINUX_KERNEL_414 || LINUX_KERNEL_510 || LINUX_KERNEL_419 || LINUX_KERNEL_504
	disk = alloc_disk(1);
#endif
	if (!disk) {
		tmemory_err(tm, "alloc_disk failed");
		goto out_free_queue;
	}

	disk->major = tmemory_major;
	disk->first_minor = devno * max_part;
	disk->minors = 1;
	disk->fops = &tmemory_fops;
#if LINUX_KERNEL_515
	q = disk->queue;
#elif LINUX_KERNEL_414 || LINUX_KERNEL_510 || LINUX_KERNEL_419 || LINUX_KERNEL_504
	disk->queue = q;
#endif
	disk->private_data = tm;
	disk->flags = GENHD_FL_EXT_DEVT;

	sprintf(disk->disk_name, "tmemory-%d", devno);

#if LINUX_KERNEL_515
	set_capacity(disk, bdev_nr_sectors(tm->rbd));
#elif LINUX_KERNEL_510
	set_capacity(disk, tm->rbd->bd_part->nr_sects);
#elif LINUX_KERNEL_414 || LINUX_KERNEL_419 || LINUX_KERNEL_504
	set_capacity(disk, tm->rbd->bd_part->nr_sects);
	blk_queue_make_request(q, tmemory_make_request);
#endif

	blk_queue_max_hw_sectors(q, 1024);
	blk_queue_physical_block_size(q, PAGE_SIZE);
	blk_queue_logical_block_size(q, PAGE_SIZE);
	blk_queue_io_min(q, PAGE_SIZE);
	blk_queue_io_opt(q, PAGE_SIZE);

	q->limits.discard_granularity = PAGE_SIZE;
	blk_queue_max_discard_sectors(q, UINT_MAX);

	tmemory_queue_flag_set(QUEUE_FLAG_NONROT, q);
	tmemory_queue_flag_set(QUEUE_FLAG_ADD_RANDOM, q);

	if (blk_queue_discard(tm->rbd->bd_disk->queue))
		tmemory_queue_flag_set(QUEUE_FLAG_DISCARD, q);
	if (blk_queue_secure_erase(tm->rbd->bd_disk->queue))
		tmemory_queue_flag_set(QUEUE_FLAG_SECERASE, q);

	blk_queue_write_cache(q, true, true);

	tm->queue = q;
	tm->disk = disk;

	tmemory_iocontrol_init(tm);

#ifdef CONFIG_TMEMORY_MIGRATION
	address_space_init_once(&tm->mapping);
	tm->mapping.a_ops = &tmemory_aops;
	init_rwsem(&tm->migrate_lock);
#endif

	spin_lock_init(&tm->crypto_bitmap_lock);
	memset(&tm->crypto_bitmap, 0,
		sizeof(unsigned long) * TMEMORY_CRYPTO_BITMAP_SIZE);
	__set_bit_le(TMEMORY_FIRST_SB_BLKADDR, tm->crypto_bitmap);
	__set_bit_le(TMEMORY_SECOND_SB_BLKADDR, tm->crypto_bitmap);

	return 0;
out_free_queue:
#if LINUX_KERNEL_414 || LINUX_KERNEL_510 || LINUX_KERNEL_419 || LINUX_KERNEL_504
	blk_cleanup_queue(q);
#endif
	return -1;
}

ssize_t tmemory_load_device(struct kobject *kobj, struct kobj_attribute *attr,
						const char *buf, size_t len)
{
	struct tmemory_device *tm;
	struct tmemory_transaction *trans;
	const char *path;
	int ret;
	int i;
	int clen;

	//handle multi device, now we only support single tmemory device
	tm = &devices[0];
	if (tm->queue) {
		tmemory_err(tm, "tm has initialized");
		return -EINVAL;
	}

	tm->log_level = TMEMORY_LOG_LEVEL_ERROR;
	for (i = 0; i < TMEMORY_STAT_SIZE; i++) {
		tm->pages_per_flush[i] = 0;
		tm->pages_per_trans[i] = 0;
	}

	if (len >= sizeof(tm->name))
		return -EINVAL;

	memset(tm->name, 0, sizeof(tm->name));
	clen = buf[len - 1]  == '\n' ? len - 1 : len;
	path = strncpy(tm->name, buf, clen);
	if (!path) {
		tmemory_err(tm, "strncpy failed");
		return -ENOMEM;
	}

	tm->rbd = blkdev_get_by_path(path, FMODE_READ | FMODE_WRITE, tm);
	if (IS_ERR(tm->rbd)) {
		ret = PTR_ERR(tm->rbd);
		tmemory_err(tm, "blkdev_get_by_path failed, %s ret=%d", path, ret);
		tm->rbd = NULL;
		return ret;
	}

	INIT_LIST_HEAD(&tm->transactions);
	mutex_init(&tm->commit_lock);
	mutex_init(&tm->nomem_lock);
	init_rwsem(&tm->commit_rwsem);

	spin_lock_init(&tm->update_lock);
	spin_lock_init(&tm->state_lock);

	INIT_RADIX_TREE(&tm->global_space, GFP_ATOMIC);
	INIT_RADIX_TREE(&tm->discard_space, GFP_ATOMIC);

	spin_lock_init(&tm->discard_lock);
	init_waitqueue_head(&tm->discard_wq);

	atomic_set(&tm->inflight_write_bio, 0);
	atomic_set(&tm->inflight_write_page, 0);
	atomic_set(&tm->inflight_read_bio, 0);
	atomic_set(&tm->inflight_read_page, 0);
	atomic_set(&tm->inflight_discard_bio, 0);
	atomic_set(&tm->inflight_discard_page, 0);
	atomic_set(&tm->switch_time, CONFIG_TMEMORY_SWITCH_TIME_DEFAULT);

#ifdef TMEMORY_XCOPY_SUPPORT
	atomic_set(&tm->inflight_xcopy_read, 0);
#endif

	atomic_set(&tm->trans_slab, 0);
	atomic_set(&tm->discard_slab, 0);
	atomic_set(&tm->read_slab, 0);
	atomic_set(&tm->encrypt_slab, 0);
	atomic_set(&tm->crypto_slab, 0);
	atomic_set(&tm->bypass_ctx_slab, 0);
	atomic_set(&tm->bio_cnt, 0);
	atomic_set(&tm->page_cnt, 0);

	init_waitqueue_head(&tm->inflight_write_wait);
	init_waitqueue_head(&tm->alloc_pages_wait);
	init_waitqueue_head(&tm->nomem_wait);
	init_waitqueue_head(&tm->inflight_discard_wait);
	init_waitqueue_head(&tm->exec_wait);
	init_waitqueue_head(&tm->switch_wait);
	init_waitqueue_head(&tm->block_wait);

	atomic_set(&tm->dirty_pages, 0);
	atomic_set(&tm->max_dirty_pages, 0);
	atomic_set(&tm->total_cached_pages, 0);

	tm->io_depths = 0;

	atomic_set(&tm->flushmerge_waiters, 0);
	atomic_set(&tm->flushmerge_no, 0);

	atomic_set(&tm->w_count_fg, 0);
	atomic_set(&tm->max_block_time, TMEMORY_DEF_MAX_BLOCK_TIME);

	init_waitqueue_head(&tm->commit_trans_wait);
	tm->need_commit = false;
	tm->no_mem = false;

	tm->emergency_flag = false;

	tmemory_init_capacity(tm);
	tm->high_watermark_ratio = TMEMORY_HIGH_WATERMARK_RATIO;
	atomic_set(&tm->high_watermark_pages,
			atomic_read(&tm->tmemory_capacity) *
				tm->high_watermark_ratio / 100);

	tm->jiffies_interval = jiffies;
	tm->flush_busy_interval = TMEMORY_DEF_FLUSH_BUSY_INTERVAL;
	tm->flush_idle_interval = TMEMORY_DEF_FLUSH_IDLE_INTERVAL;
	tm->threshold[0] = tm->flush_busy_interval * HZ / TMEMORY_MS_PER_SEC;
	tm->threshold[1] = tm->flush_idle_interval * HZ / TMEMORY_MS_PER_SEC;

	tm->bypass_remaining = 0;
	tm->tmemory_remaining = 0;
	tm->state = TM_STATE_STOPPED;
	tm->die = false;
	tm->need_block = false;
	tm->switch_flag = TMEMORY_SWITCH_FLAG_RUS;
	tm->panic_no_close = false;

	tm->switch_jiffies = 0;
	tm->openedflag = false;
	tm->lazy_switch = 0;
	spin_lock_init(&tm->switch_set);

#ifdef CONFIG_TMEMORY_DEBUG
	atomic_set(&tm->crypt_parallels, 0);
	atomic_set(&tm->crypt_parallels_max, 0);
#endif /* CONFIG_TMEMORY_DEBUG */

	// set memory status
	atomic_set(&tm->memory_stat, MEMORY_STAT_NORMAL);

	ret = tmemory_alloc(tm, 0);
	if (ret) {
		tmemory_err(tm, "tmemory_alloc failed");
		return ret;
	}

	trans = tmemory_start_transaction(tm, GFP_NOIO);
	if (IS_ERR(trans)) {
		tmemory_err(tm, "tmemory_start_transaction failed");
		return PTR_ERR(trans);
	}

	list_add(&trans->list, &tm->transactions);

	// create flush thread
	tm->flush_thread = kthread_run(tmemory_flush_thread_func, tm,
				"tmemory-%u:%u", tmemory_major, 0);
	if (IS_ERR(tm->flush_thread)) {
		tmemory_err(tm, "flush_thread failed");
		ret = PTR_ERR(tm->flush_thread);
		tm->flush_thread = NULL;
		return ret;
	}

	ret = tmemory_init_device_sysfs(tm);
	if (ret) {
		tmemory_err(tm, "qos_thread failed");

		kthread_stop(tm->flush_thread);
		tm->flush_thread = NULL;
		return ret;
	}

	tm->crypto_wq = alloc_workqueue("crypto_wq",
						 WQ_UNBOUND | WQ_HIGHPRI,
						 num_online_cpus());
	if (!tm->crypto_wq) {
		tmemory_err(tm, "alloc_workqueue failed");

		tmemory_exit_device_sysfs(tm);
		kthread_stop(tm->flush_thread);
		tm->flush_thread = NULL;
		return ret;
	}

	tmemory_build_fault_attr(tm, 0, 0);

	tmemory_join_shrinker(tm);

	add_disk(tm->disk);

#if LINUX_KERNEL_515
	bdev_get_queue(tm->disk->part0)->ksm = bdev_get_queue(tm->rbd)->ksm;
#elif LINUX_KERNEL_414 || LINUX_KERNEL_510 || LINUX_KERNEL_419 || LINUX_KERNEL_504
	tm->disk->queue->ksm = bdev_get_queue(tm->rbd)->ksm;
#endif

	devices_num++;

	tmemory_info(tm, "setup %s as tmemory device", tm->name);

	return len;
}

ssize_t tmemory_unload_device(struct kobject *kobj, struct kobj_attribute *attr,
						const char *buf, size_t len)
{
	struct tmemory_device *tm;

	if (buf[0] != '\n' && buf[0] != '\0')
		return -EINVAL;

	tm = get_tmemory_device();
	if (!tm)
		return -EINVAL;

	tmemory_info(tm, "tmemory_unload_device %s", tm->name);
	tmemory_switch_set(tm, TMEMORY_SWITCH_FLAG_FORCE, true);

	fsync_bdev(tm->rbd);

	tmemory_leave_shrinker(tm);
	tmemory_iocontrol_exit(tm);

	tmemory_exit_device_sysfs(tm);

	if (tm->flush_thread) {
		struct task_struct *flush_thread = tm->flush_thread;

		tm->flush_thread = NULL;
		kthread_stop(flush_thread);
	}

	if (tm->disk) {
#if LINUX_KERNEL_515
		del_gendisk(tm->disk);
		blk_cleanup_disk(tm->disk);
#elif LINUX_KERNEL_414 || LINUX_KERNEL_510 || LINUX_KERNEL_419 || LINUX_KERNEL_504
		del_gendisk(tm->disk);
		blk_cleanup_queue(tm->queue);
		put_disk(tm->disk);
#endif
	}

	if (tm->rbd)
		blkdev_put(tm->rbd, FMODE_READ|FMODE_WRITE);

	if (tm->crypto_wq)
		destroy_workqueue(tm->crypto_wq);

	memset(tm, 0, sizeof(struct tmemory_device));

	devices_num--;

	return len;
}

ssize_t tmemory_register_handle(struct kobject *kobj, struct kobj_attribute *attr,
						const char *buf, size_t len)
{
	if (len == 1)
		return tmemory_unload_device(kobj, attr, buf, len);

	return tmemory_load_device(kobj, attr, buf, len);
}

static int __init tmemory_init(void)
{
	int err = 0;

	if (PAGE_SIZE != 4096) {
		pr_err("only support 4KB PAGE_SIZE");
		return -EINVAL;
	}

	tmemory_major = register_blkdev(0, "tmemory");
	if (tmemory_major <= 0) {
		pr_err("unable to get major number");
		return -EBUSY;
	}
	pr_info("tmemory major %d", tmemory_major);

	err = tmemory_init_transaction_cache();
	if (err)
		goto unregister;

	err = tmemory_init_discard_cache();
	if (err)
		goto destroy_transaction_cache;

	err = tmemory_init_read_cache();
	if (err)
		goto destroy_discard_cache;

	err = tmemory_init_encrypt_cache();
	if (err)
		goto destroy_read_cache;

	err = tmemory_init_crypto_cache();
	if (err)
		goto destroy_encrypt_cache;

	err = tmemory_init_bypass_cache();
	if (err)
		goto destroy_crypto_cache;

	err = tmemory_init_module_sysfs();
	if (err)
		goto destroy_bypass_cache;

	err = register_shrinker(&tmemory_shrinker_info);
	if (err)
		goto destroy_module_sysfs;
	pr_info("tmemory init successfully");

	return 0;
destroy_module_sysfs:
	tmemory_exit_module_sysfs();
destroy_bypass_cache:
	tmemory_destroy_bypass_cache();
destroy_crypto_cache:
	tmemory_destroy_crypto_cache();
destroy_encrypt_cache:
	tmemory_destroy_encrypt_cache();
destroy_read_cache:
	tmemory_destroy_read_cache();
destroy_discard_cache:
	tmemory_destroy_discard_cache();
destroy_transaction_cache:
	tmemory_destroy_transaction_cache();
unregister:
	unregister_blkdev(tmemory_major, "tmemory");
	pr_err("tmemory init error");
	return err;
}

static void tmemory_exit(void)
{
	if (devices_num) {
		pr_err("there is a registered device");
		return;
	}
	unregister_shrinker(&tmemory_shrinker_info);
	tmemory_exit_module_sysfs();
	tmemory_destroy_crypto_cache();
	tmemory_destroy_bypass_cache();
	tmemory_destroy_encrypt_cache();
	tmemory_destroy_read_cache();
	tmemory_destroy_discard_cache();
	tmemory_destroy_transaction_cache();
	unregister_blkdev(tmemory_major, "tmemory");
}

module_init(tmemory_init);
module_exit(tmemory_exit);

module_param(max_part, uint, 0644);
MODULE_PARM_DESC(max_part, "max part");

MODULE_AUTHOR("nobody");
MODULE_DESCRIPTION("TMemory");
MODULE_LICENSE("GPL");
