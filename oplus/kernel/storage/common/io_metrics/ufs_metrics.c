#include "io_metrics_entry.h"
#include "procfs.h"
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
#include "drivers/scsi/ufs/ufshcd.h"
#else
#include <scsi/scsi_cmnd.h>
#include <ufs/ufshcd.h>
#endif
#include "ufs_metrics.h"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
#include <trace/hooks/ufshcd.h>
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
#include <trace/hooks/oplus_ufs.h>
#endif

bool ufs_compl_command_enabled = false;
module_param(ufs_compl_command_enabled, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ufs_compl_command_enabled, " Debug android_vh_ufs_compl_command");

atomic64_t ufs_metrics_timestamp[CYCLE_MAX];

struct {
    atomic64_t read_size;
    atomic64_t read_cnt;
    atomic64_t read_elapse;
    atomic64_t write_size;
    atomic64_t write_cnt;
    atomic64_t write_elapse;
    char padding[16];
} ufs_metrics[CYCLE_MAX] = {0};

void cb_android_vh_ufs_compl_command(void *ignore, struct ufs_hba *hba,
                                     struct ufshcd_lrb *lrbp)
{
    ktime_t elapsed_in_ufs;
    int transfer_len = 0;

    if (unlikely(!io_metrics_enabled)) {
        return ;
    }
    if (!lrbp->cmd) {
        goto exit;
    }
    /*complete*/
    elapsed_in_ufs = lrbp->compl_time_stamp - lrbp->issue_time_stamp;
    switch(lrbp->cmd->cmnd[0]) {
        case READ_6:
        case READ_10:
        case READ_16:
        {
            int i;
            u64 current_time_ns, elapse;
            transfer_len = be32_to_cpu(lrbp->ucd_req_ptr->sc.exp_data_transfer_len);
            current_time_ns = lrbp->compl_time_stamp;
            for (i = 0; i < CYCLE_MAX; i++) {
                elapse = current_time_ns - atomic64_read(&ufs_metrics_timestamp[i]);
                if (unlikely(elapse >= current_time_ns)) {
                    atomic64_set(&ufs_metrics_timestamp[i], current_time_ns);
                    atomic64_set(&ufs_metrics[i].read_cnt, 1);
                    atomic64_set(&ufs_metrics[i].read_size, transfer_len);
                    atomic64_set(&ufs_metrics[i].read_elapse, elapsed_in_ufs);
                    elapse = 0;
                } else {
                    atomic64_inc(&ufs_metrics[i].read_cnt);
                    atomic64_add(transfer_len, &ufs_metrics[i].read_size);
                    atomic64_add(elapsed_in_ufs, &ufs_metrics[i].read_elapse);
                }
                if (unlikely(elapse >= sample_cycle_config[i].cycle_value)) {
                    /* 过期复位 */
                    atomic64_set(&ufs_metrics_timestamp[i], 0);
                }
            }
            if (unlikely(ufs_compl_command_enabled || io_metrics_debug_enabled)) {
                io_metrics_print("read %d bytes cost %llu ns\n",
                                 transfer_len, elapsed_in_ufs);
            }
            break;
        }
        case WRITE_6:
        case WRITE_10:
        case WRITE_16:
        {
            int i;
            u64 current_time_ns, elapse;
            transfer_len = be32_to_cpu(lrbp->ucd_req_ptr->sc.exp_data_transfer_len);
            current_time_ns = lrbp->compl_time_stamp;
            for (i = 0; i < CYCLE_MAX; i++) {
                elapse = current_time_ns - atomic64_read(&ufs_metrics_timestamp[i]);
                if (unlikely(elapse >= current_time_ns)) {
                    atomic64_set(&ufs_metrics_timestamp[i], current_time_ns);
                    atomic64_set(&ufs_metrics[i].write_cnt, 1);
                    atomic64_set(&ufs_metrics[i].write_size, transfer_len);
                    atomic64_set(&ufs_metrics[i].write_elapse, elapsed_in_ufs);
                    elapse = 0;
                } else {
                    atomic64_inc(&ufs_metrics[i].write_cnt);
                    atomic64_add(transfer_len, &ufs_metrics[i].write_size);
                    atomic64_add(elapsed_in_ufs, &ufs_metrics[i].write_elapse);
                }
                if (unlikely(elapse >= sample_cycle_config[i].cycle_value)) {
                    /* 过期复位 */
                    atomic64_set(&ufs_metrics_timestamp[i], 0);
                }
            }
            if (unlikely(ufs_compl_command_enabled || io_metrics_debug_enabled)) {
                io_metrics_print("write %d bytes cost %llu ns\n",
                                 transfer_len, elapsed_in_ufs);
            }
            break;
        }
        case UNMAP:
        {
            if (unlikely(ufs_compl_command_enabled || io_metrics_debug_enabled)) {
                transfer_len = be32_to_cpu(lrbp->ucd_req_ptr->sc.exp_data_transfer_len);
                io_metrics_print("unmap %d bytes cost %llu ns\n",
                                 transfer_len, elapsed_in_ufs);
            }
            break;
        }
        default:
            goto exit;
    }

exit:
    return;
}

