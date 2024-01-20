// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020-2022 Oplus. All rights reserved.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/sched/clock.h>
#include <linux/cgroup.h>
#include <linux/ftrace.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/rwsem.h>
#include <linux/mutex.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include <trace/hooks/dtask.h>
#include <trace/hooks/futex.h>
#endif
#include <trace/events/sched.h>
#include <linux/sort.h>

#include <linux/sched_assist/sched_assist_common.h>
#include "kern_lock_stat.h"
#include "locking_main.h"



/*********************************commom***********************************/
#define LIMIT_GRP_TYPES 		(GRP_OT + 1)
#define TO_LIMIT_GRP_IDX(x)		(x <= GRP_OT ? x : GRP_OT)

static char *group_str[GRP_TYPES] = {"GRP_UX", "GRP_RT", "GRP_OT",
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
					"GRP_BG", "GRP_FG", "GRP_TA"
#endif
};

static char *lock_str[LOCK_TYPES] = {
#ifdef INCLUDE_UNUSE
					"spinlock",
#endif
					"mutex", "rwsem_read", "rwsem_write",
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
					"osq_mutex", "osq_rwsem_read", "osq_rwsem_write",
#endif
					"futex_art",
#ifdef INCLUDE_UNUSE
					"futex_juc"
#endif
};

static char *func_str[LOCK_TYPES] = {
#ifdef INCLUDE_UNUSE
					" ",
#endif
					"__mutex_lock_slowpath", "rwsem_down_read_slowpath", "rwsem_down_write_slowpath",
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
					"__mutex_lock_slowpath", "rwsem_down_read_slowpath", "rwsem_down_write_slowpath",
#endif
					"futex_wait",
#ifdef INCLUDE_UNUSE
					" "
#endif
};

static int get_task_grp_idx(void)
{
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
	struct cgroup_subsys_state *css;
#endif
	int ret = GRP_OT;

	if (current->prio < MAX_RT_PRIO)
		return GRP_RT;

	if (current->ux_state & SCHED_ASSIST_UX_MASK)
		return GRP_UX;

#ifdef CONFIG_OPLUS_INTERNAL_VERSION
	/* More group info can be found in internal version */
	rcu_read_lock();
	css = task_css(current, cpu_cgrp_id);

	if (!css) {
		ret = GRP_OT;
	} else if (css->id == CGROUP_TOP_APP) {
		ret = GRP_TA;
	} else if (css->id == CGROUP_FOREGROUND) {
		ret = GRP_FG;
	} else if (css->id == CGROUP_BACKGROUND) {
		ret = GRP_BG;
	} else {
		ret = GRP_OT;
	}

	rcu_read_unlock();
#endif

	return ret;
}

#ifdef CONFIG_OPLUS_INTERNAL_VERSION
static void print_time(u64 t, char *pbuf, int len)
{
	if (!pbuf)
		return;

	if (t >= 1000000000) {
		snprintf(pbuf, len, "%lu.%lu%s", t / 1000000000, t % 1000000000 / 10000000, "s");
	} else if (t >= 1000000) {
		snprintf(pbuf, len, "%lu.%lu%s", t / 1000000, t % 1000000 / 10000, "ms");
	} else if (t >= 1000) {
		snprintf(pbuf, len, "%lu.%lu%s", t / 1000, t % 1000 / 10, "us");
	} else {
		snprintf(pbuf, len, "%lu%s", t, "ns");
	}
}
#endif

