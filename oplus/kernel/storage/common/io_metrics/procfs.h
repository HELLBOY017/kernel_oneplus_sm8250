#ifndef __PROFS_H__
#define __PROFS_H__
#include "io_metrics_entry.h"

extern bool proc_show_enabled;
extern struct sample_cycle sample_cycle_config[CYCLE_MAX];
int io_metrics_procfs_init(void);
void io_metrics_procfs_exit(void);

#endif /* __PROFS_H__ */