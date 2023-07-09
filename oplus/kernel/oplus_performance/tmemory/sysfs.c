#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/uidgid.h>
#include <linux/vmstat.h>

#include "tmemory.h"

static const char *err_str[TMEMORY_ERR_ITEMS] = {
	[TMEMORY_ERR_ENDREAD]		= "end_read",
	[TMEMORY_ERR_ENDWRITE]		= "end_write",
	[TMEMORY_ERR_STOPTRANS]		= "stop_tran",
	[TMEMORY_ERR_CHECKBOUND]	= "check_bound",
	[TMEMORY_ERR_NOTSUPP]		= "not_supp",
	[TMEMORY_ERR_DISCARD_ISSUE]	= "discard_issue",
	[TMEMORY_ERR_DISCARD_COMPLETE]	= "discard_complete",
	[TMEMORY_ERR_RADIX_PRELOAD]	= "radix_preload",
	[TMEMORY_ERR_TRANSACTION]	= "commit_tran",
	[TMEMORY_ERR_BLKFLUSH]		= "issue_flush",
	[TMEMORY_ERR_OP]		= "io_fail",
	[TMEMORY_ERR_ALLOC_SLAB]	= "alloc_slab",
	[TMEMORY_ERR_ALLOC_PAGE]	= "alloc_page",
};

/* sysfs */
#define kobject_attribute_write(name, show, store)		\
	static struct kobj_attribute				\
		tmemory_attr_##name = __ATTR(name, 0200, show, store)

struct kobject *tmemory_kobj;

struct tmemory_attr {
	struct attribute attr;
	ssize_t (*show)(struct tmemory_attr *, struct tmemory_device *, char *);
	ssize_t (*store)(struct tmemory_attr *, struct tmemory_device *,
			 const char *, size_t);
	int struct_type;
	int offset;
	int id;
};

enum {
	TMEMORY_DEVICE,
#ifdef CONFIG_TMEMORY_FAULT_INJECTION
	FAULT_INFO_RATE,
	FAULT_INFO_TYPE,
#endif
};

static unsigned char *__struct_ptr(struct tmemory_device *tm, int struct_type)
{
	if (struct_type == TMEMORY_DEVICE)
		return (unsigned char *)tm;
#ifdef CONFIG_TMEMORY_FAULT_INJECTION
	else if (struct_type == FAULT_INFO_RATE ||
					struct_type == FAULT_INFO_TYPE)
		return (unsigned char *)&tm->fault_info;
#endif
	return NULL;
}

static ssize_t tmemory_set_memory(struct tmemory_device *tm, const char *buf, size_t count)
{
	char memory_info[MEMORY_INFO_LEN] = {0};
	char *ptr = memory_info;
	char *next;
	const char st = '|';
	int level = 0;
	int max_level = 0;
	int value = 0;
	int clen;
	const char *path;

	if (count >= sizeof(memory_info))
		return -EINVAL;

	clen = buf[count - 1]  == '\n' ? count - 1 : count;
	path = strncpy(memory_info, buf, clen);
	if (!path) {
		tmemory_err(tm, "check your format!");
		return -EINVAL;
	}

	/* format:"level max_level value" */
	next = strchr(ptr, st);
	if (!next)
		return -EINVAL;
	*next++ = '\0';
	if (kstrtoint(ptr, 10, &level))
		return -EINVAL;
	printk("memory_stat level:%d", level);
	ptr = next;

	next = strchr(ptr, st);
	if (!next)
		return -EINVAL;
	*next++ = '\0';
	if (kstrtoint(ptr, 10, &max_level))
		return -EINVAL;
	printk("memory_stat max_level:%d", max_level);
	ptr = next;

	if (kstrtoint(ptr, 10, &value))
		return -EINVAL;
	printk("memory_stat value:%d", value);

	if (level == max_level) {
		tmemory_purposememory(value);
		tmemory_switch_set(tm, TMEMORY_SWITCH_FLAG_OSENSE, false);
	}

	return count;
}