static  u64 lockstat_clock(void)
{
	return local_clock();
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
unsigned int stack_trace_save(unsigned long *store, unsigned int size,
			     unsigned int skipnr)
{
	struct stack_trace trace = {
		.entries	= store,
		.max_entries	= size,
		.skip		= skipnr + 1,
	};

	save_stack_trace(&trace);
	return trace.nr_entries;
}
#endif
/*
 * Caller must make sure sizeof(addr_buf) >= (entries * sizeof(long))
 */
static __always_inline int fetch_trace_addr(int type, int entries, unsigned long *addr_buf, int skipnr)
{
	if (!addr_buf)
		return -1;

	switch (type) {
#ifdef INCLUDE_UNUSE
	case SPINLOCK:
#endif
	case MUTEX:
		return stack_trace_save(addr_buf, entries, skipnr + 3);
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
	case OSQ_MUTEX:
	case OSQ_RWSEM_READ:
	case OSQ_RWSEM_WRITE:
		return stack_trace_save(addr_buf, entries, skipnr + 2);
#endif

	case RWSEM_READ:
	case RWSEM_WRITE:
	case FUTEX_ART:
		return stack_trace_save(addr_buf, entries, skipnr + 2);
#ifdef INCLUDE_UNUSE
	case FUTEX_JUC:
		break;
#endif
	default:
		pr_err("[kern_lock_stat]:Can not handle unknown type!\n");
		return -1;
	}

	return 0;
}

/*********************************commom***********************************/


/*******************************fatal info********************************/
struct record_struct {
	struct list_head fatal_head;
	int list_cnt;
	spinlock_t fatal_lock;
}____cacheline_aligned_in_smp;

static struct record_struct fatal_record[LOCK_TYPES];

#define FATAL_RECORD_MAX		5
#define FATAL_TRACE_DEPTH		6

struct fatal_info {
	u64 timestamp;
	u64 wait_time;
	u32 wait_type;
	pid_t waiter_tid;
	pid_t waiter_pid;
	enum group_type grp_type;
	char waiter_comm[TASK_COMM_LEN];

	unsigned long addr[FATAL_TRACE_DEPTH];
        int naddrs;

	struct list_head node;
};

static int insert_fatal_info(int type, struct fatal_info *new) {
	struct fatal_info *first;

	if (type < 0 || type >= LOCK_TYPES)
		return -1;

	spin_lock(&fatal_record[type].fatal_lock);
	if (fatal_record[type].list_cnt < FATAL_RECORD_MAX) {
		list_add_tail(&new->node, &fatal_record[type].fatal_head);
		fatal_record[type].list_cnt++;
		goto done;
	}
	first = list_first_entry_or_null(&fatal_record[type].fatal_head, struct fatal_info, node);
	if (!first)
		goto err;

	list_add_tail(&new->node, &fatal_record[type].fatal_head);

	list_del(&first->node);
	kfree(first);

done:
	spin_unlock(&fatal_record[type].fatal_lock);
	return 0;
err:
	spin_unlock(&fatal_record[type].fatal_lock);
	return -1;
}

static noinline int gen_insert_fatal_info(int type, u64 time)
{
	struct fatal_info *info;
	struct task_struct *leader;
	int ret;

	info = (struct fatal_info*)kmalloc(sizeof(*info), GFP_ATOMIC);
	if (!info)
		return -ENOMEM;


	info->timestamp = sched_clock();
	info->wait_type = type;
	memcpy(info->waiter_comm, current->comm, TASK_COMM_LEN);
	info->waiter_tid = current->pid;
	info->wait_time = time;
	info->grp_type = get_task_grp_idx();
	info->waiter_pid = 0;

	rcu_read_lock();
	leader = current->group_leader;
	if (leader)
		info->waiter_pid = leader->pid;
	rcu_read_unlock();

	info->naddrs = fetch_trace_addr(type, FATAL_TRACE_DEPTH, info->addr, 1);
        if (info->naddrs < 1) {
                kfree(info);
                return -1;
        }

	ret = insert_fatal_info(type, info);
	if (ret < 0)
		kfree(info);

	return ret;
}


static void fatal_collect_init(void)
{
	int i;

	for (i = 0; i < LOCK_TYPES; i++) {
		INIT_LIST_HEAD(&fatal_record[i].fatal_head);
		spin_lock_init(&fatal_record[i].fatal_lock);
	}
}

static void fatal_collect_exit(void)
{
	struct fatal_info *p, *tmp;
	int i;

	for (i = 0; i < LOCK_TYPES; i++) {
		list_for_each_entry_safe(p, tmp, &fatal_record[i].fatal_head, node) {
			kfree(p);
		}
	}
}

#define FATAL_SHOW_MAX_BUF   (8 * 1024)
static int fatal_stats_show(struct seq_file *m, void *v)
{
	char *buf;
	int i, k, idx = 0;
	int ret;
	bool found = false;
	char trace_str[KSYM_SYMBOL_LEN];
	struct fatal_info *info;

	buf = (char *)kmalloc(FATAL_SHOW_MAX_BUF, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	for (i = 0; i < LOCK_TYPES; i++) {
		spin_lock(&fatal_record[i].fatal_lock);
		ret = snprintf(&buf[idx], (FATAL_SHOW_MAX_BUF - idx), "%s-{", lock_str[i]);
		if ((ret < 0) || (ret >= FATAL_SHOW_MAX_BUF - idx))
			goto err;
		idx += ret;

		list_for_each_entry(info, &fatal_record[i].fatal_head, node) {
			ret = snprintf(&buf[idx], (FATAL_SHOW_MAX_BUF - idx),
				"[stamp:%lu,wait_time:%lu,wait_type:%d,waiter_tid:%ld,waiter_pid:%ld,"
				"grp_type:%s,waiter_comm:%s,msg:", info->timestamp,
				info->wait_time, info->wait_type, info->waiter_tid,
				info->waiter_pid, group_str[info->grp_type],
				info->waiter_comm);

			if ((ret < 0) || (ret >= FATAL_SHOW_MAX_BUF - idx))
				goto err;
			idx += ret;

			found = false;
			for (k = 0; k < info->naddrs; k++) {
				sprint_symbol_no_offset(trace_str, info->addr[k]);

				if (!found) {
					if (strncmp(func_str[info->wait_type], trace_str,
								strlen(func_str[info->wait_type]) - 1)) {
						continue;
					} else {
						found = true;
						continue;
					}
				}
				if (k != info->naddrs - 1)
					ret = snprintf(&buf[idx],
							(FATAL_SHOW_MAX_BUF - idx), "%s <- ", trace_str);
				else
					ret = snprintf(&buf[idx],
							(FATAL_SHOW_MAX_BUF - idx), "%s],", trace_str);
				if ((ret < 0) || (ret >= FATAL_SHOW_MAX_BUF - idx))
					goto err;
				idx += ret;
			}
		}
		ret = snprintf(&buf[idx], (FATAL_SHOW_MAX_BUF - idx), "}\n");
		if ((ret < 0) || (ret >= FATAL_SHOW_MAX_BUF - idx))
			goto err;
		idx += ret;

		spin_unlock(&fatal_record[i].fatal_lock);
	}
	buf[idx] = '\0';
	seq_printf(m, "%s\n", buf);
	kfree(buf);

	return 0;
err:
	spin_unlock(&fatal_record[i].fatal_lock);
	kfree(buf);
	return -EFAULT;
}


static int fatal_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, fatal_stats_show, inode);
}

