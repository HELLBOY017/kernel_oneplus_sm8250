#include "tmemory.h"

DEFINE_PER_CPU(struct tmemory_stat, tmemory_stats) = {{0}};
EXPORT_PER_CPU_SYMBOL(tmemory_stats);

static int g_purpose_mem;

void tmemory_printk(struct tmemory_device *tm, int log_level, const char * func, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;
	int level;

	if (!tm || log_level > tm->log_level)
		return;

	va_start(args, fmt);

	level = printk_get_level(fmt);
	vaf.fmt = printk_skip_level(fmt);
	vaf.va = &args;
	printk("%c%ctmemory (%s): %s %pV\n",
	       KERN_SOH_ASCII, level, tm->name, func, &vaf);

	va_end(args);
}

#ifdef CONFIG_TMEMORY_MIGRATION
#include <linux/migrate.h>
int tmemory_migrate_page_move_mapping(struct address_space *mapping,
				struct page *newpage, struct page *page)
{
	struct tmemory_device *tm = get_tmemory_device();
	struct tmemory_transaction *trans;
	struct page *old;
	void **slot;

	if (!page->private) {
		tmemory_info(tm, "tmemory_migrate_page() page private is NULL");
		return -EINVAL;
	}

#ifdef CONFIG_TMEMORY_CRYPTO
	if (!tmemory_page_crypt(page)->trans) {
		tmemory_info(tm, "tmemory_migrate_page(), not belong to any transaction");
		return -EINVAL;
	}
#endif

	if (unlikely(page_count(page) > 3)) {
		tmemory_info(tm, "tmemory_migrate_page(), page count(%u) > 3", page_count(page));
		return -EINVAL;
	}

	set_page_private(newpage, page_private(page));
	set_page_private(page, 0);
	newpage->index = page->index;
	newpage->mapping = page->mapping;
	get_page(newpage);
	atomic_inc(&tm->page_cnt);

	if (PageSwapBacked(page)) {
		__SetPageSwapBacked(newpage);
		if (PageSwapCache(page))
			SetPageSwapCache(newpage);
	} else {
		VM_BUG_ON_PAGE(PageSwapCache(page), page);
	}

	if (PageDirty(page)) {
		ClearPageDirty(page);
		SetPageDirty(newpage);
	}

	slot = radix_tree_lookup_slot(&tm->global_space, page->index);
	if (slot) {
		old = radix_tree_deref_slot_protected(slot, &tm->update_lock);
		if (old == page) {
			radix_tree_replace_slot(&tm->global_space,
							slot, newpage);
			put_page(old);
			atomic_dec(&tm->page_cnt);
			get_page(newpage);
			atomic_inc(&tm->page_cnt);
		}
	}

	trans = tmemory_page_crypt((newpage))->trans;

	slot = radix_tree_lookup_slot(&trans->space, page->index);
	if (slot) {
		old = radix_tree_deref_slot_protected(slot, &tm->update_lock);
		if (old == page) {
			radix_tree_replace_slot(&trans->space, slot, newpage);
			put_page(old);
			atomic_dec(&tm->page_cnt);
			get_page(newpage);
			atomic_inc(&tm->page_cnt);
		}
	}

	/* decrease page refcnt which increased by alloc_page */
	put_page(page);
	atomic_dec(&tm->page_cnt);

	return MIGRATEPAGE_SUCCESS;
}

int tmemory_migrate_page(struct address_space *mapping,
		struct page *newpage, struct page *page, enum migrate_mode mode)
{
	struct tmemory_device *tm = get_tmemory_device();
	unsigned long flags;
	int rc;

	TMEMORY_BUG_ON(PageWriteback(page), tm, "");

	down_write(&tm->migrate_lock);
	spin_lock_irqsave(&tm->update_lock, flags);