static ssize_t tmemory_device_attr_store(struct tmemory_attr *a,
			struct tmemory_device *tm,
			const char *buf, size_t count)
{
	unsigned char *ptr = NULL;
	unsigned int *ui;
	int ret;
	unsigned long value;

	if (!tm)
		return -EINVAL;

	ptr = __struct_ptr(tm, a->struct_type);
	if (!ptr)
		return -EINVAL;

	if (!strcmp(a->attr.name, "memory_stat"))
		return tmemory_set_memory(tm, buf, count);

	ui = (unsigned int *)(ptr + a->offset);

	ret = kstrtoul(skip_spaces(buf), 0, &value);
	if (ret < 0)
		return ret;

	if (!strcmp(a->attr.name, "switch")) {
		tmemory_info(tm, "tmemoryd: switch write %lu", value);
		tmemory_switch_set(tm, value, false);

		return count;
	}

	if (!strcmp(a->attr.name, "panic_no_close")) {
		if (value > 0)
			tm->panic_no_close = true;
		else
			tm->panic_no_close = false;
		return count;
	}

	if (!strcmp(a->attr.name, "capacity")) {
		if (value < TMEMORY_MIN_CAPACITY)
			return -EINVAL;

		atomic_set(&tm->tmemory_capacity, value);

		atomic_set(&tm->high_watermark_pages,
				atomic_read(&tm->tmemory_capacity) *
					tm->high_watermark_ratio / 100);
		return count;
	}

	if (!strcmp(a->attr.name, "high_watermark_ratio")) {
		if (!value || value > TMEMORY_MAX_WATERMARK_RATIO)
			return -EINVAL;

		tm->high_watermark_ratio = value;

		atomic_set(&tm->high_watermark_pages,
			atomic_read(&tm->tmemory_capacity) *
				tm->high_watermark_ratio / 100);
		return count;
	}

	if (!strcmp(a->attr.name, "flush_busy_interval")) {
		if (value > 0 && value <= TMEMORY_MAX_FLUSH_INTERVAL) {
			tm->flush_busy_interval = value;
			tm->threshold[0] = tm->flush_busy_interval * HZ /
							TMEMORY_MS_PER_SEC;
		}
		return count;
	}

	if (!strcmp(a->attr.name, "flush_idle_interval")) {
		if (value > 0 && value <= TMEMORY_MAX_FLUSH_INTERVAL) {
			tm->flush_idle_interval = value;
			tm->threshold[1] = tm->flush_idle_interval * HZ /
							TMEMORY_MS_PER_SEC;
		}
		return count;
	}

	if (!strcmp(a->attr.name, "pages_per_flush")) {
		int i;

		if (value)
			return -EINVAL;

		down_write(&tm->commit_rwsem);
		for (i = 0; i < TMEMORY_STAT_SIZE; i++)
			tm->pages_per_flush[i] = 0;
		tm->max_flush_pages = 0;
		up_write(&tm->commit_rwsem);
		return count;
	}

	if (!strcmp(a->attr.name, "pages_per_trans")) {
		int i;

		if (value)
			return -EINVAL;

		down_write(&tm->commit_rwsem);
		for (i = 0; i < TMEMORY_STAT_SIZE; i++)
			tm->pages_per_trans[i] = 0;
		tm->max_trans_pages = 0;
		up_write(&tm->commit_rwsem);
		return count;
	}

	if (!strcmp(a->attr.name, "log_level")) {
		tm->log_level = value;
		return count;
	}

	if (!strcmp(a->attr.name, "switch_time")) {
		if (value < 0)
			return -EINVAL;
		atomic_set(&tm->switch_time, value);
		return count;
	}

	if (!strcmp(a->attr.name, "max_block_time")) {
		if (value < 0)
			return -EINVAL;
		atomic_set(&tm->max_block_time, value);
		return count;
	}

#ifdef CONFIG_TMEMORY_FAULT_INJECTION
	if (!strcmp(a->attr.name, "fault_rate")) {
		tm->fault_info.inject_rate = value;
		return count;
	}

	if (!strcmp(a->attr.name, "fault_type")) {
		tm->fault_info.inject_type = value;
		return count;
	}
#endif

	*ui = value;

	return count;
}