static const struct file_operations fatal_stat_fops = {
	.open		= fatal_stats_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
/*******************************fatal info********************************/


/********************************top info********************************/
#ifdef CONFIG_OPLUS_INTERNAL_VERSION

/* Showing TOP_NUM nodes when cat top_lock_stats.*/
#define TOP_NUM    20
#define MAX_BUCKET 256

/* Limit to max node recorded.*/
#define MAX_NODE_IN_HASH_PER_GRP	100

#define TOP_TRACE_DEPTH			6

struct top_node {
	struct hlist_node node;
	unsigned long addr[TOP_TRACE_DEPTH];
	int naddrs;
	long cnt;
	int type;
	int grp_idx;
};

struct top_info {
	struct hlist_head *top_htlb[MAX_BUCKET];
	atomic_t total_cnt[LIMIT_GRP_TYPES];
	char *top_hash_pool;
};

static struct top_info tinfo;

static int hash_func(unsigned long *addr, int len)
{
	int i;
	unsigned long sum = 0;

	if (!addr)
		return -1;

	for (i = 0; i < len; i++) {
		sum += addr[i];
	}
	return (int)(sum / 8 % MAX_BUCKET);
}

static struct hlist_head *top_hash_head(int key)
{
	if (key >= MAX_BUCKET)
		return NULL;
	return tinfo.top_htlb[key];
}

static int hash_insert_node(struct top_node *n)
{
	struct hlist_head *head;
	struct top_node *tmp;
	int key;
	int i;

	if (!n)
		return -1;

	key = hash_func(n->addr, n->naddrs);
	if (key < 0)
		return -1;

	head = top_hash_head(key);
	if (!head) {
		return -1;
	}

	hlist_for_each_entry(tmp, head, node) {
		for (i = 0; i < TOP_TRACE_DEPTH; i++) {
			if (tmp->addr[i] != n->addr[i])
				break;
		}
		if (TOP_TRACE_DEPTH != i)
			continue;
		if ((tmp->type != n->type) || (tmp->grp_idx != n->grp_idx))
			continue;

		/* Find a same node, just increase cnt and return old node */
		tmp->cnt += n->cnt;
		kfree(n);
		return 0;
	}

	if (atomic_read(&tinfo.total_cnt[n->grp_idx]) >= MAX_NODE_IN_HASH_PER_GRP) {
		kfree(n);
		trace_printk("[kern_lock_stat] : Top node reach max limit, ignore it.\n");
		return 0;
	}

	hlist_add_head(&n->node, head);
	atomic_inc(&tinfo.total_cnt[n->grp_idx]);

	return 1;
}

static noinline struct top_node *gen_top_node(int type, int grp_idx)
{
	struct top_node *node;

	if (type < 0 || type >= LOCK_TYPES ||
		grp_idx < 0 || grp_idx >= GRP_TYPES)
		return NULL;

	node = (struct top_node *)kmalloc(sizeof(*node), GFP_ATOMIC);
	if (!node)
		return NULL;

	node->naddrs = fetch_trace_addr(type, TOP_TRACE_DEPTH, node->addr, 0);
	if (node->naddrs < 1) {
		kfree(node);
		return NULL;
	}

	node->type = type;
	node->cnt = 1;
	node->grp_idx = TO_LIMIT_GRP_IDX(grp_idx);

	return node;
}


static __always_inline int update_node(int type, int grp_idx)
{
	struct top_node *new;
	int ret;

	new = gen_top_node(type, grp_idx);
	if (!new)
		return -EFAULT;

	ret = hash_insert_node(new);
	if (ret < 0)
		kfree(new);

	return ret;
}

static int top_lock_hash_init(void)
{
	int i;

	tinfo.top_hash_pool = kmalloc(sizeof(struct hlist_head) * MAX_BUCKET, GFP_KERNEL);
	if (!tinfo.top_hash_pool)
		return -ENOMEM;

	for (i = 0; i < MAX_BUCKET; i++) {
		tinfo.top_htlb[i] = (struct hlist_head *)(tinfo.top_hash_pool
						+ sizeof(struct hlist_head) * i);
		INIT_HLIST_HEAD(tinfo.top_htlb[i]);
	}

	return 0;
}


static void top_lock_hash_exit(void)
{
	struct top_node *node;
	struct hlist_node *tmp;
	int i;

	for (i = 0; i < MAX_BUCKET; i++) {
		hlist_for_each_entry_safe(node, tmp, tinfo.top_htlb[i], node) {
			kfree(node);
		}
	}
	kfree(tinfo.top_hash_pool);
}



static int compare_cnt(const void *a, const void *b)
{
	struct top_node *p1 = *(struct top_node**)a;
	struct top_node *p2 = *(struct top_node**)b;

	if (!p1 || !p2) {
		pr_err("[kern_lock_stat]:p1 or p2 is NULL in compare_cnt \n");
		return 0;
	}

	return p2->cnt - p1->cnt;
}

#define TOP_SHOW_MAX_BUF   (TOP_NUM * 3 * 180)
static int top_stats_show(struct seq_file *m, void *v)
{
	char *buf;
	int idx = 0;
	int i, j, k, ret;
	unsigned long vector[LIMIT_GRP_TYPES][MAX_NODE_IN_HASH_PER_GRP];
	int node_nr[LIMIT_GRP_TYPES] = {0};
	char trace_str[KSYM_SYMBOL_LEN];
	struct top_node *p;
	u64 start;
	bool found = false;

	buf = (char *)kmalloc(TOP_SHOW_MAX_BUF, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ret = snprintf(&buf[idx], (TOP_SHOW_MAX_BUF - idx), "%-10s%-18s%-10s%s\n",
					"group", "locktype", "counts", "trace_function");
	if (ret < 0 || ret >= TOP_SHOW_MAX_BUF - idx) {
		kfree(buf);
		return -EFAULT;
	}
	idx += ret;

	start = sched_clock();
	for (i = 0; i < MAX_BUCKET; i++) {
		hlist_for_each_entry(p, tinfo.top_htlb[i], node) {
			if (node_nr[p->grp_idx] >= MAX_NODE_IN_HASH_PER_GRP) {
				pr_err("[kern_lock_stat]:top node is more than %d \n",
							MAX_NODE_IN_HASH_PER_GRP);
				break;
			}
			vector[p->grp_idx][node_nr[p->grp_idx]++] = (unsigned long)p;
		}
	}

	sort(vector[0], node_nr[0], sizeof(unsigned long), compare_cnt, NULL);
	sort(vector[1], node_nr[1], sizeof(unsigned long), compare_cnt, NULL);
	sort(vector[2], node_nr[2], sizeof(unsigned long), compare_cnt, NULL);

	for (j = 0; j < LIMIT_GRP_TYPES; j++) {
		for (i = 0; i < TOP_NUM && i < node_nr[j]; i++) {
			p = (struct top_node*)vector[j][i];
			if (NULL == p)
				break;
			ret = snprintf(&buf[idx], (TOP_SHOW_MAX_BUF - idx), "%-10s%-18s%-10d",
					group_str[p->grp_idx], lock_str[p->type],
					p->cnt);
			if ((ret < 0) || (ret >= TOP_SHOW_MAX_BUF - idx))
				goto err;
			idx += ret;

			found = false;
			for (k = 0; k < p->naddrs; k++) {
				sprint_symbol_no_offset(trace_str, p->addr[k]);

				if (!found) {
					if (strncmp(func_str[p->type], trace_str, strlen(func_str[p->type]) - 1)) {
						continue;
					} else {
						found = true;
						continue;
					}
				}

				if (k != p->naddrs - 1)
					ret = snprintf(&buf[idx],
						(TOP_SHOW_MAX_BUF - idx), "%s <- ", trace_str);
				else
					ret = snprintf(&buf[idx],
						(TOP_SHOW_MAX_BUF - idx), "%s \n", trace_str);
				if ((ret < 0) || (ret >= TOP_SHOW_MAX_BUF - idx))
					goto err;
				idx += ret;
			}
		}
		ret = snprintf(&buf[idx], (TOP_SHOW_MAX_BUF - idx), "\n");
		if ((ret < 0) || (ret >= TOP_SHOW_MAX_BUF - idx))
			goto err;
		idx += ret;
	}
	ret = snprintf(&buf[idx], (TOP_SHOW_MAX_BUF - idx),
				"\ntotal hash node = %lu, %lu, %lu \n",
				atomic_read(&tinfo.total_cnt[0]), atomic_read(&tinfo.total_cnt[1]),
				atomic_read(&tinfo.total_cnt[2]));
	if ((ret < 0) || (ret >= TOP_SHOW_MAX_BUF - idx))
		goto err;
	idx += ret;

	ret = snprintf(&buf[idx], (TOP_SHOW_MAX_BUF - idx),
				"total use time in show operation = %lu \n", sched_clock() - start);
	if ((ret < 0) || (ret >= TOP_SHOW_MAX_BUF - idx))
		goto err;
	idx += ret;

	buf[idx] = '\0';
	seq_printf(m, "%s\n", buf);
	kfree(buf);
	return 0;

err:
	kfree(buf);
	return -EFAULT;
}

static int top_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, top_stats_show, inode);
}

