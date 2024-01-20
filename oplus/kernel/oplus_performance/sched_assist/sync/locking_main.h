/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020-2022 Oplus. All rights reserved.
 */

#ifndef _OPLUS_LOCKING_MAIN_H_
#define _OPLUS_LOCKING_MAIN_H_

#define cond_trace_printk(cond, fmt, ...)	\
do {										\
	if (cond)								\
		trace_printk(fmt, ##__VA_ARGS__);	\
} while (0)



#define MAGIC_NUM       (0xdead0000)
#define MAGIC_MASK      (0xffff0000)
#define MAGIC_SHIFT     (16)
#define OWNER_BIT       (1 << 0)
#define THREAD_INFO_BIT (1 << 1)
#define TYPE_BIT        (1 << 2)

#define UX_FLAG_BIT       (1<<0)
#define SS_FLAG_BIT       (1<<1)
#define GRP_SHIFT         (2)
#define GRP_FLAG_MASK     (7 << GRP_SHIFT)
#define U_GRP_OTHER       (1 << GRP_SHIFT)
#define U_GRP_BACKGROUND  (2 << GRP_SHIFT)
#define U_GRP_FRONDGROUD  (3 << GRP_SHIFT)
#define U_GRP_TOP_APP     (4 << GRP_SHIFT)

#define LOCK_TYPE_SHIFT (30)
#define INVALID_TYPE    (0)
#define LOCK_ART        (1)
#define LOCK_JUC        (2)

enum {
	CGROUP_RESV = 0,
	CGROUP_DEFAULT,
	CGROUP_FOREGROUND,
	CGROUP_BACKGROUND,
	CGROUP_TOP_APP,

	CGROUP_NRS,
};

#define LK_MUTEX_ENABLE (1 << 0)
#define LK_RWSEM_ENABLE (1 << 1)
#define LK_FUTEX_ENABLE (1 << 2)

#define LK_OSQ_ENABLE   (1 << 3)

#ifdef CONFIG_OPLUS_LOCKING_MONITOR
/*
 * The bit definitions of the g_opt_enable:
 * bit 0-7: reserved bits for other locking optimation.
 * bit8 ~ bit10(each monitor version is exclusive):
 * 1 : monitor control, level-0(internal version).
 * 2 : monitor control, level-1(trial version).
 * 3 : monitor control, level-2(official version).
 */
#define LK_MONITOR_SHIFT  (8)
#define LK_MONITOR_MASK   (7 << LK_MONITOR_SHIFT)
#define LK_MONITOR_LEVEL0 (1 << LK_MONITOR_SHIFT)
#define LK_MONITOR_LEVEL1 (2 << LK_MONITOR_SHIFT)
#define LK_MONITOR_LEVEL2 (3 << LK_MONITOR_SHIFT)
#endif

#define LK_DEBUG_TRACE  (1 << 0)
#define LK_DEBUG_FTRACE (1 << 1)

extern unsigned int g_opt_enable;
extern unsigned int g_opt_debug;

static inline bool locking_opt_enable(unsigned int enable)
{
	return g_opt_enable & enable;
}

#ifdef CONFIG_OPLUS_LOCKING_MONITOR
static inline bool lock_supp_level(level)
{
	return (g_opt_enable & LK_MONITOR_MASK) == level;
}
#endif

static inline bool locking_opt_debug(int debug)
{
	return g_opt_debug & debug;
}

void lk_sysfs_init(void);
#ifdef CONFIG_OPLUS_LOCKING_MONITOR
int kern_lstat_init(void);
#endif
void lk_sysfs_exit(void);
#ifdef CONFIG_OPLUS_LOCKING_MONITOR
void kern_lstat_exit(void);
#endif
#endif /* _OPLUS_LOCKING_MAIN_H_ */