static ssize_t tmemory_device_attr_show(struct tmemory_attr *a,
			struct tmemory_device *tm, char *buf)
{
	unsigned char *ptr = NULL;
	unsigned int *ui;
	int len = 0;
	int tmp;
	unsigned long flags;

	if (!tm)
		return sprintf(buf, "tmemory device is not initialized\n");

	ptr = __struct_ptr(tm, a->struct_type);
	if (!ptr)
		return -EINVAL;

	if (!strcmp(a->attr.name, "switch")) {
		spin_lock_irqsave(&tm->state_lock, flags);
		tmp = tm->state;
		spin_unlock_irqrestore(&tm->state_lock, flags);
		return sprintf(buf, "%d\n", tmp);
	}

	if (!strcmp(a->attr.name, "panic_no_close")) {
		return sprintf(buf, "%d\n", tm->panic_no_close);
	}

	if (!strcmp(a->attr.name, "tmemory_remaining")) {
		spin_lock_irqsave(&tm->state_lock, flags);
		tmp = tm->tmemory_remaining;
		spin_unlock_irqrestore(&tm->state_lock, flags);
		return sprintf(buf, "%d\n", tmp);
	}

	if (!strcmp(a->attr.name, "capacity"))
		return sprintf(buf, "%u\n",
			atomic_read(&tm->tmemory_capacity));

	if (!strcmp(a->attr.name, "sys_dirty_pages"))
		return sprintf(buf, "%lu\n",
			global_node_page_state(NR_FILE_DIRTY));

	if (!strcmp(a->attr.name, "dirty_pages"))
		return sprintf(buf, "%u\n",
			atomic_read(&tm->dirty_pages));

	if (!strcmp(a->attr.name, "max_dirty_pages"))
		return sprintf(buf, "%u\n",
			atomic_read(&tm->max_dirty_pages));

	if (!strcmp(a->attr.name, "total_cached_pages"))
		return sprintf(buf, "%u\n",
			atomic_read(&tm->total_cached_pages));

	if (!strcmp(a->attr.name, "high_watermark_pages"))
		return sprintf(buf, "%u\n",
			atomic_read(&tm->high_watermark_pages));

	if (!strcmp(a->attr.name, "high_watermark_ratio"))
		return sprintf(buf, "%u\n", tm->high_watermark_ratio);

	if (!strcmp(a->attr.name, "log_level"))
		return sprintf(buf, "%d\n", tm->log_level);

	if (!strcmp(a->attr.name, "switch_time"))
		return sprintf(buf, "%d\n", atomic_read(&tm->switch_time));

	if (!strcmp(a->attr.name, "max_block_time"))
		return sprintf(buf, "%d\n", atomic_read(&tm->max_block_time));

	if (!strcmp(a->attr.name, "flush_busy_interval"))
		return sprintf(buf, "%u\n", tm->flush_busy_interval);

	if (!strcmp(a->attr.name, "flush_idle_interval"))
		return sprintf(buf, "%u\n", tm->flush_idle_interval);

	if (!strcmp(a->attr.name, "pages_per_flush")) {
		int i, size;

		for (i = 0; i < TMEMORY_STAT_SIZE - 1; i++) {
			size = 4 << i;

			if (size < 1024)
				len += sprintf(buf + len, "%dkb		%-10lu\n",
					size, tm->pages_per_flush[i]);
			else
				len += sprintf(buf + len, "%dmb		%-10lu\n",
					size / 1024, tm->pages_per_flush[i]);
		}
		len += sprintf(buf + len, "other		%-10lu\n",
						tm->pages_per_flush[i]);
		len += sprintf(buf + len, "max_flush_pages	%-10lu\n",
						tm->max_flush_pages);
		return len;
	}