static const struct file_operations top_stat_fops = {
	.open		= top_stats_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

/********************************top info********************************/



/*****************************generic stats********************************/
#define SHOW_STAT_BUF_SIZE  (3 * PAGE_SIZE)

struct track_stat {
	atomic_t	level[MAX_THRES];
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
	atomic_t	total_nr;                       /* total contended counts. */
	atomic64_t	total_time;                     /* total contended time. */
	atomic64_t	exp_total_time;                 /* total time exceed low thres. */
#endif
};

struct lock_stats {
	struct track_stat       per_type_stat[LOCK_TYPES];
};

/*
 * There are many types os thread groups, For memory considerations,
 * divide multiple groups in 3 : UX/RT/OTHER.
 */
static __read_mostly u32 thres_array[LIMIT_GRP_TYPES][LOCK_TYPES][MAX_THRES]  = {
			/* UX */
			{
#ifdef INCLUDE_UNUSE
			{0, 0, 0},  /* spinlock, not yet implement.*/
#endif
			{8 * NSEC_PER_MSEC, 15 * NSEC_PER_MSEC, 30 * NSEC_PER_MSEC},
			{8 * NSEC_PER_MSEC, 15 * NSEC_PER_MSEC, 30 * NSEC_PER_MSEC},
			{8 * NSEC_PER_MSEC, 15 * NSEC_PER_MSEC, 30 * NSEC_PER_MSEC},
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
			{1 * NSEC_PER_MSEC, 4 * NSEC_PER_MSEC, 20 * NSEC_PER_MSEC},
			{0, 0, 0},
			{1 * NSEC_PER_MSEC, 3 * NSEC_PER_MSEC, 15 * NSEC_PER_MSEC},
#endif
			{30 * NSEC_PER_MSEC, 50 * NSEC_PER_MSEC, 100 * NSEC_PER_MSEC},
#ifdef INCLUDE_UNUSE
			{0, 0, 0},
#endif
			},
			/* RT */
			{
#ifdef INCLUDE_UNUSE
			{0, 0, 0},
#endif
			{2 * NSEC_PER_MSEC, 5 * NSEC_PER_MSEC, 20 * NSEC_PER_MSEC},
			{2 * NSEC_PER_MSEC, 5 * NSEC_PER_MSEC, 20 * NSEC_PER_MSEC},
			{2 * NSEC_PER_MSEC, 5 * NSEC_PER_MSEC, 20 * NSEC_PER_MSEC},
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
			{300 * NSEC_PER_USEC, 1 * NSEC_PER_MSEC, 5 * NSEC_PER_MSEC},
			{0, 0, 0},
			{300 * NSEC_PER_USEC, 1 * NSEC_PER_MSEC, 5 * NSEC_PER_MSEC},
#endif
			{2 * NSEC_PER_MSEC, 5 * NSEC_PER_MSEC, 20 * NSEC_PER_MSEC},
#ifdef INCLUDE_UNUSE
			{0, 0, 0},
#endif
			},
			/* OTHER */
			{
#ifdef INCLUDE_UNUSE
			{0, 0, 0},
#endif
			{12 * NSEC_PER_MSEC, 25 * NSEC_PER_MSEC, 40 * NSEC_PER_MSEC},
			{12 * NSEC_PER_MSEC, 25 * NSEC_PER_MSEC, 50 * NSEC_PER_MSEC},
			{12 * NSEC_PER_MSEC, 25 * NSEC_PER_MSEC, 50 * NSEC_PER_MSEC},

#ifdef CONFIG_OPLUS_INTERNAL_VERSION
			{5 * NSEC_PER_MSEC, 8 * NSEC_PER_MSEC, 30 * NSEC_PER_MSEC},
			{0, 0, 0},
			{5 * NSEC_PER_MSEC, 8 * NSEC_PER_MSEC, 30 * NSEC_PER_MSEC},
#endif
			{80 * NSEC_PER_MSEC, 160 * NSEC_PER_MSEC, 300 * NSEC_PER_MSEC},
#ifdef INCLUDE_UNUSE
			{0, 0, 0},
#endif
			}
};

static u32 proc_type[LOCK_TYPES];
static struct lock_stats stats_info[GRP_TYPES];


static int waittime_thres_exceed_type(int grp_idx, int type, u64 time)
{
	int real_idx;

	real_idx = TO_LIMIT_GRP_IDX(grp_idx);
	if (time > thres_array[real_idx][type][FATAL_THRES]) {
		return FATAL_THRES;
	} else if (time > thres_array[real_idx][type][HIGH_THRES]) {
		return HIGH_THRES;
	} else if (time > thres_array[real_idx][type][LOW_THRES]) {
		return LOW_THRES;
	} else {
		return -1;
	}
}

static int track_stat_update(int grp_idx, int type, struct track_stat *ts, u64 time)
{
	int thres_type;

	if (NULL == ts)
		return -1;

	thres_type = waittime_thres_exceed_type(grp_idx, type, time);
	if (thres_type < 0)
		return thres_type;

	atomic_inc(&ts->level[thres_type]);
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
	atomic64_add(time, &ts->exp_total_time);

#endif
	cond_trace_printk(locking_opt_debug(LK_DEBUG_FTRACE),
			"[kern_lock_stat] : add exceed low thres item, grp = %s,"
			"type = %s, time = %lu\n", group_str[grp_idx],
			lock_str[type], time);
	return thres_type;
}




static int lock_stats_update(int grp_idx, int type, u64 time)
{
	struct lock_stats *pcs;
	int ret;

	/*
	 * Get percpu data ptr.
	 * Prevent process from switching to another CPU, disable preempt.
	 * There only is a atomic_inc operation(and/or a trace printk)
	 * during the preemtion off, no particularly time-consuming operations.
	 */
	pcs = &stats_info[grp_idx];

	ret = track_stat_update(grp_idx, type, &pcs->per_type_stat[type], time);

#ifdef CONFIG_OPLUS_INTERNAL_VERSION
	/*Total nr++, Whether or not the minimum threshold is exceeded*/
	atomic_inc(&pcs->per_type_stat[type].total_nr);
	atomic64_add(time, &pcs->per_type_stat[type].total_time);
#endif

	return ret;
}


__always_inline void handle_wait_stats(int type, u64 time)
{
	int grp_idx;
	int ret;

	grp_idx = get_task_grp_idx();
	ret = lock_stats_update(grp_idx, type, time);
	if (ret >= 0) {
		/*
		 * Only record UX/RT's fatal info, We don't care other groups.
		 */
		if ((ret == FATAL_THRES) && (grp_idx <= GRP_RT)) {
			if (gen_insert_fatal_info(type, time))
				pr_err("[kern_lock_stat]:Failed to generate&insert fatal info \n");
		}
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
		/*
		 * Update top node while exceed low thres.
		 * gen_top_node may return NULL, update_node can cover it.
		 */
		if (update_node(type, grp_idx) < 0)
			pr_err("[kern_lock_stat]:Failed to update top node \n");
#endif
	}
}


static ssize_t lock_thres_ctrl_write(struct file *file,
			       const char __user * user_buffer, size_t count,
			       loff_t * offset)
{
	char *lvl_str[] = {"low", "high", "fatal"};
	char kbuf[64];
	char grp[10], lock[20], level[10];
	int ret, i, j, k;
	u32 val;

	if (count < 15 || count >= 64) {
		pr_err("[kern_lock_stat]:Invailid argument \n");
		return -EINVAL;
	}

	if (copy_from_user(kbuf, user_buffer, count)) {
		pr_err("[kern_lock_stat]:Copy from user failed\n");
		return -EFAULT;
	}

	kbuf[count] = '\0';

	ret = sscanf(kbuf, "%9s %19s %9s %10d", grp, lock, level, &val);
	if (ret != 4) {
		pr_err("[kern_lock_stat]:Failed to execute sscanf\n");
		return -EFAULT;
	}

	for (i = 0; i < LIMIT_GRP_TYPES; i++) {
		if (0 == strcmp(grp, group_str[i]))
			break;
	}
	if (LIMIT_GRP_TYPES == i) {
		pr_err("[kern_lock_stat]:Invalid Group\n");
		return -EFAULT;
	}

	for (j = 0; j < LOCK_TYPES; j++) {
		if (0 == strcmp(lock, lock_str[j]))
			break;
	}
	if (LOCK_TYPES == j) {
		pr_err("[kern_lock_stat]:Invalid lock name\n");
		return -EFAULT;
	}

	for (k = 0; k < 3; k++) {
		if (0 == strcmp(level, lvl_str[k]))
			break;
	}
	if (3 == k) {
		pr_err("[kern_lock_stat]:Invalid level name\n");
		return -EFAULT;
	}

	WRITE_ONCE(thres_array[i][j][k], val * 1000);

	return count;
}

static int lock_thres_ctrl_show(struct seq_file *m, void *v)
{
	char *buf;
	int i, j, idx = 0;
	int ret;

	/* PAGE_SIZE is big enough, noneed to use snprintf. */
	buf = (char *)kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ret = sprintf(&buf[idx], "/*************************************************/\n"
			"Set Usage: \necho group lock_type level value > THIS_FILE \n"
			"Example: echo GRP_UX spinlock low 100000 > ${THIS_FILE} \n\n"
			"Note:Please refer to the output below for the group name and lock name,\n"
			"which are case-sensitive and strictly follow the format, "
			"otherwise the setting will fail."
			"\n/*************************************************/\n\n");

	if ((ret < 0) || (ret >= PAGE_SIZE - idx))
		goto err;
	idx += ret;


	ret = sprintf(&buf[idx], "%-20s%-15s%-15s%-15s\n", "lock-type",
			"low(us)", "high(us)", "fatal(us)");
	if ((ret < 0) || (ret >= PAGE_SIZE - idx))
		goto err;

	idx += ret;

	for (i = 0; i < LIMIT_GRP_TYPES; i++) {
		ret = sprintf(&buf[idx], "\n%s:\n", group_str[i]);
		if ((ret < 0) || (ret >= PAGE_SIZE - idx))
			goto err;
		idx += ret;

		for (j = 0; j < LOCK_TYPES; j++) {
			ret = sprintf(&buf[idx], "%-20s%-15lu%-15lu%-15lu\n", lock_str[j],
						thres_array[i][j][LOW_THRES] / 1000,
						thres_array[i][j][HIGH_THRES] / 1000,
						thres_array[i][j][FATAL_THRES] / 1000);
			if ((ret < 0) || (ret >= PAGE_SIZE - idx))
				goto err;
			idx += ret;
		}
	}
	buf[idx] = '\n';
	seq_printf(m, "%s\n", buf);
	kfree(buf);
	return 0;

err:
	kfree(buf);
	return -EFAULT;
}

static int lock_thres_ctrl_open(struct inode *inode, struct file *file)
{
	return single_open(file, lock_thres_ctrl_show, PDE_DATA(inode));
}

static const struct file_operations lock_thres_ctrl_fops = {
	.open		= lock_thres_ctrl_open,
	.write		= lock_thres_ctrl_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};


static int show_stats(char *buf, u32 blen)
{
	int i, j;
	int idx = 0;
	int ret;
	/*
	 * groups -> locks -> low/high/fatal.
	 * Cause the stack size limit(2048), We can't just define a static array
	 * with 3 columns. The last column of memory get from kmalloc.
	 */
	struct track_stat *lock_count_info;

	for (i = 0; i < GRP_TYPES; i++) {
		for (j = 0; j < LOCK_TYPES; j++) {
			if (0 == j) {
        			ret = snprintf(&buf[idx], (blen - idx), "%s-{", group_str[i]);
				if ((ret < 0) || (ret >= blen - idx))
					goto err;
				idx += ret;
			}

			lock_count_info = &stats_info[i].per_type_stat[j];
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
			ret = snprintf(&buf[idx], (blen - idx), "%s:[%lu,%lu,%lu,%lu,%lu,%lu]",
					lock_str[j], atomic_read(&lock_count_info->level[0]),
					atomic_read(&lock_count_info->level[1]),
					atomic_read(&lock_count_info->level[2]),
					atomic_read(&lock_count_info->total_nr),
					atomic64_read(&lock_count_info->total_time),
					atomic64_read(&lock_count_info->exp_total_time));
#else
			ret = snprintf(&buf[idx], (blen - idx), "%s:[%lu,%lu,%lu]",
					lock_str[j], atomic_read(&lock_count_info->level[0]),
					atomic_read(&lock_count_info->level[1]),
					atomic_read(&lock_count_info->level[2]));
#endif
			if ((ret < 0) || (ret >= blen - idx))
				goto err;
			idx += ret;

			if ((LOCK_TYPES - 1) != j) {
				ret = snprintf(&buf[idx], (blen - idx), ",");
				if ((ret < 0) || (ret >= blen - idx))
					goto err;
				idx += ret;
			}
		}
		ret = snprintf(&buf[idx], (blen - idx), "}\n");
		if ((ret < 0) || (ret >= blen - idx))
			goto err;
		idx += ret;
	}

	buf[idx] = '\0';

	return 0;

err:
	return -EFAULT;
}



static void clear_stats(void)
{
	int i, j;

	for (i = 0; i < GRP_TYPES; i++) {
		for (j = 0; j < LOCK_TYPES; j++) {
			atomic_set(&stats_info[i].per_type_stat[j].level[0], 0);
			atomic_set(&stats_info[i].per_type_stat[j].level[1], 0);
			atomic_set(&stats_info[i].per_type_stat[j].level[2], 0);
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
			atomic_set(&stats_info[i].per_type_stat[j].total_nr, 0);
			atomic64_set(&stats_info[i].per_type_stat[j].total_time, 0);
			atomic64_set(&stats_info[i].per_type_stat[j].exp_total_time, 0);
#endif
		}
	}
}

static void read_clear_stats(void)
{
	clear_stats();
}
static int lock_stats_rclear_show(struct seq_file *m, void *v)
{
	char *buf;
	int ret;

	buf = kmalloc(SHOW_STAT_BUF_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ret = show_stats(buf, SHOW_STAT_BUF_SIZE);
	if (ret < 0) {
		kfree(buf);
		return ret;
	}

	read_clear_stats();

	seq_printf(m, "%s\n", buf);
	kfree(buf);

	return 0;
}

static int lock_stats_rclear_open(struct inode *inode, struct file *file)
{
	return single_open(file, lock_stats_rclear_show, inode);
}


static const struct file_operations lock_stat_rclear_fops = {
	.open		= lock_stats_rclear_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};


static int lock_stats_show(struct seq_file *m, void *v)
{
	char *buf;
	int ret;

	buf = kmalloc(SHOW_STAT_BUF_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ret = show_stats(buf, SHOW_STAT_BUF_SIZE);
	if (ret < 0) {
		kfree(buf);
		return -EFAULT;
	}

	seq_printf(m, "%s\n", buf);
	kfree(buf);

	return 0;
}

static int lock_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, lock_stats_show, inode);
}


static const struct file_operations lock_stat_fops = {
	.open		= lock_stats_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};


static int per_lock_stats_show(struct seq_file *m, void *v)
{
	char *buf;
	u32 *ptr;
	int type;
	int i, ret, idx = 0;
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
	char time[64], exp_time[64];
#endif
	struct track_stat *lock_count_info;

	ptr = (u32*)m->private;
	if (NULL == ptr) {
		return -EFAULT;
	}
	type = *ptr;

	buf = (char*)kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

#ifdef CONFIG_OPLUS_INTERNAL_VERSION
	ret = snprintf(&buf[idx], (PAGE_SIZE - idx), "%-12s%-12s%-12s%-12s%-12s%-12s%-12s\n",
			" ", "low", "high", "fatal", "total_nr", "total_time", "exp_total_time");
#else
	ret = snprintf(&buf[idx], (PAGE_SIZE - idx), "%-12s%-12s%-12s%-12s\n",
			" ", "low", "high", "fatal");
#endif
	if ((ret < 0) || (ret >= PAGE_SIZE - idx))
		goto err;
	idx += ret;

	for (i = 0; i < GRP_TYPES; i++) {
		lock_count_info = &stats_info[i].per_type_stat[type];
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
		print_time(atomic64_read(&lock_count_info->total_time), time, 64);
		print_time(atomic64_read(&lock_count_info->exp_total_time), exp_time, 64);
		ret = snprintf(&buf[idx], (PAGE_SIZE - idx), "%-12s%-12u%-12u%-12u%-12u%-12s%-12s\n",
				group_str[i], atomic_read(&lock_count_info->level[0]),
				atomic_read(&lock_count_info->level[1]),
				atomic_read(&lock_count_info->level[2]),
				atomic_read(&lock_count_info->total_nr),
				time, exp_time);
#else
		ret = snprintf(&buf[idx], (PAGE_SIZE - idx), "%-12s%-12u%-12u%-12u\n",
				group_str[i], atomic_read(&lock_count_info->level[0]),
				atomic_read(&lock_count_info->level[1]),
				atomic_read(&lock_count_info->level[2]));
#endif
		if ((ret < 0) || (ret >= PAGE_SIZE - idx))
			goto err;
		idx += ret;
	}

	buf[idx] = '\0';
	seq_printf(m, "%s\n", buf);
	kfree(buf);

	return 0;

err:
	kfree(buf);
	return -EFAULT;
}

static int per_lock_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, per_lock_stats_show, PDE_DATA(inode));
}

