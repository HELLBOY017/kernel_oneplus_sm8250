#ifndef __F2FS_METRICS_H__
#define __F2FS_METRICS_H__
#include <linux/fs.h>
#include <linux/f2fs_fs.h>

void f2fs_register_tracepoint_probes(void);
void f2fs_unregister_tracepoint_probes(void);
int f2fs_metrics_proc_open(struct inode *inode, struct file *file);
void f2fs_metrics_reset(void);
void f2fs_metrics_init(void);

#endif /* __F2FS_METRICS_H__ */