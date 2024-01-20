#ifndef __TMEMORY_H
#define __TMEMORY_H

// for debug
#define TMEMORY_DEV_DEBUG
//#define TMEMORY_XCOPY_SUPPORT

#include <linux/version.h>

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + ((c) > 255 ? 255 : (c)))
#endif

#ifndef LINUX_VERSION_CODE
#error "Need LINUX_VERSION_CODE"
#endif

#define LINUX_KERNEL_515	((LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 256)))
#define LINUX_KERNEL_510	((LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 256)))
#define LINUX_KERNEL_504	((LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 256)))
#define LINUX_KERNEL_419	((LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 256)))
#define LINUX_KERNEL_414	((LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 256)))

#if (!LINUX_KERNEL_515 && !LINUX_KERNEL_510 && !LINUX_KERNEL_419 && !LINUX_KERNEL_414 && !LINUX_KERNEL_504)
#error "Kernel version not supported"
#endif

#if LINUX_KERNEL_515 || LINUX_KERNEL_510 || LINUX_KERNEL_504
#include <linux/compiler_attributes.h>
#elif LINUX_KERNEL_414 || LINUX_KERNEL_419
#if __has_attribute(__fallthrough__)
# define fallthrough                    __attribute__((__fallthrough__))
#else
# define fallthrough                    do {} while (0)  /* fallthrough */
#endif
#endif

#include <linux/io.h>
#include <linux/blkdev.h>
#include <linux/backing-dev.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#if LINUX_KERNEL_515 || LINUX_KERNEL_510 || LINUX_KERNEL_504
#include <linux/blk-crypto.h>
#include <linux/blk-mq.h>
#endif
#include <linux/semaphore.h>
#include <linux/delay.h>

#ifdef TMEMORY_XCOPY_SUPPORT
#include <linux/blk_types.h>
#endif

#define SECTOR_SHIFT			9

#define TMEMORY_PAGE_ARRAY_SIZE		16

#define TMEMORY_MAX_DEVICES		1
#define TMEMORY_FLUSH_MERGE_WAITERS	7
#define TMEMORY_FLUSH_MERGE_RETRIES	4

#define TMEMORY_DISCARD_MARKED		((void *)-4)
#define TMEMORY_DISCARD_ISSUED		((void *)-8)

#define TMEMORY_MAX_INFLIGHT_DISCARD_PAGES	8192
#define TMEMORY_MAX_INBATCH_DISCARD_PAGES	512

#define TMEMORY_MAX_DEPTH		24
#define TMEMORY_FG_DEPTH		24
#define TMEMORY_VIP_DEPTH		1

#define TMEMORY_FLUSH_IDLE_INTERVAL	(HZ/15)
#define TMEMORY_FLUSH_BUSY_INTERVAL	(HZ)

#define TMEMORY_COMMIT_TIME		500
#define TMEMORY_COMMIT_WRITE_TIME	100
#define TMEMORY_COMMIT_FLUSH_TIME	20
#define TMEMORY_COMMIT_LASTTRANS_TIME	40
#define TMEMORY_MAKE_REQUEST_TIME	50
#define TMEMORY_STAT_SIZE		18
#define TMEMORY_REGISTER_TIMEOUT	500
#define TMEMORY_ALLOC_RETRY_NUM		3
#define TMEMORY_DEF_MAX_BLOCK_TIME	5000
#define TMEMORY_WAIT_BYPASS_IO_TIMEOUT	500

#define TMEMORY_IO_RETRY_COUNT		10

#define TMEMORY_FLUSH_RETRY_COUNT	3

#define TMEMORY_FLUSH_TIMEOUT		(500 / HZ)

