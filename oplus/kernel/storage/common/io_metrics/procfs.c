#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/dcache.h>

#include "procfs.h"
#include "block_metrics.h"
#include "f2fs_metrics.h"
#include "ufs_metrics.h"
#include "abnormal_io.h"

#define STORAGE_DIR_NODE "oplus_storage"
#define IO_METRICS_DIR_NODE "io_metrics"
#define IO_METRICS_CONTROL_DIR_NODE "control"
#define DUMP_PATH_LEN 1024
static char abnormal_io_dump_path[DUMP_PATH_LEN];
bool proc_show_enabled = true;
module_param(proc_show_enabled, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(proc_show_enabled, " Debug proc");

static struct proc_dir_entry *storage_procfs;
static struct proc_dir_entry *io_metrics_procfs;
static struct proc_dir_entry *io_metrics_control_procfs;
static struct proc_dir_entry *sample_dir[CYCLE_MAX] = {0};

struct sample_cycle sample_cycle_config[] = {
//    { CYCLE_SECOND_10,  "second_10", 10000000000        }, /* 10秒均值 */
//    { CYCLE_MINUTES_1,  "minutes_1", 60000000000        }, /* 1分钟均值 */
//    {CYCLE_MINUTES_10, "minutes_10", 600000000000       }, /* 10分钟均值 */
//    {    CYCLE_HOUR_1,       "hour", 3600000000000      }, /* 1小时均值 */
//    {     CYCLE_DAY_1,        "day", 86400000000000     }, /* 1天均值 */
//    {    CYCLE_WEEK_1,       "week", 604800000000000    }, /* 1周均值 */
    {   CYCLE_FOREVER,    "forever", 3153600000000000000}, /* 总流量 */
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
static const struct proc_ops block_metrics_proc_fops = {
    .proc_open      = block_metrics_proc_open,
    .proc_read      = seq_read,
    .proc_lseek     = seq_lseek,
    .proc_release   = single_release,
};

static const struct proc_ops f2fs_metrics_proc_fops = {
    .proc_open      = f2fs_metrics_proc_open,
    .proc_read      = seq_read,
    .proc_lseek     = seq_lseek,
    .proc_release   = single_release,
};

static const struct proc_ops ufs_metrics_proc_fops = {
    .proc_open      = ufs_metrics_proc_open,
    .proc_read      = seq_read,
    .proc_lseek     = seq_lseek,
    .proc_release   = single_release,
};
#else
static const struct file_operations block_metrics_proc_fops = {
    .open      = block_metrics_proc_open,
    .read      = seq_read,
    .llseek     = seq_lseek,
    .release   = single_release,
};

static const struct file_operations f2fs_metrics_proc_fops = {
    .open      = f2fs_metrics_proc_open,
    .read      = seq_read,
    .llseek     = seq_lseek,
    .release   = single_release,
};

static const struct file_operations ufs_metrics_proc_fops = {
    .open      = ufs_metrics_proc_open,
    .read      = seq_read,
    .llseek     = seq_lseek,
    .release   = single_release,
};
#endif

static int io_metrics_control_show(struct seq_file *seq_filp, void *data)
{
    struct file *file = (struct file *)seq_filp->private;

    if (proc_show_enabled || unlikely(io_metrics_debug_enabled)) {
        io_metrics_print("%s(%d) read %s/%s\n",
            current->comm, current->pid, file->f_path.dentry->d_parent->d_iname,
            file->f_path.dentry->d_iname);
    }

    if (!strcmp(file->f_path.dentry->d_iname, "enable")) {
       seq_printf(seq_filp, "%d\n", io_metrics_enabled);
    } else if (!strcmp(file->f_path.dentry->d_iname, "debug_enable")) {
        seq_printf(seq_filp, "%d\n", io_metrics_debug_enabled);
    } else if (!strcmp(file->f_path.dentry->d_iname, "abnormal_io_enabled")) {
        seq_printf(seq_filp, "%d\n", atomic_read(&abnormal_io_enabled));
    } else if (!strcmp(file->f_path.dentry->d_iname, "abnormal_io_trigger")) {
        seq_printf(seq_filp, "%d\n", abnormal_io_trigger);
    } else if (!strcmp(file->f_path.dentry->d_iname, "abnormal_io_dump_min_interval_s")) {
        seq_printf(seq_filp, "%d\n", abnormal_io_dump_min_interval_s);
    } else if (!strcmp(file->f_path.dentry->d_iname, "abnormal_io_dump_limit_1_day")) {
        seq_printf(seq_filp, "%d\n", abnormal_io_dump_limit_1_day);
    } else if (!strcmp(file->f_path.dentry->d_iname, "abnormal_io_dump_path")) {
        abnormal_io_dump_path[DUMP_PATH_LEN - 1] = '\0';
        seq_printf(seq_filp, "%s\n", abnormal_io_dump_path);
    }

    return 0;
}

static ssize_t io_metrics_control_write(struct file *file,
                       const char __user *buf, size_t len, loff_t *ppos)
{
    enum  {
        INT_VALUE = 0,
        BOOL_VALUE,
        STR_VALUE
    } val_t = INT_VALUE;
#define DATA_LEN    DUMP_PATH_LEN
    char buffer[DATA_LEN+1] = {0};
    int ret = 0;
    int value = 0;

    len = (len > DATA_LEN) ? DATA_LEN : len;
    //kstrtoint_from_user
    if (copy_from_user(buffer, buf, len)) {
        return -EFAULT;
    }
    buffer[len] = '\0';

    if (!strcmp(file->f_path.dentry->d_iname, "enable")) {
        ret = kstrtoint(strstrip(buffer), len, &value);
        if (ret) {
            return ret;
        }
        val_t = INT_VALUE;
        WRITE_ONCE(io_metrics_enabled, value);
    } else if (!strcmp(file->f_path.dentry->d_iname, "debug_enable")) {
        ret = kstrtoint(strstrip(buffer), len, &value);
        if (ret) {
            return ret;
        }
        val_t = INT_VALUE;
        WRITE_ONCE(io_metrics_debug_enabled, value);
    } else if (!strcmp(file->f_path.dentry->d_iname, "reset_stat")) {
        ret = kstrtoint(strstrip(buffer), len, &value);
        if (ret) {
            return ret;
        }
        val_t = INT_VALUE;
        if (value == 1) {
            io_metrics_print("reset_stat start\n");
            f2fs_metrics_reset();
            block_metrics_reset();
            ufs_metrics_reset();
        }
    } else if (!strcmp(file->f_path.dentry->d_iname, "abnormal_io_enabled")) {
        ret = kstrtoint(strstrip(buffer), len, &value);
        if (ret) {
            return ret;
        }
        val_t = INT_VALUE;
        if (0 == value) {
            abnormal_io_exit();
        } else if (1 == value) {
            abnormal_io_init();
        }
    } else if (!strcmp(file->f_path.dentry->d_iname, "abnormal_io_trigger")) {
        ret = kstrtoint(strstrip(buffer), len, &value);
        if (ret) {
            return ret;
        }
        val_t = INT_VALUE;
        if (value == 1) {
            ret = abnormal_io_dump_to_file("/data/persist_log/DCS/de/storage/storage_io.hex");
            if (!ret) {
                io_metrics_print("abnormal_io_dump_to_file err(%d)\n", ret);
            }
        }
    } else if (!strcmp(file->f_path.dentry->d_iname, "abnormal_io_dump_min_interval_s")) {
        ret = kstrtoint(strstrip(buffer), len, &value);
        if (ret) {
            return ret;
        }
        val_t = INT_VALUE;
        WRITE_ONCE(abnormal_io_dump_min_interval_s, value);
    } else if (!strcmp(file->f_path.dentry->d_iname, "abnormal_io_dump_limit_1_day")) {
        ret = kstrtoint(strstrip(buffer), len, &value);
        if (ret) {
            return ret;
        }
        val_t = INT_VALUE;
        WRITE_ONCE(abnormal_io_dump_limit_1_day, value);
    } else if (!strcmp(file->f_path.dentry->d_iname, "abnormal_io_dump_path")) {
        val_t = STR_VALUE;
        if (strncpy(abnormal_io_dump_path, strstrip(buffer), len)) {
            if (strcat(abnormal_io_dump_path, "/storage_io.hex")) {
                ret = abnormal_io_dump_to_file((const char *)abnormal_io_dump_path);
                if (!ret) {
                    io_metrics_print("abnormal_io_dump_to_file err(%d)\n", ret);
                }
            }
        }
    }
    /* update log */
    if (val_t == INT_VALUE) {
        io_metrics_print("%s(%d) write %d to %s/%s\n", current->comm, current->pid, value,
                     file->f_path.dentry->d_parent->d_iname, file->f_path.dentry->d_iname);
    } else if (val_t == STR_VALUE) {
        io_metrics_print("%s(%d) write %s to %s/%s\n", current->comm, current->pid, buffer,
                     file->f_path.dentry->d_parent->d_iname, file->f_path.dentry->d_iname);
    }
    *ppos += len;

    return len;
}

int io_metrics_control_open(struct inode *inode, struct file *file)
{
    return single_open(file, io_metrics_control_show, file);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
static const struct proc_ops io_metrics_control_proc_fops = {
    .proc_open      = io_metrics_control_open,
    .proc_read      = seq_read,
    .proc_write     = io_metrics_control_write,
    .proc_lseek     = seq_lseek,
    .proc_release   = single_release,
};
#else
static const struct file_operations io_metrics_control_proc_fops = {
    .open      = io_metrics_control_open,
    .read      = seq_read,
    .write     = io_metrics_control_write,
    .llseek     = seq_lseek,
    .release   = single_release,
};
#endif

enum node_type {
    F2FS = 0,
    BLOCK,
    UFS,
    CONTROL,
};

struct {
    const char *name;
    enum node_type node_type;
    umode_t mode;
} io_metrics_procfs_node[] = {
    /* filesystem layer */
    {"f2fs_discard_cnt",             F2FS, S_IRUGO},
    {"f2fs_fg_gc_cnt",               F2FS, S_IRUGO},
    {"f2fs_fg_gc_avg_time",          F2FS, S_IRUGO},
    {"f2fs_fg_gc_seg_cnt",           F2FS, S_IRUGO},
    {"f2fs_bg_gc_cnt",               F2FS, S_IRUGO},
    {"f2fs_bg_gc_avg_time",          F2FS, S_IRUGO},
    {"f2fs_bg_gc_seg_cnt",           F2FS, S_IRUGO},
    {"f2fs_cp_cnt",                  F2FS, S_IRUGO},
    {"f2fs_cp_avg_time",             F2FS, S_IRUGO},
    {"f2fs_cp_max_time",             F2FS, S_IRUGO},
    {"f2fs_ipu_cnt",                 F2FS, S_IRUGO},
    {"f2fs_fsync_cnt",                F2FS, S_IRUGO},
    /* block layer */
    {"bio_read_cnt",                BLOCK, S_IRUGO},
    {"bio_read_avg_size",           BLOCK, S_IRUGO},
    {"bio_read_size_dist",          BLOCK, S_IRUGO},
    {"bio_read_avg_time",           BLOCK, S_IRUGO},
    {"bio_read_max_time",           BLOCK, S_IRUGO},
    {"bio_read_4k_blk_avg_time",    BLOCK, S_IRUGO},
    {"bio_read_4k_blk_max_time",    BLOCK, S_IRUGO},
    {"bio_read_4k_drv_avg_time",    BLOCK, S_IRUGO},
    {"bio_read_4k_drv_max_time",    BLOCK, S_IRUGO},
    {"bio_read_512k_blk_avg_time",  BLOCK, S_IRUGO},
    {"bio_read_512k_blk_max_time",  BLOCK, S_IRUGO},
    {"bio_read_512k_drv_avg_time",  BLOCK, S_IRUGO},
    {"bio_read_512k_drv_max_time",  BLOCK, S_IRUGO},
    {"bio_write_cnt",               BLOCK, S_IRUGO},
    {"bio_write_avg_size",          BLOCK, S_IRUGO},
    {"bio_write_size_dist",         BLOCK, S_IRUGO},
    {"bio_write_avg_time",          BLOCK, S_IRUGO},
    {"bio_write_max_time",          BLOCK, S_IRUGO},
    {"bio_write_4k_blk_avg_time",   BLOCK, S_IRUGO},
    {"bio_write_4k_blk_max_time",   BLOCK, S_IRUGO},
    {"bio_write_4k_drv_avg_time",   BLOCK, S_IRUGO},
    {"bio_write_4k_drv_max_time",   BLOCK, S_IRUGO},
    {"bio_write_512k_blk_avg_time", BLOCK, S_IRUGO},
    {"bio_write_512k_blk_max_time", BLOCK, S_IRUGO},
    {"bio_write_512k_drv_avg_time", BLOCK, S_IRUGO},
    {"bio_write_512k_drv_max_time", BLOCK, S_IRUGO},
    /* ufs layer */
    {"ufs_total_read_size_mb",        UFS, S_IRUGO},
    {"ufs_total_read_time_ms",        UFS, S_IRUGO},
    {"ufs_total_write_size_mb",       UFS, S_IRUGO},
    {"ufs_total_write_time_ms",       UFS, S_IRUGO},
    /* control */
    {"enable",                    CONTROL, S_IRUGO | S_IWUGO},
    {"debug_enable",              CONTROL, S_IRUGO | S_IWUGO},
    {"reset_stat",                CONTROL, S_IRUGO | S_IWUGO},
    {"abnormal_io_enabled",       CONTROL, S_IRUGO | S_IWUGO},
    {"abnormal_io_trigger",       CONTROL, S_IRUGO | S_IWUGO},
    {"abnormal_io_dump_min_interval_s", CONTROL, S_IRUGO | S_IWUGO},
    {"abnormal_io_dump_limit_1_day",        CONTROL, S_IRUGO | S_IWUGO},
    {"abnormal_io_dump_path",               CONTROL, S_IRUGO | S_IWUGO},
    {NULL,                               0,                 0}
};

int io_metrics_procfs_init(void)
{
    int i = 0;
    int j = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
    struct proc_ops *proc_ops = NULL;
#else
    struct file_operations *proc_ops = NULL;
#endif

    struct proc_dir_entry *pnode;
    /* /proc/oplus_storage */
    storage_procfs = proc_mkdir(STORAGE_DIR_NODE, NULL);
    if (!storage_procfs) {
        io_metrics_print("Can't create procfs node\n");
        goto error_out;
    }
    /* /proc/oplus_storage/io_metrics */
    io_metrics_procfs = proc_mkdir(IO_METRICS_DIR_NODE, storage_procfs);
    if (!io_metrics_procfs) {
        io_metrics_print("Can't create procfs node\n");
        goto error_out;
    }
    /* /proc/oplus_storage/io_metrics/control */
    io_metrics_control_procfs = proc_mkdir(IO_METRICS_CONTROL_DIR_NODE, io_metrics_procfs);
    if (!io_metrics_control_procfs) {
        io_metrics_print("Can't create procfs node\n");
        goto error_out;
    }
    /* 在/proc/oplus_storage/io_metrics下面创建按照周期统计的目录 */
    for (i = 0; i < CYCLE_MAX; i++) {
        sample_dir[i] = proc_mkdir(sample_cycle_config[i].tag, io_metrics_procfs);
        if (!sample_dir[i]) {
            io_metrics_print("Can't create procfs node\n");
            goto error_out;
        }
    }
    for (i = 0; io_metrics_procfs_node[i].name; i++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
        if (io_metrics_procfs_node[i].node_type == F2FS) {
            proc_ops = (struct proc_ops *)&f2fs_metrics_proc_fops;
        } else if (io_metrics_procfs_node[i].node_type == BLOCK) {
            proc_ops = (struct proc_ops *)&block_metrics_proc_fops;
        } else if (io_metrics_procfs_node[i].node_type == UFS) {
            proc_ops = (struct proc_ops *)&ufs_metrics_proc_fops;
        }
#else
        if (io_metrics_procfs_node[i].node_type == F2FS) {
            proc_ops = (struct file_operations *)&f2fs_metrics_proc_fops;
        } else if (io_metrics_procfs_node[i].node_type == BLOCK) {
            proc_ops = (struct file_operations *)&block_metrics_proc_fops;
        } else if (io_metrics_procfs_node[i].node_type == UFS) {
            proc_ops = (struct file_operations *)&ufs_metrics_proc_fops;
        }
#endif
        if (io_metrics_procfs_node[i].node_type == CONTROL) {
            pnode = proc_create(io_metrics_procfs_node[i].name,
                                io_metrics_procfs_node[i].mode,
                                io_metrics_control_procfs,
                                &io_metrics_control_proc_fops);
            if (!pnode) {
                io_metrics_print("Can't create %s\n", io_metrics_procfs_node[i].name);
                goto error_out;
            }
        } else {
            for (j = 0; j < CYCLE_MAX; j++) {
                pnode = proc_create(io_metrics_procfs_node[i].name,
                                    io_metrics_procfs_node[i].mode,
                                    sample_dir[j],
                                    proc_ops);
                if (!pnode) {
                    io_metrics_print("Can't create %s\n", io_metrics_procfs_node[i].name);
                    goto error_out;
                }
            }
        }
    }

    return 0;

error_out:
    return -1;
}
void io_metrics_procfs_exit(void)
{
    if (storage_procfs) {
        if (io_metrics_procfs) {
            remove_proc_entry(IO_METRICS_DIR_NODE, storage_procfs);
        }
        remove_proc_entry(STORAGE_DIR_NODE, NULL);
    }
}