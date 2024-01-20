// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020-2022 Oplus. All rights reserved.
 */
#define pr_fmt(fmt) "proactive_compact: " fmt

#include <linux/module.h>
#include <linux/types.h>
#include <linux/swap.h>
#include <linux/mm.h>
#include <linux/math64.h>
#include <linux/proc_fs.h>

#define PARA_BUF_LEN 128

/* Page order with-respect-to which proactive compaction calculates external fragmentation. */
static unsigned int g_compaction_hpage_order = 4;
static unsigned int g_compaction_proactiveness = 20;
static struct proc_dir_entry *fragmentation_index_entry;

struct contig_page_info {
	unsigned long free_pages;
	unsigned long free_blocks_total;
	unsigned long free_blocks_suitable;
};

static inline bool debug_get_val(char *buf, char *token, unsigned long *val)
{
	int ret = -EINVAL;
	char *str = strstr(buf, token);

	if (!str)
		return ret;

	ret = kstrtoul(str + strlen(token), 0, val);
	if (ret)
		return -EINVAL;

	if (*val > 200) {
		pr_err("%lu is invalid\n", *val);
		return -EINVAL;
	}

	return 0;
}

static void fill_contig_page_info(struct zone *zone,
				unsigned int suitable_order,
				struct contig_page_info *info)
{
	unsigned int order;
#if defined(OPLUS_FEATURE_MULTI_FREEAREA) && defined(CONFIG_PHYSICAL_ANTI_FRAGMENTATION)
	int flc;
#endif

	info->free_pages = 0;
	info->free_blocks_total = 0;
	info->free_blocks_suitable = 0;

#if defined(OPLUS_FEATURE_MULTI_FREEAREA) && defined(CONFIG_PHYSICAL_ANTI_FRAGMENTATION)
	for (flc = 0; flc < FREE_AREA_COUNTS; flc++) {
#endif
	for (order = 0; order < MAX_ORDER; order++) {
		unsigned long blocks;

		/* Count number of free blocks */
#if defined(OPLUS_FEATURE_MULTI_FREEAREA) && defined(CONFIG_PHYSICAL_ANTI_FRAGMENTATION)
		blocks = zone->free_area[flc][order].nr_free;
#else
		blocks = zone->free_area[order].nr_free;
#endif
		info->free_blocks_total += blocks;

		/* Count free base pages */
		info->free_pages += blocks << order;

		/* Count the suitable free blocks */
		if (order >= suitable_order)
			info->free_blocks_suitable += blocks <<
						(order - suitable_order);
	}
#if defined(OPLUS_FEATURE_MULTI_FREEAREA) && defined(CONFIG_PHYSICAL_ANTI_FRAGMENTATION)
	}
#endif
}

static unsigned int external_fragmentation_for_order(struct zone *zone, unsigned int order)
{
	struct contig_page_info info;

	fill_contig_page_info(zone, order, &info);
	if (info.free_pages == 0)
		return 0;

	return div_u64((info.free_pages - (info.free_blocks_suitable << order)) * 100, info.free_pages);
}

/*
 * A zone's fragmentation score is the external fragmentation wrt to the
 * COMPACTION_HPAGE_ORDER. It returns a value in the range [0, 100].
 */
static unsigned int fragmentation_score_zone(struct zone *zone)
{
	return external_fragmentation_for_order(zone, g_compaction_hpage_order);
}

static unsigned int fragmentation_score_zone_weighted(struct zone *zone)
{
	unsigned long score;

	score = zone->present_pages * fragmentation_score_zone(zone);
	return div64_ul(score, zone->zone_pgdat->node_present_pages + 1);
}

static unsigned int fragmentation_score_node(pg_data_t *pgdat)
{
	unsigned int score = 0;
	int zoneid;

	for (zoneid = 0; zoneid < MAX_NR_ZONES; zoneid++) {
		struct zone *zone;

		zone = &pgdat->node_zones[zoneid];
		score += fragmentation_score_zone_weighted(zone);
	}

	return score;
}