static const struct file_operations per_lock_stat_fops = {
	.open		= per_lock_stats_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

#define LOCK_STATS_DIRNAME "lock_stats"

extern struct proc_dir_entry *d_oplus_locking;
struct proc_dir_entry *d_lock_stats;
static int create_stats_procs(void)
{
	struct proc_dir_entry *p;
	int i;

	if (!d_oplus_locking)
		return -ENOMEM;

	d_lock_stats = proc_mkdir(LOCK_STATS_DIRNAME, d_oplus_locking);
	if (NULL == d_lock_stats)
		goto err;

	for (i = 0; i < LOCK_TYPES; i++) {
		proc_type[i] = i;
		p = proc_create_data(lock_str[i], S_IRUGO | S_IWUGO,
					d_lock_stats, &per_lock_stat_fops, &proc_type[i]);
		if (NULL == p)
			goto err1;
	}

	/*For error condtion, to clean up. */
	i -= 1;

	p = proc_create("kern_lock_stats", S_IRUGO | S_IWUGO,
			d_oplus_locking, &lock_stat_fops);
	if (NULL == p)
		goto err1;

	p = proc_create("kern_lock_stats_rclear", S_IRUGO | S_IWUGO,
			d_oplus_locking, &lock_stat_rclear_fops);
	if (NULL == p)
		goto err2;

	p = proc_create("fatal_lock_stats", S_IRUGO | S_IWUGO,
			d_oplus_locking, &fatal_stat_fops);
	if (NULL == p)
		goto err3;

	p = proc_create("lock_thres_ctrl", S_IRUGO | S_IWUGO,
			d_oplus_locking, &lock_thres_ctrl_fops);
	if (NULL == p)
		goto err4;

#ifdef CONFIG_OPLUS_INTERNAL_VERSION
	p = proc_create("top_lock_stats", S_IRUGO | S_IWUGO,
			d_oplus_locking, &top_stat_fops);
	if (NULL == p)
		goto err5;
#endif

	return 0;

#ifdef CONFIG_OPLUS_INTERNAL_VERSION
err5:
	remove_proc_entry("lock_thres_ctrl", d_oplus_locking);
#endif

err4:
	remove_proc_entry("fatal_lock_stats", d_oplus_locking);
err3:
	remove_proc_entry("kern_lock_stats_rclear", d_oplus_locking);
err2:
	remove_proc_entry("kern_lock_stats", d_oplus_locking);
err1:
	for (; i >= 0; i--) {
		remove_proc_entry(lock_str[i], d_lock_stats);
	}
	remove_proc_entry(LOCK_STATS_DIRNAME, d_oplus_locking);
err:
	return -ENOMEM;
}

static void remove_stats_procs(void)
{
	int i;
	remove_proc_entry("kern_lock_stats", d_oplus_locking);
        remove_proc_entry("kern_lock_stats_rclear", d_oplus_locking);
        remove_proc_entry("fatal_lock_stats", d_oplus_locking);
        remove_proc_entry("lock_thres_ctrl", d_oplus_locking);
#ifdef CONFIG_OPLUS_INTERNAL_VERSION
        remove_proc_entry("top_lock_stats", d_oplus_locking);
#endif
	for (i = 0; i < LOCK_TYPES; i++) {
		remove_proc_entry(lock_str[i], d_lock_stats);
	}
	remove_proc_entry(LOCK_STATS_DIRNAME, d_oplus_locking);
}

/******************************generic stats********************************/



/****************************hooks operations*****************************/
static __always_inline void lk_contended(int type)
{
	if (current->lkinfo.waittime_stamp) {
		trace_printk("kern_lock_stat : error start,times_stamp not 0,type = %d !\n", type);
		return;
	}

	current->lkinfo.waittime_stamp = lockstat_clock();
}


static __always_inline void lk_acquired(int type)
{
	u64 now, delta = 0;

	if (current->lkinfo.waittime_stamp) {
		now = lockstat_clock();
		delta = now - current->lkinfo.waittime_stamp;
		current->lkinfo.waittime_stamp = 0;
		handle_wait_stats(type, delta);
	} else {
		trace_printk("kern_lock_stat : error end,"
			"no start recorded, type = %s !\n", lock_str[type]);
		return;
	}
}

void monitor_futex_wait_start(unsigned int flags, u32 bitset)
{
	int app_type, kern_type;

	app_type = flags >> LOCK_TYPE_SHIFT;

#ifdef INCLUDE_UNUSE
	kern_type = app_type + LOCK_TYPES - 3;
	if (app_type & (LOCK_ART | LOCK_JUC)) {
		lk_contended(kern_type);
	}
#else
	kern_type = app_type + LOCK_TYPES - 2;
	if (app_type & LOCK_ART) {
		lk_contended(kern_type);
	}
#endif
}

void monitor_futex_wait_end(unsigned int flags, u32 bitset)
{
	int app_type, kern_type;

	app_type = flags >> LOCK_TYPE_SHIFT;

#ifdef INCLUDE_UNUSE
	if (!(app_type & (LOCK_ART | LOCK_JUC))) {
		return;
	}
	kern_type = app_type + LOCK_TYPES - 3;
#else
	if (!(app_type & LOCK_ART)) {
		return;
	}
	kern_type = app_type + LOCK_TYPES - 2;
#endif

	lk_acquired(kern_type);
}

void monitor_mutex_wait_start(struct mutex *lock)
{
	lk_contended(MUTEX);
}

void monitor_mutex_wait_finish(struct mutex *lock)
{
	lk_acquired(MUTEX);
}

void monitor_rwsem_read_wait_start(struct rw_semaphore *sem)
{
	lk_contended(RWSEM_READ);
}

void monitor_rwsem_read_wait_finish(struct rw_semaphore *sem)
{
	lk_acquired(RWSEM_READ);
}

void monitor_rwsem_write_wait_start(struct rw_semaphore *sem)
{
	lk_contended(RWSEM_WRITE);
}

void monitor_rwsem_write_wait_finish(struct rw_semaphore *sem)
{
	lk_acquired(RWSEM_WRITE);
}


/****************************hooks operations*****************************/

int kern_lstat_init(void)
{
	int ret;

	fatal_collect_init();

	ret = create_stats_procs();
	if (ret < 0)
		goto err;

#ifdef CONFIG_OPLUS_INTERNAL_VERSION
	ret = top_lock_hash_init();
	if (ret)
		goto err;
#endif

	return 0;

err:
	remove_stats_procs();

	return ret;
}


void  kern_lstat_exit(void)
{
	remove_stats_procs();
	fatal_collect_exit();

#ifdef CONFIG_OPLUS_INTERNAL_VERSION
	top_lock_hash_exit();
#endif
}