#define TMEMORY_DEFAULT_CAPACITY		((64 << 10) >> 2)	/* 64MB */
#define TMEMORY_DELTA_CAPACITY			((32 << 10) >> 2)	/* 32MB */
#define TMEMORY_MIN_CAPACITY			((4 << 10) >> 2)	/* 4MB */
#define TMEMORY_MAX_CAPACITY			((512 << 10) >> 2)	/* 512MB for total 32G memory */
#define TMEMORY_HIGH_WATERMARK_RATIO		80		/* 80% */
#define TMEMORY_MAX_WATERMARK_RATIO		100

#define TMEMORY_DEF_FLUSH_BUSY_INTERVAL		1000	/* unit: ms */
#define TMEMORY_DEF_FLUSH_IDLE_INTERVAL		50	/* unit: ms */
#define TMEMORY_MAX_FLUSH_INTERVAL		5000	/* unit: ms */
#define TMEMORY_MS_PER_SEC			1000

#define TMEMORY_ANDROID_SYSTEM			1000

#define TMEMORY_SWITCH_ALLMASK            0xffff
#define TMEMORY_SWITCH_BITMASK            0xfffe
#define TMEMORY_SWITCH_LASTBIT            0x1
// LastBIT for enable or disable
#define TMEMORY_SWITCH_FLAG_SHELL      0x1 // for shell debug
#define TMEMORY_SWITCH_FLAG_LOWBETTERY 0x2
#define TMEMORY_SWITCH_FLAG_POWER      0x4
#define TMEMORY_SWITCH_FLAG_PANIC      0x8
#define TMEMORY_SWITCH_FLAG_UNREG      0x10
#define TMEMORY_SWITCH_FLAG_OSENSE     0x20
#define TMEMORY_SWITCH_FLAG_RUS        0x40
#define TMEMORY_SWITCH_FLAG_MULTIUSER  0x80
#define TMEMORY_SWITCH_FLAG_FORCE      0x100

#define TMEMORY_SWITCH_FLAG_OSENSE_SET  (TMEMORY_SWITCH_FLAG_OSENSE|TMEMORY_SWITCH_LASTBIT)
#define TMEMORY_SWITCH_FLAG_RUS_SET    (TMEMORY_SWITCH_FLAG_RUS|TMEMORY_SWITCH_LASTBIT)

#define SWITCH_FROZEN_TIME (HZ*60*30)

#define TMEMORY_OSENSE_THRESHOLD 512
#define TMEMORY_PAGE_TO_MBYTES(x) ((x) >> (20 - PAGE_SHIFT))

enum {
	TMEMORY_LOG_LEVEL_ERROR,
	TMEMORY_LOG_LEVEL_WARN,
	TMEMORY_LOG_LEVEL_NOTICE,
	TMEMORY_LOG_LEVEL_INFO,
	TMEMORY_LOG_LEVEL_DEBUG,
	TMEMORY_LOG_LEVEL_TEMP,
};

/* for IO statistics: error count, IO latency */
enum {
	TMEMORY_ERR_ENDREAD,
	TMEMORY_ERR_ENDWRITE,
	TMEMORY_ERR_STOPTRANS,
	TMEMORY_ERR_CHECKBOUND,
	TMEMORY_ERR_NOTSUPP,
	TMEMORY_ERR_DISCARD_ISSUE,
	TMEMORY_ERR_DISCARD_COMPLETE,
	TMEMORY_ERR_RADIX_PRELOAD,
	TMEMORY_ERR_TRANSACTION,
	TMEMORY_ERR_BLKFLUSH,
	TMEMORY_ERR_OP,
	TMEMORY_ERR_ALLOC_SLAB,
	TMEMORY_ERR_ALLOC_PAGE,
	TMEMORY_ERR_ITEMS
};

#define TMEMORY_TIME_STAT 	20