	if (!strcmp(a->attr.name, "pages_per_trans")) {
		int i, size;

		for (i = 0; i < TMEMORY_STAT_SIZE - 1; i++) {
			size = 4 << i;

			if (size < 1024)
				len += sprintf(buf + len, "%dkb		%-10lu\n",
					size, tm->pages_per_trans[i]);
			else
				len += sprintf(buf + len, "%dmb		%-10lu\n",
					size / 1024, tm->pages_per_trans[i]);
		}
		len += sprintf(buf + len, "other		%-10lu\n",
						tm->pages_per_trans[i]);
		len += sprintf(buf + len, "max_trans_pages	%-10lu\n",
						tm->max_trans_pages);
		return len;
	}

	if (!strcmp(a->attr.name, "io_latency")) {
		int i;
		char * const time_str[]= {"1ms", "5ms", "10ms", "50ms", "100ms",
			"200ms", "500ms", "1s", "2s", "3s", "4s", "5s", "6s",
			"7s", "8s", "9s", "10s", "30s", "60s", "other"};

		len += sprintf(buf + len,
			"%-8s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s"
			"%-10s%-10s%-10s%-10s%-10s%-10s\n",
			"latency", "write", "read", "discard", "preflush",
			"postflush", "reg_page", "xcopy", "barrier", "submit",
			"trans", "blkwait", "switch", "unmap", "nomem",
			"bgtimeout", "sched");
		for (i = 0; i < TMEMORY_TIME_STAT; i++) {
			unsigned long request[TMEMORY_TIME_STAT] = {0};
			int j;

			tmemory_get_latency_stat(request, i);

			len += sprintf(buf + len, "%-8s", time_str[i]);
			for (j = 0; j < TMEMORY_MAX_OP; j++)
				len += sprintf(buf + len, "%-10lu", request[j]);
			len += sprintf(buf + len, "\n");
		}
		return len;
	}

	if (!strcmp(a->attr.name, "error_stat")) {
		unsigned long err[TMEMORY_ERR_ITEMS] = {0};
		int i;

		tmemory_get_error_stat(err);
		for (i = 0; i < TMEMORY_ERR_ITEMS; i++)
			len += sprintf(buf + len, "%-20s%-10lu\n", err_str[i], err[i]);
		return len;
	}

	if (!strcmp(a->attr.name, "inflight_write_bio"))
		return sprintf(buf, "%u\n", atomic_read(&tm->inflight_write_bio));

	if (!strcmp(a->attr.name, "inflight_write_page"))
		return sprintf(buf, "%u\n", atomic_read(&tm->inflight_write_page));

	if (!strcmp(a->attr.name, "inflight_read_bio"))
		return sprintf(buf, "%u\n", atomic_read(&tm->inflight_read_bio));

	if (!strcmp(a->attr.name, "inflight_read_page"))
		return sprintf(buf, "%u\n", atomic_read(&tm->inflight_read_page));

	if (!strcmp(a->attr.name, "inflight_discard_bio"))
		return sprintf(buf, "%u\n", atomic_read(&tm->inflight_discard_bio));

	if (!strcmp(a->attr.name, "inflight_discard_page"))
		return sprintf(buf, "%u\n", atomic_read(&tm->inflight_discard_page));

	if (!strcmp(a->attr.name, "memory_stat"))
		return sprintf(buf, "%u\n", atomic_read(&tm->memory_stat));