static unsigned int fragmentation_score_wmark(pg_data_t *pgdat)
{
	unsigned int wmark_low;

	/*
	 * Cap the low watermark to avoid excessive compaction
	 * activity in case a user sets the proactiveness tunable
	 * close to 100 (maximum).
	 */
	wmark_low = max(100U - g_compaction_proactiveness, 5U);
	return min(wmark_low + 10, 100U);
}

static bool should_proactive_compact_node(pg_data_t *pgdat)
{
	int wmark_high;

	wmark_high = fragmentation_score_wmark(pgdat);
	return fragmentation_score_node(pgdat) > wmark_high;
}

static ssize_t fragmentation_index_write(struct file *file,
		const char __user *buff, size_t len, loff_t *ppos)
{
	char kbuf[PARA_BUF_LEN] = {'\0'};
	char *str;
	unsigned long val;

	if ((len > PARA_BUF_LEN - 1) || (len == 0)) {
		pr_err("len %d is invalid\n", len);
		return -EINVAL;
	}

	if (copy_from_user(&kbuf, buff, len))
		return -EFAULT;
	kbuf[len] = '\0';

	str = strstrip(kbuf);
	if (!str) {
		pr_err("buff %s is invalid\n", kbuf);
		return -EINVAL;
	}

	if (!debug_get_val(str, "compaction_hpage_order=", &val)) {
		g_compaction_hpage_order = val;
		return len;
	}

	if (!debug_get_val(str, "compaction_proactiveness=", &val)) {
		g_compaction_proactiveness = val;
		return len;
	}

	return -EINVAL;
}

static ssize_t fragmentation_index_read(struct file *file,
		char __user *buffer, size_t count, loff_t *off)
{
	char kbuf[PARA_BUF_LEN] = {'\0'};
	int len, is_fragmented = 0;
	pg_data_t *pgdat;

	pgdat = NODE_DATA(0);
	if (should_proactive_compact_node(pgdat))
		is_fragmented = 1;

	len = scnprintf(kbuf, PARA_BUF_LEN, "%d %d %d\n", is_fragmented,
			g_compaction_hpage_order, g_compaction_proactiveness);

	if (len == PARA_BUF_LEN)
		kbuf[len - 1] = '\0';

	if (len > *off)
		len -= *off;
	else
		len = 0;

	if (copy_to_user(buffer, kbuf + *off, (len < count ? len : count)))
		return -EFAULT;

	*off += (len < count ? len : count);
	return (len < count ? len : count);
}

static const struct file_operations proc_fragmentation_index_ops = {
	.write	= fragmentation_index_write,
	.read	= fragmentation_index_read,
};

static int __init create_fragmentation_index_proc(void)
{
	struct proc_dir_entry *root_dir_entry = proc_mkdir("oplus_mem", NULL);

	fragmentation_index_entry = proc_create((root_dir_entry ?
				"fragmentation_index" : "oplus_mem/fragmentation_index"),
				0666, root_dir_entry, &proc_fragmentation_index_ops);

	if (fragmentation_index_entry) {
		pr_info("Register fragmentation_index interface passed!\n");
		return 0;
	}

	pr_err("Register fragmentation_index interface failed!\n");
	return -ENOMEM;
}

static void __exit destroy_fragmentation_index_proc(void)
{
	proc_remove(fragmentation_index_entry);
	fragmentation_index_entry = NULL;
}

static int __init proactive_compact_init(void)
{
	int ret = 0;

	ret = create_fragmentation_index_proc();
	if (ret)
		return ret;

	pr_info("proactive_compact init success!\n");
	return 0;
}

static void __exit proactive_compact_exit(void)
{
	destroy_fragmentation_index_proc();

	pr_info("proactive_compact exit success!\n");
	return;
}

module_init(proactive_compact_init);
module_exit(proactive_compact_exit);

module_param_named(compaction_hpage_order, g_compaction_hpage_order, uint, S_IRUGO | S_IWUSR);
module_param_named(compaction_proactiveness, g_compaction_proactiveness, uint, S_IRUGO | S_IWUSR);

MODULE_LICENSE("GPL v2");
