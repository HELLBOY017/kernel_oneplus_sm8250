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
#include <linux/version.h>
#include "last_boot_reason.h"

#define LAST_BOOT_REASON_MAX_SIZE 512

#define LAST_BOOT_REASON_PRINTK(a, arg...)\
    do{\
         printk("[Last_Boot_Reason]: " a, ##arg);\
    }while(0)

#define BUF_SIZE 512
#define TARGET_DEV_BLOCK "/dev/block/by-name/oplusreserve1"
#define OPLUS_RESERVE1_SHUDOWN_KERNEL_UFS_START              (1180 * 4096 + 2048)
#define SHUTDOWN_KERNEL_MAGIC "KERNEL_SHD:"

#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 15, 0)
static int blkdev_fsync(struct file *filp, loff_t start, loff_t end, int datasync);
#endif

char global_buffer[LAST_BOOT_REASON_MAX_SIZE] = {0};
static bool shutdown_recorder_enable = false;
extern void *return_address(unsigned int);
int format_buf(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    int r;

    va_start(args, fmt);
    r = vscnprintf(buf, size, "%pS\n", args);
    va_end(args);

    return r;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 15, 0)
static int blkdev_fsync(struct file *filp, loff_t start, loff_t end,
		int datasync)
{
	struct inode *bd_inode = filp->f_mapping->host;
	struct block_device *bdev = I_BDEV(bd_inode);
	int error;

	error = file_write_and_wait_range(filp, start, end);
	if (error)
		return error;

	/*
	 * There is no need to serialise calls to blkdev_issue_flush with
	 * i_mutex and doing so causes performance issues with concurrent
	 * O_SYNC writers to a block device.
	 */
	error = blkdev_issue_flush(bdev);
	if (error == -EOPNOTSUPP)
		error = 0;

	return error;
}
#endif

/*this callback will print stackframe, task name, module name,to partition*/
static int reboot_callback_func(struct notifier_block *nb,
                  unsigned long mode, void *cmd)

/* mode is SYS_RESTART/SYS_HALT/SYS_POWER_OFF
   cmd is from  kernel_restart input parameter buffer 
*/
{
    void *call = NULL;
    uint8_t i = 0;
    char write_buf[BUF_SIZE] = {0};
    char *ptr = write_buf;
    int len = 0, total_len = 0, ret = 0;
    struct block_device *bdev = NULL;
    struct kiocb kiocb;
    struct iov_iter iter;
    struct kvec iov;
    const struct file_operations f_op = {.fsync = blkdev_fsync};
    struct file dev_map_file;


    /*init trigger shutdown/reboot, don't record the kernel dumpstack*/
    if (current->pid == 1)
        return 0;

    len = scnprintf(ptr, sizeof(write_buf), ((current->mm)?"%s\n%d:%s\n":"%s\n%d:[%s]\n"), SHUTDOWN_KERNEL_MAGIC, current->pid, current->comm);
    total_len = len;
    ptr = write_buf + total_len;

    while ((call = return_address(i)) != NULL) {
        len = format_buf(ptr, sizeof(write_buf)-total_len, "%pS\n", call);
        total_len += len;
        /*buf is full*/
        if (total_len > BUF_SIZE  - 1) {
            break;
        }
        i++;
        ptr = write_buf + total_len;
    }

    bdev = blkdev_get_by_path(TARGET_DEV_BLOCK, FMODE_READ | FMODE_WRITE, NULL);

    if (IS_ERR(bdev)) {
        pr_err("Failed to get dev block\n");
        return -1;
    }

    memset(&dev_map_file, 0, sizeof(struct file));

    dev_map_file.f_mapping = bdev->bd_inode->i_mapping;
    dev_map_file.f_flags = O_DSYNC | __O_SYNC | O_NOATIME;
    dev_map_file.f_inode = bdev->bd_inode;

    init_sync_kiocb(&kiocb, &dev_map_file);
    kiocb.ki_pos = OPLUS_RESERVE1_SHUDOWN_KERNEL_UFS_START; /*start header offset*/
    iov.iov_base = write_buf;
    iov.iov_len = total_len;
    iov_iter_kvec(&iter, WRITE | ITER_KVEC, &iov, 1, total_len);

    ret = generic_write_checks(&kiocb, &iter);

    if (likely(ret > 0))
        ret = generic_perform_write(&dev_map_file, &iter, kiocb.ki_pos);
    else {
        pr_err("generic_write_checks failed\n");
        return 1;
    }
    if (likely(ret > 0)) {
        dev_map_file.f_op = &f_op;
        kiocb.ki_pos += ret;
        pr_err("Write back size %d\n", ret);
        ret = generic_write_sync(&kiocb, ret);
        if (ret < 0) {
            pr_err("Write sync failed\n");
            return 1;
        }
    }
    else {
        pr_err("generic_perform_write failed\n");
        return 1;
        }

    filp_close(&dev_map_file, NULL);

    /*pr_err("[oem] buf is %s",write_buf);
    dump_stack();
    panic("reboot_callback_func 2");*/
    return 0;
}

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

static struct notifier_block monitor_reboot_nb = {
    .notifier_call = reboot_callback_func,
};
static ssize_t shutdown_recorder_trigger(struct file *filp, const char *ubuf, size_t cnt, loff_t *data)
{
    char buf[64];
    int val = 0;
    int ret = 0;

    if (cnt >= sizeof(buf)) {
        return -EINVAL;
    }
    if (copy_from_user(&buf, ubuf, cnt)) {
        return -EFAULT;
    }

    buf[cnt] = 0;
    ret = kstrtoint(buf, 0, (int *)&val);

    if (ret < 0) {
        return ret;
    }

    if ((!!val) && (shutdown_recorder_enable == false)) {
        pr_info("shutdown_recorder enable");
        ret = register_reboot_notifier(&monitor_reboot_nb);
        if (ret) {
            pr_err("shtdown_recorder register_reboot_notifier fail");
        }
        else {
            pr_info("shutdown_recorder register_reboot_notifier success");
            shutdown_recorder_enable = true;
        }
    }
    else if ((!val) && (shutdown_recorder_enable == true)) {
        pr_info("shutdown_recorder disable");
        ret = unregister_reboot_notifier(&monitor_reboot_nb);
        if (ret) {
            pr_err("shutdown_recorder unregister_reboot_notifier fail");
        }
        else {
            pr_info("shutdown_recorder unregister_reboot_notifier success");
            shutdown_recorder_enable = false;
        }
    }
    return cnt;
}
static ssize_t shutdown_recorder_show(struct file *file, char __user *buf,
        size_t count,loff_t *off)
{
    char page[64] = {0};
    int len = 0;

    len = sprintf(&page[len], "=== shutdown_recorder_enable:%d ===\n", shutdown_recorder_enable);
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
    static const struct proc_ops shutdown_recorder_fops = {
    .proc_read = shutdown_recorder_show,
    .proc_write = shutdown_recorder_trigger,
    .proc_lseek = seq_lseek,
     };
#else
    struct file_operations last_boot_reason_fops = {
    .write       = last_boot_reason_write,
    .read        = last_boot_reason_read,
     };
    struct file_operations shutdown_recorder_fops = {
    .write       = shutdown_recorder_trigger,
    .read        = shutdown_recorder_show,
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

    pe = proc_create("shutdown_recorder", 0664, NULL, &shutdown_recorder_fops);
    if (!pe) {
        LAST_BOOT_REASON_PRINTK("shutdown_recorder :Failed to register shutdown_recorder interface\n");
        return -ENOMEM;
    }

    return 0;
}

late_initcall(init_last_boot_reason);

#if IS_MODULE(CONFIG_OPLUS_FEATURE_SHUTDOWN_DETECT)
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
#endif
MODULE_LICENSE("GPL v2");