	if (!strcmp(a->attr.name, "memory_usage"))
		return sprintf(buf,
			"trans slab:	%-10u		%-10lu\n"
			"discard slab:	%-10u		%-10lu\n"
			"read slab:	%-10u		%-10lu\n"
			"encrypt slab:	%-10u		%-10lu\n"
			"crypto slab:	%-10u		%-10lu\n"
			"bypass slab:	%-10u		%-10lu\n"
			"bio count:	%-10u		%-10lu\n"
			"page count:	%-10u		%-10lu\n",
			atomic_read(&tm->trans_slab),
			atomic_read(&tm->trans_slab) * sizeof(struct tmemory_transaction),
			atomic_read(&tm->discard_slab),
			atomic_read(&tm->discard_slab) * sizeof(struct tmemory_discard_context),
			atomic_read(&tm->read_slab),
			atomic_read(&tm->read_slab) * sizeof(struct tmemory_read_context),
			atomic_read(&tm->encrypt_slab),
			atomic_read(&tm->encrypt_slab) * sizeof(struct tmemory_encrypt_context),
			atomic_read(&tm->crypto_slab),
			atomic_read(&tm->crypto_slab) * sizeof(struct tmemory_crypto_context),
			atomic_read(&tm->bypass_ctx_slab),
			atomic_read(&tm->bypass_ctx_slab) * sizeof(struct tmemory_bypass_context),
			atomic_read(&tm->bio_cnt),
			atomic_read(&tm->bio_cnt) * sizeof(struct bio),
			atomic_read(&tm->page_cnt),
			(unsigned long)atomic_read(&tm->page_cnt) << PAGE_SHIFT);

#ifdef CONFIG_TMEMORY_DEBUG
	if (!strcmp(a->attr.name, "crypt_parallels_max"))
		return sprintf(buf, "%u\n", atomic_read(&tm->crypt_parallels_max));
#endif /* CONFIG_TMEMORY_DEBUG */

	ui = (unsigned int *)(ptr + a->offset);

	return sprintf(buf, "%u\n", *ui);
}

#define TMEMORY_ATTR_OFFSET(_struct_type, _name, _mode, _show, _store, _offset) \
static struct tmemory_attr tmemory_attr_##_name = {			\
	.attr = {.name = __stringify(_name), .mode = _mode },	\
	.show	= _show,					\
	.store	= _store,					\
	.struct_type = _struct_type,				\
	.offset = _offset					\
}

#define TMEMORY_RW_ATTR(struct_type, struct_name, name, elname)	\
	TMEMORY_ATTR_OFFSET(struct_type, name, 0664,		\
		tmemory_device_attr_show, tmemory_device_attr_store,	\
		offsetof(struct struct_name, elname))

#define TMEMORY_RO_ATTR(struct_type, struct_name, name, elname)	\
	TMEMORY_ATTR_OFFSET(struct_type, name, 0444,		\
		tmemory_device_attr_show, NULL,			\
		offsetof(struct struct_name, elname))

/* switch */
TMEMORY_RW_ATTR(TMEMORY_DEVICE, tmemory_device, switch, state);

/* panic no close */
TMEMORY_RW_ATTR(TMEMORY_DEVICE, tmemory_device, panic_no_close, panic_no_close);

/* cache status */
TMEMORY_RW_ATTR(TMEMORY_DEVICE, tmemory_device, capacity, tmemory_capacity);
TMEMORY_RO_ATTR(TMEMORY_DEVICE, tmemory_device, sys_dirty_pages, fake);
TMEMORY_RO_ATTR(TMEMORY_DEVICE, tmemory_device, dirty_pages, dirty_pages);
TMEMORY_RO_ATTR(TMEMORY_DEVICE, tmemory_device, max_dirty_pages, max_dirty_pages);
TMEMORY_RO_ATTR(TMEMORY_DEVICE, tmemory_device, total_cached_pages, total_cached_pages);
TMEMORY_RW_ATTR(TMEMORY_DEVICE, tmemory_device, high_watermark_ratio, high_watermark_ratio);
TMEMORY_RO_ATTR(TMEMORY_DEVICE, tmemory_device, high_watermark_pages, high_watermark_pages);

/* flush thread control */
TMEMORY_RW_ATTR(TMEMORY_DEVICE, tmemory_device, flush_busy_interval, flush_busy_interval);
TMEMORY_RW_ATTR(TMEMORY_DEVICE, tmemory_device, flush_idle_interval, flush_idle_interval);

/* for io stat */
TMEMORY_RW_ATTR(TMEMORY_DEVICE, tmemory_device, pages_per_flush, pages_per_flush);
TMEMORY_RW_ATTR(TMEMORY_DEVICE, tmemory_device, pages_per_trans, pages_per_trans);
TMEMORY_RO_ATTR(TMEMORY_DEVICE, tmemory_device, io_latency, fake);
TMEMORY_RO_ATTR(TMEMORY_DEVICE, tmemory_device, error_stat, fake);

