#ifndef __ABNORMAL_IO_H__
#define __ABNORMAL_IO_H__
#include <linux/fs.h>
#include <linux/f2fs_fs.h>

extern atomic_t abnormal_io_enabled;
extern bool abnormal_io_trigger;
extern int abnormal_io_dump_min_interval_s;
extern int abnormal_io_dump_limit_1_day;

int abnormal_io_dump_to_file(const char *logpath);
void abnormal_io_init(void);
void abnormal_io_exit(void);

#endif /* __ABNORMAL_IO_H__ */