void ufs_register_tracepoint_probes(void)
{
    int ret = 0;
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 4, 0)
    ret = register_trace_android_vh_ufs_compl_command(cb_android_vh_ufs_compl_command, NULL);
    WARN_ON(ret);
#endif
    io_metrics_print("run:%d\n", ret);
}

void ufs_unregister_tracepoint_probes(void)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 4, 0)
    unregister_trace_android_vh_ufs_compl_command(cb_android_vh_ufs_compl_command, NULL);
#endif
    return;
}

static int ufs_metrics_proc_show(struct seq_file *seq_filp, void *data)
{
    u64 value = 123;
    struct file *file = (struct file *)seq_filp->private;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
    int i = 0;
    enum sample_cycle_type cycle;
#endif

    if (unlikely(!io_metrics_enabled)) {
        seq_printf(seq_filp, "io_metrics_enabled not set to 1:%d\n", io_metrics_enabled);
        return 0;
    }

    if (proc_show_enabled || unlikely(io_metrics_debug_enabled)) {
        io_metrics_print("%s(%d) read %s/%s\n",
            current->comm, current->pid, file->f_path.dentry->d_parent->d_iname,
            file->f_path.dentry->d_iname);
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
    /* 确定采样周期的值（父目录） */
    cycle = CYCLE_MAX;
    for (i = 0; i < CYCLE_MAX; i++) {
        if(!strcmp(file->f_path.dentry->d_parent->d_iname, sample_cycle_config[i].tag)) {
            cycle = sample_cycle_config[i].value;
        }
    }
    if (unlikely(cycle == CYCLE_MAX)) {
        goto err;
    }
    if(!strcmp(file->f_path.dentry->d_iname, "ufs_total_read_size_mb")) {
        value = atomic64_read(&ufs_metrics[cycle].read_size) >> 20;
    } else if (!strcmp(file->f_path.dentry->d_iname, "ufs_total_read_time_ms")) {
        /*1ns=1/(1000*1000)ms≈1/(1024*1024)ms=1>>20ms,Precision=95.1%*/
        value = atomic64_read(&ufs_metrics[cycle].read_elapse) >> 20;
    } else if (!strcmp(file->f_path.dentry->d_iname, "ufs_total_write_size_mb")) {
        value = atomic64_read(&ufs_metrics[cycle].write_size) >> 20;;
    } else if (!strcmp(file->f_path.dentry->d_iname, "ufs_total_write_time_ms")) {
        value = atomic64_read(&ufs_metrics[cycle].write_elapse) >> 20;
    }
#else
    value = 0;
#endif
    seq_printf(seq_filp, "%llu\n", value);

    return 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
err:
    io_metrics_print("%s(%d) I don't understand what the operation: %s/%s\n",
             current->comm,current->pid, file->f_path.dentry->d_parent->d_iname,
             file->f_path.dentry->d_iname);
    return -1;
#endif
}

int ufs_metrics_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, ufs_metrics_proc_show, file);
}

void ufs_metrics_reset(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
    int i = 0;

    for (i = 0; i < CYCLE_MAX; i++) {
        atomic64_set(&ufs_metrics_timestamp[i], 0);
        atomic64_set(&ufs_metrics[i].read_size, 0);
        atomic64_set(&ufs_metrics[i].read_cnt, 0);
        atomic64_set(&ufs_metrics[i].read_elapse, 0);
        atomic64_set(&ufs_metrics[i].write_size, 0);
        atomic64_set(&ufs_metrics[i].write_cnt, 0);
        atomic64_set(&ufs_metrics[i].write_elapse, 0);
    }
#else
    return;
#endif
}
void ufs_metrics_init(void)
{
    ufs_metrics_reset();
}