/* for debug */
TMEMORY_RW_ATTR(TMEMORY_DEVICE, tmemory_device, log_level, log_level);
TMEMORY_RW_ATTR(TMEMORY_DEVICE, tmemory_device, switch_time, switch_time);
TMEMORY_RW_ATTR(TMEMORY_DEVICE, tmemory_device, max_block_time, max_block_time);

/* fault injection */
#ifdef CONFIG_TMEMORY_FAULT_INJECTION
TMEMORY_RW_ATTR(FAULT_INFO_RATE, tmemory_fault_info, inject_rate, inject_rate);
TMEMORY_RW_ATTR(FAULT_INFO_TYPE, tmemory_fault_info, inject_type, inject_type);
#endif

/* inflight data stat */
TMEMORY_RO_ATTR(TMEMORY_DEVICE, tmemory_device, inflight_write_bio, inflight_write_bio);
TMEMORY_RO_ATTR(TMEMORY_DEVICE, tmemory_device, inflight_write_page, inflight_write_page);
TMEMORY_RO_ATTR(TMEMORY_DEVICE, tmemory_device, inflight_read_bio, inflight_read_bio);
TMEMORY_RO_ATTR(TMEMORY_DEVICE, tmemory_device, inflight_read_page, inflight_read_page);
TMEMORY_RO_ATTR(TMEMORY_DEVICE, tmemory_device, inflight_discard_bio, inflight_discard_bio);
TMEMORY_RO_ATTR(TMEMORY_DEVICE, tmemory_device, inflight_discard_page, inflight_discard_page);

/* memory stat */
TMEMORY_RW_ATTR(TMEMORY_DEVICE, tmemory_device, memory_stat, memory_stat);
TMEMORY_RO_ATTR(TMEMORY_DEVICE, tmemory_device, memory_usage, fake);

#ifdef CONFIG_TMEMORY_DEBUG
TMEMORY_RO_ATTR(TMEMORY_DEVICE, tmemory_device, crypt_parallels_max, fake);
#endif /* CONFIG_TMEMORY_DEBUG */

/* config */
TMEMORY_RW_ATTR(TMEMORY_DEVICE, tmemory_device, config, config);

TMEMORY_RO_ATTR(TMEMORY_DEVICE, tmemory_device, tmemory_remaining, fake);

#define ATTR_LIST(name) (&tmemory_attr_##name.attr)
static struct attribute *tmemory_device_attrs[] = {
	ATTR_LIST(switch),
	ATTR_LIST(panic_no_close),
	ATTR_LIST(tmemory_remaining),

	/* cache status */
	ATTR_LIST(capacity),
	ATTR_LIST(sys_dirty_pages),
	ATTR_LIST(dirty_pages),
	ATTR_LIST(max_dirty_pages),
	ATTR_LIST(total_cached_pages),
	ATTR_LIST(high_watermark_ratio),
	ATTR_LIST(high_watermark_pages),

	/* flush thread control */
	ATTR_LIST(flush_busy_interval),
	ATTR_LIST(flush_idle_interval),

	/* for io stat */
	ATTR_LIST(pages_per_flush),
	ATTR_LIST(pages_per_trans),
	ATTR_LIST(io_latency),
	ATTR_LIST(error_stat),

	/* for debug */
	ATTR_LIST(log_level),
	ATTR_LIST(switch_time),
	ATTR_LIST(max_block_time),
#ifdef CONFIG_TMEMORY_FAULT_INJECTION
	ATTR_LIST(inject_rate),
	ATTR_LIST(inject_type),
#endif

	/* inflight data stat */
	ATTR_LIST(inflight_write_bio),
	ATTR_LIST(inflight_write_page),
	ATTR_LIST(inflight_read_bio),
	ATTR_LIST(inflight_read_page),
	ATTR_LIST(inflight_discard_bio),
	ATTR_LIST(inflight_discard_page),

	/* osense interface */
	ATTR_LIST(memory_stat),

