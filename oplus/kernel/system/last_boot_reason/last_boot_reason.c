/* SPDX-License-Identifier: GPL-2.0-only */
/*===========================================================================
#  Copyright (c) 2021, OPLUS Technologies Inc. All rights reserved.
===========================================================================

                              EDIT HISTORY

 when       who        what, where, why
 --------   ---        ----------------------------------------------------------
 10/18/21   Tongyang.Xu   Created file
=============================================================================
* Description:     last_boot_reason Kernel Driver
*
* Version   : 1.0
===========================================================================*/
#include "last_boot_reason.h"

#define LAST_BOOT_REASON_MAX_SIZE 512

#define LAST_BOOT_REASON_PRINTK(a, arg...)\
    do{\
         printk("[Last_Boot_Reason]: " a, ##arg);\
    }while(0)

char global_buffer[LAST_BOOT_REASON_MAX_SIZE] = {0};

static ssize_t last_boot_reason_write(struct file *file, const char __user *buf,
        size_t count,loff_t *off)
{
    if (count >= LAST_BOOT_REASON_MAX_SIZE) {
       count = LAST_BOOT_REASON_MAX_SIZE - 1;
    }

    if (copy_from_user(global_buffer, buf, count)) {
        LAST_BOOT_REASON_PRINTK("%s: read proc input error.\n", __func__);
        return count;
    }

    return count;
}

static ssize_t last_boot_reason_read(struct file *file, char __user *buf,
        size_t count,loff_t *off)
{
    char page[512] = {0};
    int len = 0;

    len = sprintf(&page[len], global_buffer);
    page[511] = '\n';

    if(len > *off)
        len -= *off;
    else
        len = 0;

    if(copy_to_user(buf,page,(len < count ? len : count))){
       return -EFAULT;
    }
    *off += len < count ? len : count;
    return (len < count ? len : count);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    static const struct proc_ops last_boot_reason_fops = {
    .proc_read = last_boot_reason_read,
    .proc_write = last_boot_reason_write,
    .proc_lseek = seq_lseek,
     };
#else
    struct file_operations last_boot_reason_fops = {
    .write       = last_boot_reason_write,
    .read        = last_boot_reason_read,
     };
#endif

int __init init_last_boot_reason(void)
{
    struct proc_dir_entry *pe;

    LAST_BOOT_REASON_PRINTK("last_boot_reason init\n");

    pe = proc_create("last_boot_reason", 0666, NULL, &last_boot_reason_fops);
    if (!pe) {
        LAST_BOOT_REASON_PRINTK("Failed to register last_boot_reason interface\n");
        return -ENOMEM;
    }
    return 0;
}

late_initcall(init_last_boot_reason);

#if IS_MODULE(CONFIG_OPLUS_FEATURE_SHUTDOWN_DETECT)
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
#endif
MODULE_LICENSE("GPL v2");