enum {
	TMEMORY_WRITE_OP,		/* write */
	TMEMORY_READ_OP,		/* read */
	TMEMORY_DISCARD_OP,		/* discard */
	TMEMORY_PRE_FLUSH_OP,		/* preflush */
	TMEMORY_POST_FLUSH_OP,		/* postflush == FUA */
	TMEMORY_REGISTER_PAGE_OP,	/* latency of register_page waiting for flush */
	TMEMORY_DEVICE_COPY_OP,		/* xcopy */
	TMEMORY_BARRIER_OP,		/* fbarrier */
	TMEMORY_SUBMIT_BIO_WAIT_OP,	/* wait inflight write io before submit new bio */
	TMEMORY_TRANS_WAIT_OP,		/* wait io in transaction to finish */
	TMEMORY_BLOCK_WAIT_OP,		/* wait sync block when switch off */
	TMEMORY_WAIT_SWITCH_OP,		/* wait need_flush when switch off */
	TMEMORY_WAIT_UNMAP_OP,		/* wait inflight discard io */
	TMEMORY_WAIT_MEM_OP,		/* wait reclaim pages */
	TMEMORY_BG_TIMEOUT_OP,		/* flush bg timeout */
	TMEMORY_SCHED_OP,		/* delay in queue */
	TMEMORY_MAX_OP,
};

/* congestion wait timeout value, default: 20ms */
#define	TMEMORY_DEFAULT_IO_TIMEOUT	(msecs_to_jiffies(20))

#define TMEMORY_NAME_LEN	64

/* memory status */
enum {
	MEMORY_STAT_NORMAL,
	MEMORY_STAT_URGENT,
	MEMORY_STAT_ITEMS
};

#define MEMORY_INFO_LEN	64

enum {
	FAULT_PAGE_ALLOC,
	FAULT_READ_IO,
	FAULT_ISSUE_DISCARD,
	FAULT_COMPLETE_DISCARD,
	FAULT_WRITE_IO,
	FAULT_SLAB_ALLOC,
	FAULT_RADIX_INSERT,
	FAULT_FLUSH,
	FAULT_DELAY,
	FAULT_PANIC_STATE_LOCK,
	FAULT_PANIC_DISCARD_LOCK,
	FAULT_PANIC_UPDATE_LOCK,
	FAULT_PANIC_MIGRATE_LOCK,
	FAULT_PANIC_COMMIT_RWSEM,
	FAULT_PANIC_NOMEM_LOCK,
	FAULT_PANIC_PAGECRYPT_LOCK,
	FAULT_PANIC_COMMIT_LOCK,
	FAULT_PANIC_WENDIO,
	FAULT_PANIC_RENDIO,
	FAULT_MAX,
};

#ifdef CONFIG_TMEMORY_FAULT_INJECTION
struct tmemory_fault_info {
	atomic_t inject_ops;
	unsigned int inject_rate;
	unsigned int inject_type;
};
#endif

/* for struct tmemory_device::config */
enum {
	CONFIG_FORCE_SYNC_WRITE,		/* force to tag REQ_SYNC to write IO */
	CONFIG_FORCE_STOP_FLUSH,		/* force to stop flush for background thread */
	CONFIG_FORCE_REORDER_IO,		/* force to flush write IO w/ reversed order */
};

#define TMEMORY_CONFIG_FORCE_SYNC_WRITE		(1 << CONFIG_FORCE_SYNC_WRITE)
#define TMEMORY_CONFIG_FORCE_STOP_FLUSH		(1 << CONFIG_FORCE_STOP_FLUSH)
#define TMEMORY_CONFIG_FORCE_REORDER_IO		(1 << CONFIG_FORCE_REORDER_IO)

#define	TMEMORY_CRYPTO_BITMAP_SIZE		(512 / sizeof(unsigned long))

struct tmemory_device {
	char name[TMEMORY_NAME_LEN];
	struct request_queue *queue;
	struct gendisk *disk;
	struct block_device *rbd;

	struct list_head transactions;
	struct mutex commit_lock;
	struct mutex nomem_lock;
	struct rw_semaphore commit_rwsem;	/* protect transactions list */
	int tm_enabled;
	int switch_flag;
	bool panic_no_close;

