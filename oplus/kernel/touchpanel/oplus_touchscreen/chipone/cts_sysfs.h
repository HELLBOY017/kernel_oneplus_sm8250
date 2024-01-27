#ifndef CTS_SYSFS_H
#define CTS_SYSFS_H

#include "cts_config.h"

struct device;


#ifdef CONFIG_CTS_SYSFS
extern int cts_sysfs_add_device(struct device *dev);
extern void cts_sysfs_remove_device(struct device *dev);
#else
static inline int cts_sysfs_add_device(struct device *dev) {return -ENOTSUPP;}
static inline void cts_sysfs_remove_device(struct device *dev) {}
#endif

#endif /* CTS_SYSFS_H */