	/* memory usage: slab */
	ATTR_LIST(memory_usage),
#ifdef CONFIG_TMEMORY_DEBUG
	ATTR_LIST(crypt_parallels_max),
#endif /* CONFIG_TMEMORY_DEBUG */

	/* config */
	ATTR_LIST(config),

	NULL,
};
#if LINUX_KERNEL_515 || LINUX_KERNEL_510
ATTRIBUTE_GROUPS(tmemory_device);
#endif

kobject_attribute_write(register, NULL, tmemory_register_handle);

static const struct attribute *tmemory_module_attrs[] = {
	ATTR_LIST(register),
	NULL,
};

static ssize_t tmemory_attr_show(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	struct tmemory_device *sbi = container_of(kobj, struct tmemory_device,
								s_kobj);
	struct tmemory_attr *a = container_of(attr, struct tmemory_attr, attr);

	return a->show ? a->show(a, sbi, buf) : 0;
}

static ssize_t tmemory_attr_store(struct kobject *kobj, struct attribute *attr,
						const char *buf, size_t len)
{
	struct tmemory_device *sbi = container_of(kobj, struct tmemory_device,
									s_kobj);
	struct tmemory_attr *a = container_of(attr, struct tmemory_attr, attr);

	return a->store ? a->store(a, sbi, buf, len) : 0;
}

static void tmemory_attr_release(struct kobject *kobj)
{
	struct tmemory_device *tm = container_of(kobj, struct tmemory_device,
								s_kobj);
	complete(&tm->s_kobj_unregister);
}

/* FIXME: 4.14 kernel doesn't support change uid/gid of sysfs entry */
#if LINUX_KERNEL_515 || LINUX_KERNEL_510 || LINUX_KERNEL_419 || LINUX_KERNEL_504
static void tmemory_get_ownership(struct kobject *kobj, kuid_t *uid, kgid_t *gid)
{
	*gid = KGIDT_INIT(TMEMORY_ANDROID_SYSTEM);
}
#endif

static const struct sysfs_ops tmemory_attr_ops = {
	.show	= tmemory_attr_show,
	.store	= tmemory_attr_store,
};

static struct kobj_type tmemory_device_ktype = {
#if LINUX_KERNEL_515 || LINUX_KERNEL_510
	.default_groups = tmemory_device_groups,
	.get_ownership	= tmemory_get_ownership,
#elif LINUX_KERNEL_414
	.default_attrs	= tmemory_device_attrs,
#elif LINUX_KERNEL_419 || LINUX_KERNEL_504
	.default_attrs	= tmemory_device_attrs,
	.get_ownership	= tmemory_get_ownership,
#endif
	.sysfs_ops	= &tmemory_attr_ops,
	.release	= tmemory_attr_release,
};

int __init tmemory_init_module_sysfs(void)
{
	int err;

	tmemory_kobj = kobject_create_and_add("tmemory", NULL);
	if (!tmemory_kobj)
		return -ENOMEM;

	err = sysfs_create_files(tmemory_kobj, tmemory_module_attrs);
	if (err)
		kobject_put(tmemory_kobj);

	return err;
}

void tmemory_exit_module_sysfs(void)
{
	sysfs_remove_files(tmemory_kobj, tmemory_module_attrs);
	kobject_del(tmemory_kobj);
	kobject_put(tmemory_kobj);
}

int tmemory_init_device_sysfs(struct tmemory_device *tm)
{
	char name[TMEMORY_NAME_LEN];
	int err;

	snprintf(name, TMEMORY_NAME_LEN, "%pg", tm->rbd);

	init_completion(&tm->s_kobj_unregister);
	err = kobject_init_and_add(&tm->s_kobj, &tmemory_device_ktype,
			tmemory_kobj, "%s", name);
	if (err)
		kobject_put(&tm->s_kobj);

	return err;
}

void tmemory_exit_device_sysfs(struct tmemory_device *tm)
{
	kobject_del(&tm->s_kobj);
	kobject_put(&tm->s_kobj);
	wait_for_completion(&tm->s_kobj_unregister);
}