	spinlock_t update_lock;

	/* global cache space, shared with write/read/discard IOs */
	struct radix_tree_root global_space;

	/* todo: introduce sparse bitmap to take place of it */
	struct radix_tree_root discard_space;	/* record erasing page */
	/* serialize discard node modification */
	spinlock_t discard_lock;
	wait_queue_head_t discard_wq;

	/* IO statistics */
	atomic_t inflight_write_bio;		/* inflight write bio */
	atomic_t inflight_write_page;		/* inflight write page */
	atomic_t inflight_read_bio;		/* inflight read bio */
	atomic_t inflight_read_page;		/* inflight read page */
	atomic_t inflight_discard_bio;		/* inflight discard bio */
	atomic_t inflight_discard_page;		/* inflight discard page */

#ifdef TMEMORY_XCOPY_SUPPORT
	atomic_t inflight_xcopy_read;		/* inflight xcopy read bio */
#endif

	/* slab statistic */
	atomic_t trans_slab;
	atomic_t discard_slab;
	atomic_t read_slab;
	atomic_t encrypt_slab;
	atomic_t crypto_slab;
	atomic_t bypass_ctx_slab;
	atomic_t bio_cnt;
	atomic_t page_cnt;

	wait_queue_head_t inflight_write_wait;
	wait_queue_head_t alloc_pages_wait;
	wait_queue_head_t nomem_wait;
	wait_queue_head_t inflight_discard_wait;
	wait_queue_head_t exec_wait;
	wait_queue_head_t block_wait;
	struct task_struct *flush_thread;

	/* for shrinker */
	struct list_head shrink_list;
	unsigned int shrinkno;

	atomic_t max_dirty_pages;		/* max dirty pages */
	atomic_t total_cached_pages;		/* total cached pages */
	atomic_t dirty_pages;			/* total number of dirty pages */
	atomic_t cur_key_cnt;
	atomic_t tmemory_capacity;		/* tmemory capacity, unit: page */
	atomic_t high_watermark_pages;		/* high watermark page count */
	unsigned int high_watermark_ratio;	/* high watermark ratio */

	spinlock_t state_lock;
	int state;
	int bypass_remaining;
	int tmemory_remaining;
	ktime_t last_submit_time;
	wait_queue_head_t switch_wait;
	bool die;
	bool need_block;

#ifdef CONFIG_TMEMORY_DEBUG
	atomic_t crypt_parallels;
	atomic_t crypt_parallels_max;
#endif /* CONFIG_TMEMORY_DEBUG */
	atomic_t memory_stat;

	int io_depths;
	struct timer_list window_timer;	/* tmemory_iocontrol_init */
	wait_queue_head_t wait;		/* tmemory_iocontrol_init */

	atomic_t flushmerge_waiters;
	atomic_t flushmerge_no;

	atomic_t w_count_fg;

	/*for async commit */
	wait_queue_head_t commit_trans_wait;
	bool need_commit;
	bool no_mem;

	bool emergency_flag;
#ifdef CONFIG_TMEMORY_MIGRATION
	struct address_space mapping;
	struct rw_semaphore migrate_lock;
#endif
	unsigned long jiffies_interval;

	/* for flush sysfs */
	unsigned flush_busy_interval;		/* unit: ms */
	unsigned flush_idle_interval;		/* unit: ms */
	unsigned threshold[2];			/* 0 for busy, 1 for idle, unit: HZ */

	/* for stat */
	unsigned long pages_per_flush[TMEMORY_STAT_SIZE];
	unsigned long pages_per_trans[TMEMORY_STAT_SIZE];
	unsigned long max_flush_pages;
	unsigned long max_trans_pages;

	int log_level;
	atomic_t switch_time;
	atomic_t max_block_time;

	unsigned long switch_jiffies;
	bool openedflag;
	int lazy_switch;
	spinlock_t switch_set;

