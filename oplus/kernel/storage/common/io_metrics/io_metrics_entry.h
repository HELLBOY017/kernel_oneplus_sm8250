#ifndef __IO_METRICS_ENTRY_H__
#define __IO_METRICS_ENTRY_H__
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/ctype.h>
#include <linux/blkdev.h>
#include <linux/percpu.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/export.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/task_io_accounting_ops.h>
#include <linux/trace_clock.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/namei.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/tracepoint.h>
#include <linux/fs.h>
#include <linux/bio.h>
#include <linux/printk.h>

#define io_metrics_print(fmt, arg...) \
    printk("[IO_METRICS] [%-16s] %20s:%-4d "fmt, current->comm, __func__, __LINE__, ##arg)

/* 统计周期 */
enum sample_cycle_type {
//    CYCLE_SECOND_10 = 0,
//    CYCLE_MINUTES_1,
//    CYCLE_MINUTES_10,
//    CYCLE_HOUR_1,
//    CYCLE_DAY_1,
//    CYCLE_WEEK_1,
    /* 记录总的流量 */
    CYCLE_FOREVER = 0,
    CYCLE_MAX
};

struct sample_cycle {
    enum sample_cycle_type value;
    const char * tag;
    const u64 cycle_value;
};

extern bool io_metrics_enabled;
extern bool io_metrics_debug_enabled;

#endif /* __IO_METRICS_ENTRY_H__ */