	if (time_to_inject(tm, FAULT_PANIC_MIGRATE_LOCK)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_MIGRATE_LOCK);
		TMEMORY_BUG_ON(1, tm, "");
	}
	if (time_to_inject(tm, FAULT_PANIC_UPDATE_LOCK)) {
		tmemory_show_injection_info(tm, FAULT_PANIC_UPDATE_LOCK);
		TMEMORY_BUG_ON(1, tm, "");
	}

	rc = tmemory_migrate_page_move_mapping(mapping, newpage, page);
	if (rc == MIGRATEPAGE_SUCCESS)
		migrate_page_copy(newpage, page);

	spin_unlock_irqrestore(&tm->update_lock, flags);
	up_write(&tm->migrate_lock);

	return rc;
}

bool tmemory_isolate_page(struct page *page, isolate_mode_t mode)
{
	return true;
}

void tmemory_putback_page(struct page *page)
{
}

struct address_space_operations tmemory_aops = {
	.migratepage = tmemory_migrate_page,
	.isolate_page = tmemory_isolate_page,
	.putback_page = tmemory_putback_page,
};
#endif

bool is_idle(struct tmemory_device *tm)
{
#if LINUX_KERNEL_515 || LINUX_KERNEL_510 || LINUX_KERNEL_504
	return !blk_mq_queue_inflight(tm->queue);
#elif LINUX_KERNEL_414 || LINUX_KERNEL_419
	return queue_in_flight(tm->queue);
#else
#error "Kernel version not supported"
#endif
}

void calc_limits(struct tmemory_device *tm)
{
	if (is_idle(tm))
		tm->io_depths = TMEMORY_FG_DEPTH;
	else
		tm->io_depths = TMEMORY_MAX_DEPTH;
}

void tmemory_update_limits(struct tmemory_device *tm)
{
	calc_limits(tm);
	if (waitqueue_active(&tm->wait))
		wake_up_all(&tm->wait);
}

void re_arm_timer(struct tmemory_device *tm)
{
	unsigned long expire = jiffies + msecs_to_jiffies(10);
	mod_timer(&tm->window_timer, expire);
}

void __tmemory_wait(struct tmemory_device *tm)
{
	DEFINE_WAIT(wait);

	calc_limits(tm);

	if (atomic_read(&tm->inflight_write_bio) < tm->io_depths)
		return;

	do {
		prepare_to_wait_exclusive(&tm->wait, &wait, TASK_UNINTERRUPTIBLE);
		calc_limits(tm);
		if (atomic_read(&tm->inflight_write_bio) < tm->io_depths)
			break;
		io_schedule();
	} while (1);

	finish_wait(&tm->wait, &wait);
}

void tmemory_wait(struct tmemory_device *tm)
{
	unsigned long start_jiffies = jiffies;

	__tmemory_wait(tm);

	tmemory_update_latency_stat(TMEMORY_SUBMIT_BIO_WAIT_OP,
						jiffies - start_jiffies);
	if (!timer_pending(&tm->window_timer))
		re_arm_timer(tm);
}

static void tmemory_timer_fn(struct timer_list *timer)
{
	struct tmemory_device *tm =
		container_of(timer, struct tmemory_device, window_timer);

	tmemory_update_limits(tm);
	re_arm_timer(tm);
}

void tmemory_iocontrol_init(struct tmemory_device *tm)
{
	init_waitqueue_head(&tm->wait);
	timer_setup(&tm->window_timer, tmemory_timer_fn, 0);
	tmemory_update_limits(tm);
}

void tmemory_iocontrol_exit(struct tmemory_device *tm)
{
	if (tm)
		del_timer_sync(&tm->window_timer);
}

void tmemory_queue_flag_set(unsigned int flag, struct request_queue *q)
{

#if LINUX_KERNEL_515 || LINUX_KERNEL_510 || LINUX_KERNEL_419 || LINUX_KERNEL_504
	return blk_queue_flag_set(flag, q);
#elif LINUX_KERNEL_414
	return queue_flag_set(flag, q);
#endif
}

struct page* tmemory_get_first_bio(struct bio *bio)
{
#if LINUX_KERNEL_515 || LINUX_KERNEL_510 || LINUX_KERNEL_419 || LINUX_KERNEL_504
	return bio_first_page_all(bio);
#elif LINUX_KERNEL_414
	return bio->bi_io_vec[0].bv_page;
#endif
}