	unsigned int fake;

	/* For sysfs support */
	struct kobject s_kobj;			/* /sys/tmemory/<devname> */
	struct completion s_kobj_unregister;

#ifdef CONFIG_TMEMORY_FAULT_INJECTION
	struct tmemory_fault_info fault_info;
#endif

	int config;				/* tmemory config */

	struct workqueue_struct *crypto_wq;
	unsigned long crypto_bitmap[TMEMORY_CRYPTO_BITMAP_SIZE];
	spinlock_t crypto_bitmap_lock;
};

struct tmemory_stat {
	unsigned long err[TMEMORY_ERR_ITEMS];
	unsigned long request[TMEMORY_MAX_OP][TMEMORY_TIME_STAT];
};

#define TM_STATE_STOPPED	0
#define TM_STATE_STARTING	1
#define TM_STATE_STARTED	2
#define TM_STATE_STOPPING	3

struct tmemory_transaction {
	unsigned int trans_id;		/* transaction id */
	struct list_head list;
	struct radix_tree_root space;
	unsigned long start_time;
	unsigned long commit_time;
	atomic_t nr_pages;
	bool sync_trans;
	bool is_fua;
	bool freed;
};

struct tmemory_discard_context {
	struct tmemory_device *tm;
	pgoff_t head;
	unsigned int nrpages;
};

struct tmemory_read_context {
	struct bio *orig_bio;
	atomic_t bio_remaining;
	unsigned int bi_status;
	struct tmemory_device *tm;
};

struct tmemory_bypass_context {
	bio_end_io_t *origin_endio;
	void *origin_private;
	struct tmemory_device *tm;
};

#define TMEMORY_CRYPTO_MTK_49

#define TMEMORY_IV_MASK		UINT_MAX

struct tmemory_crypt_info {
	struct bio_crypt_ctx ctx;
	struct blk_crypto_key key;
/*
 *		default_key	crypto_key
 *	GC	false		false
 *	FBE	false		true
 *	OTHER	true		false
 *
 *	GC_DATA and OTHER_DATA has the same crypto_key, but has different
 *	default_key, they can not be merged in one bio
*/
#ifdef CONFIG_DM_DEFAULT_KEY
	bool skip_dm_default_key;
#endif
};

enum crypt_type {
	TMEMORY_CRYPTO_MTK,
	TMEMORY_CRYPTO_QCOM,
	TMEMORY_CRYPTO_ZEKU,
};

struct tmemory_encrypt_context {
	spinlock_t key_lock;
	pgoff_t lblk;
#ifdef CONFIG_TMEMORY_CRYPTO
	enum crypt_type type;		/* mtk, qcom, zeku */
	struct tmemory_crypt_info crypt_info;
#endif
#ifdef CONFIG_TMEMORY_MIGRATION
	struct tmemory_transaction *trans;
#endif
	atomic_t ref;
};

#ifdef CONFIG_TMEMORY_CRYPTO

#define	TMEMORY_CRYPTO_START_BLKADDR	2
#define	TMEMORY_FIRST_SB_BLKADDR	0
#define	TMEMORY_SECOND_SB_BLKADDR	1

struct tmemory_crypto_context {
	struct work_struct work;
	struct tmemory_read_context *irc;
	struct page *src, *dst;
	struct bio *orig_bio;
	pgoff_t pageidx;
	int dir;
};

enum {
	TMEMORY_ENCRYPT,
	TMEMORY_DECRYPT,
};
static inline struct tmemory_encrypt_context *tmemory_page_crypt(struct page *page)
{
	return (struct tmemory_encrypt_context *)(page->private);
}
#define tmemory_page_crypt_lock(p)		\
		tmemory_page_crypt(p)->key_lock
#define tmemory_page_crypt_info(p)		\
		tmemory_page_crypt(p)->crypt_info
#define tmemory_page_crypt_info_ctx(p)	\
		tmemory_page_crypt(p)->crypt_info.ctx
#define tmemory_page_crypt_lblk(p)		\
		tmemory_page_crypt(p)->lblk

bool bio_is_dio(struct bio *bio);
void save_bio_key_to_page(struct page *page, struct bio *bio, unsigned int ofs);
void save_page_key_to_page(struct page *dst, struct page *src);
void clean_page_key(struct page *page);

bool page_contain_key(struct page *page);
bool key_same(struct bio_crypt_ctx *key1, struct bio_crypt_ctx *key2);
bool page_key_same(struct page *dst, struct page *src);
void dup_key_from_bio(struct bio *dst, struct bio *src, pgoff_t lblk);
void dup_key_from_page(struct bio *dst, struct page *src);
void tmemory_set_bc(struct bio *dst, struct page *page);
int tmemory_read_crypted_data(struct tmemory_device *tm, struct bio *bio,
				struct page *src_page, struct page *dst_page,
				pgoff_t pageidx, int dir,
				struct tmemory_read_context *irc);
void read_endio(struct bio *bio);
void tmemory_attach_page_key(struct page *page);
void tmemory_detach_page_key(struct tmemory_device *tm, struct page *page);

#endif

#define TMEMORY_INVALID_IDX	(~0UL)

#define REQ_TMEMORY_FUA		0xffffffe

#define SECTOR_PER_PAGE_SHIFT	3
#define SECTOR_PER_PAGE		(1 << SECTOR_PER_PAGE_SHIFT)

#define TMEMORY_NODE_ID		0

/* io.c */
#if LINUX_KERNEL_515 || LINUX_KERNEL_510
unsigned int tmemory_make_request(struct bio *bio);
#elif LINUX_KERNEL_414 || LINUX_KERNEL_419 || LINUX_KERNEL_504
blk_qc_t tmemory_make_request(struct request_queue *queue, struct bio *bio);
#endif
int tmemory_flush_thread_func(void *data);
struct tmemory_transaction *tmemory_start_transaction(
				struct tmemory_device *tm, int gfp_flag);
int tmemory_stop_transaction(struct tmemory_device *tm, bool sync, bool fua);
int tmemory_commit_transactions(struct tmemory_device *tm, unsigned int nr,
			bool reclaim, bool issuediscard, bool trycommit, bool is_stopping);
void tmemory_handle_read_endio(struct tmemory_read_context *irc, unsigned int bios);
void *tmemory_kmem_cache_alloc(struct tmemory_device *tm,
			struct kmem_cache *cachep, gfp_t flags, bool nofail);

/* util.c */
void tmemory_wait(struct tmemory_device *tm);
bool is_idle(struct tmemory_device *tm);
void tmemory_iocontrol_init(struct tmemory_device *tm);
void tmemory_iocontrol_exit(struct tmemory_device *tm);

/* shrinker */
void tmemory_join_shrinker(struct tmemory_device *tm);
void tmemory_leave_shrinker(struct tmemory_device *tm);
unsigned long tmemory_shrink_count(struct shrinker *shrink,
				struct shrink_control *sc);
unsigned long tmemory_shrink_scan(struct shrinker *shrink,
				struct shrink_control *sc);

/* main.c */
struct tmemory_device *get_tmemory_device(void);
bool tmemory_enabled(struct tmemory_device *tm);
void tmemory_free_encrypt_key(struct tmemory_device *tm,
				struct tmemory_encrypt_context *ec);
void tmemory_enable_switch(struct tmemory_device *tm);
void tmemory_sync_close(struct tmemory_device *tm, bool sync);
void tmemory_switch_set(struct tmemory_device *tm, int value, bool die);
void tmemory_close(void);
ssize_t tmemory_register_handle(struct kobject *kobj, struct kobj_attribute *attr,
						const char *buf, size_t len);