void tmemory_error(u32 type)
{
	if (type >= TMEMORY_ERR_ITEMS)
		return;
	this_cpu_inc(tmemory_stats.err[type]);
}

void tmemory_update_latency_stat(int op, unsigned long delta)
{
	int i = 0;
	delta = delta*1000/HZ;

	if (delta <= 1) {
	} else if (++i && delta <= 5) {
	} else if (++i && delta <= 10) {
	} else if (++i && delta <= 50) {
	} else if (++i && delta <= 100) {
	} else if (++i && delta <= 200) {
	} else if (++i && delta <= 500) {
	} else if (++i && delta <= 1000) {
	} else if (++i && delta <= 2000) {
	} else if (++i && delta <= 3000) {
	} else if (++i && delta <= 4000) {
	} else if (++i && delta <= 5000) {
	} else if (++i && delta <= 6000) {
	} else if (++i && delta <= 7000) {
	} else if (++i && delta <= 8000) {
	} else if (++i && delta <= 9000) {
	} else if (++i && delta <= 10000) {
	} else if (++i && delta <= 30000) {
	} else if (++i && delta <= 60000) {
	} else {
		++i;
	}

	if (delta > 1000)
		pr_debug("tmemory: op %d latency %lu\n", op, delta);

	this_cpu_inc(tmemory_stats.request[op][i]);
}

void tmemory_get_error_stat(unsigned long *err)
{
	int cpu;
	int i;

	for_each_online_cpu(cpu) {
		struct tmemory_stat* this = &per_cpu(tmemory_stats, cpu);

		for (i = 0; i < TMEMORY_ERR_ITEMS; i++)
			err[i] += this->err[i];
	}
}

void tmemory_get_latency_stat(unsigned long *request, int time_granularity)
{
	int cpu;
	int i;

	for_each_online_cpu(cpu) {
		struct tmemory_stat* this = &per_cpu(tmemory_stats, cpu);

		for (i = 0; i < TMEMORY_MAX_OP; i++)
			request[i] += this->request[i][time_granularity];
	}
}

void tmemory_init_capacity(struct tmemory_device *tm)
{
	struct sysinfo si;
	unsigned long pages_2g = 2 << 18; /* 2GB */
	unsigned long pages_4g = 4 << 18; /* 4GB */
	int capacity = TMEMORY_DEFAULT_CAPACITY;

	si_meminfo(&si);
	si.totalram = round_up(si.totalram, pages_2g);
	tmemory_info(tm, "tmemory: totalram %lu pages (%lu MB)\n", si.totalram, si.totalram / 1024 * 4);

	if (si.totalram <= pages_4g) {
		atomic_set(&tm->tmemory_capacity, capacity);
		return;
	}

	si.totalram -= pages_4g;
	while (si.totalram >= pages_2g && capacity < TMEMORY_MAX_CAPACITY) {
		capacity += TMEMORY_DELTA_CAPACITY;
		si.totalram -= pages_2g;
	}

	atomic_set(&tm->tmemory_capacity, capacity);
	tmemory_info(tm, "tmemory: capacity %d (%d MB)\n", capacity, capacity / 1024 * 4);
}

void tmemory_purposememory(int value)
{
	g_purpose_mem = value;
}

bool tmemory_availablemem_check(struct tmemory_device *tm)
{
	unsigned long threshold = TMEMORY_PAGE_TO_MBYTES(atomic_read(&tm->tmemory_capacity)) << 2;
	unsigned long available = TMEMORY_PAGE_TO_MBYTES(si_mem_available());

	if (threshold < TMEMORY_OSENSE_THRESHOLD)
		threshold = TMEMORY_OSENSE_THRESHOLD;

	tmemory_info(tm, "available:%lu threshold:%ld purposemem:%d", available,
				threshold, g_purpose_mem);

	if (available > (threshold + g_purpose_mem))
		return true;

	return false;
}