/* sysfs.c */
int __init tmemory_init_module_sysfs(void);
void tmemory_exit_module_sysfs(void);
int tmemory_init_device_sysfs(struct tmemory_device *tm);
void tmemory_exit_device_sysfs(struct tmemory_device *tm);

/* fault_inject.c */
#ifdef CONFIG_TMEMORY_FAULT_INJECTION
extern const char *tmemory_fault_name[FAULT_MAX];
void tmemory_build_fault_attr(struct tmemory_device *tm,
					unsigned int rate, unsigned int type);
#define tmemory_show_injection_info(tm, type)				\
	printk_ratelimited("%sTMEMORY: inject %s in %s of %pS\n",	\
		KERN_INFO,						\
		tmemory_fault_name[type],				\
		__func__, __builtin_return_address(0))
bool time_to_inject(struct tmemory_device *tm, int type);
#else
#define tmemory_build_fault_attr(tm, rate, type)	do { } while (0)
#define tmemory_show_injection_info(sbi, type)		do { } while (0)
static inline bool time_to_inject(struct tmemory_device *tm, int type)
{
	return false;
}
#endif

__printf(4, 5)
void tmemory_printk(struct tmemory_device *tm, int log_level, const char * func,const char *fmt, ...);


#define tmemory_err(tm, fmt, ...)					\
	tmemory_printk(tm, TMEMORY_LOG_LEVEL_ERROR, __func__, KERN_ERR fmt, ##__VA_ARGS__)
#define tmemory_warn(tm, fmt, ...)					\
	tmemory_printk(tm, TMEMORY_LOG_LEVEL_WARN, __func__, KERN_WARNING fmt, ##__VA_ARGS__)
#ifdef CONFIG_TMEMORY_DEBUG
#define tmemory_notice(tm, fmt, ...)					\
	tmemory_printk(tm, TMEMORY_LOG_LEVEL_NOTICE, __func__, KERN_NOTICE fmt, ##__VA_ARGS__)
#define tmemory_info(tm, fmt, ...)					\
	tmemory_printk(tm, TMEMORY_LOG_LEVEL_INFO, __func__, KERN_INFO fmt, ##__VA_ARGS__)
#define tmemory_debug(tm, fmt, ...)					\
	tmemory_printk(tm, TMEMORY_LOG_LEVEL_DEBUG, __func__, KERN_DEBUG fmt, ##__VA_ARGS__)
#define tmemory_temp(tm, fmt, ...)					\
	tmemory_printk(tm, TMEMORY_LOG_LEVEL_TEMP, __func__, KERN_DEBUG fmt, ##__VA_ARGS__)
#else
#define tmemory_notice(sbi, fmt, ...)	do {} while (0)
#define tmemory_info(sbi, fmt, ...)	do {} while (0)
#define tmemory_debug(sbi, fmt, ...)	do {} while (0)
#define tmemory_temp(sbi, fmt, ...)	do {} while (0)
#endif

#ifdef CONFIG_TMEMORY_DEBUG
#define TMEMORY_BUG_ON(cond, tm, fmt, ...) do {			\
	if (cond) {						\
		tmemory_err(tm, fmt"\n", ##__VA_ARGS__);	\
		panic("%s:%d panic\n", __func__, __LINE__);	\
	}							\
} while (0)
#else
#define TMEMORY_BUG_ON(cond, tm, fmt, ...)
#endif

// adapter for kernel version
void tmemory_queue_flag_set(unsigned int flag, struct request_queue *q);
struct page* tmemory_get_first_bio(struct bio *bio);

void tmemory_error(u32 type);
void tmemory_update_latency_stat(int op, unsigned long latency);
void tmemory_get_latency_stat(unsigned long *request, int op);
void tmemory_get_error_stat(unsigned long *err);
void tmemory_init_capacity(struct tmemory_device *tm);
void tmemory_purposememory(int value);
bool tmemory_availablemem_check(struct tmemory_device *tm);
#endif
