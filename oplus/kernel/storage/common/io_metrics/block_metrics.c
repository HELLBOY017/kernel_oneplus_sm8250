#include "io_metrics_entry.h"
#include "procfs.h"
#include "block_metrics.h"
#include <trace/events/block.h>

bool block_rq_issue_enabled = false;
bool block_rq_complete_enabled = false;
module_param(block_rq_issue_enabled, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(block_rq_issue_enabled, " Debug block_rq_issue");
module_param(block_rq_complete_enabled, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(block_rq_complete_enabled, " Debug block_rq_complete");

struct blk_metrics_struct blk_metrics[OP_MAX][CYCLE_MAX][IO_SIZE_MAX] = {0};
spinlock_t blk_metrics_lock[OP_MAX][CYCLE_MAX][IO_SIZE_MAX];

static void block_stat_update(struct request *rq, enum io_op_type op_type,
                                                  u64 io_complete_time_ns)
{
    unsigned long flags;
    u64 elapse = 0;
    int i = 0;
    u64 in_driver = (io_complete_time_ns > rq->io_start_time_ns) && rq->io_start_time_ns ?
                    (io_complete_time_ns - rq->io_start_time_ns) : 0;
    u64 in_block = (rq->io_start_time_ns > rq->start_time_ns) && rq->start_time_ns ?
                    (rq->io_start_time_ns - rq->start_time_ns) : 0;
    u64 in_d_and_b = in_driver + in_block;
    enum io_range io_range = IO_SIZE_MAX;
    u32 nr_bytes = blk_rq_bytes(rq);

    if (nr_bytes >= IO_SIZE_512K_TO_MAX_MASK) {/* [512K, +∞) */
        io_range = IO_SIZE_512K_TO_MAX;
    } else if (nr_bytes > IO_SIZE_128K_TO_512K_MASK) {/* (128K, 512K) */
        io_range = IO_SIZE_128K_TO_512K;
    } else if (nr_bytes > IO_SIZE_32K_TO_128K_MASK) {/* (32K, 128K] */
        io_range = IO_SIZE_32K_TO_128K;
    } else if (nr_bytes > IO_SIZE_4K_TO_32K_MASK) {/* (4K, 32K] */
        io_range = IO_SIZE_4K_TO_32K;
    } else {/* (0, 4K] */
        io_range = IO_SIZE_0_TO_4K;
    }

    /* 根据不同时间窗口计算一个采样周期内的平均耗时、最大耗时*/
    for (i = 0; i < CYCLE_MAX; i++) {
        elapse = io_complete_time_ns - blk_metrics[op_type][i][io_range].timestamp;
        /* 统计复位(timestamp为0)、统计异常（timestamp比io_complete_time_ns大） */
        if (unlikely(elapse >= io_complete_time_ns)) {
            flags = 0;
            spin_lock_irqsave(&blk_metrics_lock[op_type][i][io_range], flags);
            blk_metrics[op_type][i][io_range].timestamp = io_complete_time_ns;
            blk_metrics[op_type][i][io_range].total_cnt = 1;
            blk_metrics[op_type][i][io_range].total_size = nr_bytes;
            blk_metrics[op_type][i][io_range].layer[IN_BLOCK].elapse_time = in_block;
            blk_metrics[op_type][i][io_range].layer[IN_DRIVER].elapse_time = in_driver;
            blk_metrics[op_type][i][io_range].layer[IN_BLOCK].max_time = in_block;
            blk_metrics[op_type][i][io_range].layer[IN_DRIVER].max_time = in_driver;
            blk_metrics[op_type][i][io_range].max_time = in_d_and_b;
            spin_unlock_irqrestore(&blk_metrics_lock[op_type][i][io_range], flags);
            elapse = 0;
        } else { /* 没有满足一个采样周期时更新数据 */
            flags = 0;
            spin_lock_irqsave(&blk_metrics_lock[op_type][i][io_range], flags);
            blk_metrics[op_type][i][io_range].total_cnt += 1;
            blk_metrics[op_type][i][io_range].total_size += nr_bytes;
            blk_metrics[op_type][i][io_range].layer[IN_BLOCK].elapse_time += in_block;
            blk_metrics[op_type][i][io_range].layer[IN_DRIVER].elapse_time += in_driver;

            /* 最大值 */
            blk_metrics[op_type][i][io_range].layer[IN_BLOCK].max_time =
               (blk_metrics[op_type][i][io_range].layer[IN_BLOCK].max_time > in_block) ?
               blk_metrics[op_type][i][io_range].layer[IN_BLOCK].max_time : in_block;
            blk_metrics[op_type][i][io_range].layer[IN_DRIVER].max_time =
               (blk_metrics[op_type][i][io_range].layer[IN_DRIVER].max_time > in_driver) ?
               blk_metrics[op_type][i][io_range].layer[IN_DRIVER].max_time : in_driver;
            blk_metrics[op_type][i][io_range].max_time =
               (blk_metrics[op_type][i][io_range].max_time > in_d_and_b) ?
               blk_metrics[op_type][i][io_range].max_time : in_d_and_b;
            spin_unlock_irqrestore(&blk_metrics_lock[op_type][i][io_range], flags);
        }
        if (unlikely(elapse >= sample_cycle_config[i].cycle_value)) {
            /* 过期复位 */
            flags = 0;
            spin_lock_irqsave(&blk_metrics_lock[op_type][i][io_range], flags);
            blk_metrics[op_type][i][io_range].timestamp = 0;
            spin_unlock_irqrestore(&blk_metrics_lock[op_type][i][io_range], flags);
        }
    }
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 11, 0)
static char *blk_get_disk_name(struct gendisk *hd, int partno, char *buf)
{
    if (!partno)
        snprintf(buf, BDEVNAME_SIZE, "%s", hd->disk_name);
    else if (isdigit(hd->disk_name[strlen(hd->disk_name)-1]))
        snprintf(buf, BDEVNAME_SIZE, "%sp%d", hd->disk_name, partno);
    else
        snprintf(buf, BDEVNAME_SIZE, "%s%d", hd->disk_name, partno);

    return buf;
}
#endif

static void blk_fill_rwbs_private(char *rwbs, unsigned int op, int bytes)
{
    int i = 0;
    if (op & REQ_PREFLUSH)
        rwbs[i++] = 'F';
    switch (op & REQ_OP_MASK) {
    case REQ_OP_WRITE:
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
    case REQ_OP_WRITE_SAME:
#endif
        rwbs[i++] = 'W';
        break;
    case REQ_OP_DISCARD:
        rwbs[i++] = 'D';
        break;
    case REQ_OP_SECURE_ERASE:
        rwbs[i++] = 'D';
        rwbs[i++] = 'E';
        break;
    case REQ_OP_FLUSH:
        rwbs[i++] = 'F';
        break;
    case REQ_OP_READ:
        rwbs[i++] = 'R';
        break;
    default:
        rwbs[i++] = 'N';
    }
    if (op & REQ_FUA)
        rwbs[i++] = 'F';
    if (op & REQ_RAHEAD)
        rwbs[i++] = 'A';
    if (op & REQ_SYNC)
        rwbs[i++] = 'S';
    if (op & REQ_META)
        rwbs[i++] = 'M';
    rwbs[i] = '\0';
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 15, 0)
/**
There is no consistency between Google and the Linux community here.
The Linux community only one parameter(request) after 5.10.136, But
Google Revert the Linux community update to the old version
on Android 12-5.10-LTS, which means the item on 5.10
No matter how the small version is upgraded, as long as it is an Android 12-5.10
project, it will have two parameters(request_queue and request)
https://android-review.googlesource.com/c/kernel/common/+/2201398
*/
static void cb_block_rq_issue(void *ignore, struct request_queue *q,
                             struct request *rq)
#else
static void cb_block_rq_issue(void *ignore, struct request *rq)
#endif /* LINUX_VERSION_CODE <= KERNEL_VERSION(5, 10, 136) */
{
    if (unlikely(!io_metrics_enabled)) {
        return ;
    }
    rq->io_start_time_ns = ktime_get_ns();
    if (unlikely(io_metrics_debug_enabled || block_rq_issue_enabled)) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
        char *devname = rq->rq_disk ? rq->rq_disk->disk_name : "";
#else
        char *devname = rq->part ? (rq->part->bd_disk ? rq->part->bd_disk->disk_name : "") : "";
#endif
        char rwbs[RWBS_LEN]={};
        unsigned int nr_bytes = blk_rq_bytes(rq);

        blk_fill_rwbs_private(rwbs, rq->cmd_flags, nr_bytes);

        io_metrics_print("dev:%-6s rwbs:%-4s nr_bytes:%-10d " \
          "start_time_ns:%-16llu io_start_time_ns:%-16llu \n",
          devname, rwbs, nr_bytes, rq->start_time_ns, rq->io_start_time_ns);
    }
}
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
static void cb_block_rq_complete(void *ignore, struct request *rq,
                      int error, unsigned int nr_bytes)
#else
static void cb_block_rq_complete(void *ignore, struct request *rq,
                      blk_status_t error, unsigned int nr_bytes)
#endif
{
    u64 io_complete_time_ns = ktime_get_ns();
    u64 in_driver = (io_complete_time_ns > rq->io_start_time_ns) && rq->io_start_time_ns ?
                    (io_complete_time_ns - rq->io_start_time_ns) : 0;
    u64 in_block = (rq->io_start_time_ns > rq->start_time_ns) && rq->start_time_ns ?
                    (rq->io_start_time_ns - rq->start_time_ns) : 0;

    if (unlikely(!io_metrics_enabled)) {
        return ;
    }

    switch (rq->cmd_flags & REQ_OP_MASK) {
        case REQ_OP_WRITE:
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
        case REQ_OP_WRITE_SAME:
#endif
            if (!error && nr_bytes) {
#if 1
                block_stat_update(rq, OP_WRITE, io_complete_time_ns);
#endif
#if 0
                if (rq->cmd_flags & REQ_SYNC) {
                    block_stat_update(rq, OP_WRITE_SYNC, io_complete_time_ns);
                } else if (rq->cmd_flags & REQ_META) {
                    block_stat_update(rq, OP_WRITE_META, io_complete_time_ns);
                } else {
                    block_stat_update(rq, OP_WRITE, io_complete_time_ns);
                }
#endif
            }
            break;
#if 0
        case REQ_OP_DISCARD:
            if (!error && nr_bytes) {
                block_stat_update(rq, OP_DISCARD, io_complete_time_ns);
            }
            break;
        case REQ_OP_SECURE_ERASE:
            if (!error && nr_bytes) {
                block_stat_update(rq, OP_SECURE_ERASE, io_complete_time_ns);
            }
            break;
        case REQ_OP_FLUSH:
            if (!error && nr_bytes) {
                block_stat_update(rq, OP_FLUSH, io_complete_time_ns);
            }
            break;
#endif
        case REQ_OP_READ:
            if (!error && nr_bytes) {
#if 1
                block_stat_update(rq, OP_READ, io_complete_time_ns);
#endif
#if 0
                if (rq->cmd_flags & REQ_RAHEAD) {
                    block_stat_update(rq, OP_RAHEAD, io_complete_time_ns);
                } else if (rq->cmd_flags & REQ_META) {
                    block_stat_update(rq, OP_READ_META, io_complete_time_ns);
                }else {
                    block_stat_update(rq, OP_READ, io_complete_time_ns);
                }
#endif
            }
            break;
        default:
            break;
    }

    if (unlikely(io_metrics_debug_enabled || block_rq_complete_enabled)) {
        char devname[BDEVNAME_SIZE] = {0};
        char rwbs[RWBS_LEN]={0};

        blk_fill_rwbs_private(rwbs, rq->cmd_flags, nr_bytes);
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 11, 0)
        if (rq->bio && rq->bio->bi_disk) {
            blk_get_disk_name(rq->bio->bi_disk, rq->bio->bi_partno, devname);
#else
        if (rq->bio && rq->bio->bi_bdev) {
        //todo 通过block_device获取设备名称
        //disk_name(rq->bio->bi_disk, rq->bio->bi_partno, devname);
        devname[0] = '\0';
#endif
        } else {
            devname[0] = '\0';
        }

        io_metrics_print("dev:%-6s rwbs:%-4s nr_bytes:%-10d error:%-3d " \
            "start_time_ns:%-16llu io_start_time_ns:%-16llu io_complete_time_ns:%-16llu " \
            "in_driver:%-10llu in_block:%-10llu\n", devname, rwbs, nr_bytes,
            error, rq->start_time_ns, rq->io_start_time_ns, io_complete_time_ns,
            in_driver, in_block);
    }
    return;
}

struct {
    const char *name;
    void *callback;
    struct tracepoint *tp;
    void *data;
} tracepoint_probes[] = {
    {"block_rq_issue", cb_block_rq_issue, NULL, NULL},
    {"block_rq_complete", cb_block_rq_complete, NULL, NULL},
    {NULL, NULL, NULL, NULL}
};

void block_register_tracepoint_probes(void)
{
    int ret;

    ret = register_trace_block_rq_issue(cb_block_rq_issue, NULL);
    ret = register_trace_block_rq_complete(cb_block_rq_complete, NULL);

    return;
}

void block_unregister_tracepoint_probes(void)
{
    unregister_trace_block_rq_issue(cb_block_rq_issue, NULL);
    unregister_trace_block_rq_complete(cb_block_rq_complete, NULL);

    return;
}

struct {
    enum io_op_type value;
    const char * tag;
} io_op_config[] = {
    {OP_READ,         "read"      },
    {OP_WRITE,        "write"     },
#if 0
    {OP_RAHEAD,       "rahead"    },
    {OP_WRITE_SYNC,   "write_sync"},
    {OP_READ_META,    "read_meta" },
    {OP_WRITE_META,   "write_meta"},
    {OP_DISCARD,      "discard"   },
    {OP_SECURE_ERASE, "erase"     },
    {OP_FLUSH,        "flush"     },
#endif
    {OP_MAX,          NULL        },
};

/*当前函数理论每个node一天只需要访问一次，因此可以不用太考虑性能，只关注代码紧凑性*/
static int block_metrics_proc_show(struct seq_file *seq_filp, void *data)
{
    int i = 0;
    enum io_op_type io_op;
    u64 value = 0;
    enum sample_cycle_type cycle;
    struct file *file = (struct file *)seq_filp->private;

    if (unlikely(!io_metrics_enabled)) {
        seq_printf(seq_filp, "io_metrics_enabled not set to 1:%d\n", io_metrics_enabled);
        return 0;
    }
    if (proc_show_enabled ||unlikely(io_metrics_debug_enabled)) {
        io_metrics_print("%s(%d) read %s/%s\n",
            current->comm, current->pid, file->f_path.dentry->d_parent->d_iname,
            file->f_path.dentry->d_iname);
    }
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

    /* 确定读、写操作命令 */
    io_op = OP_MAX;
    for (i = 0; i < OP_MAX; i++) {
        if (strstr(file->f_path.dentry->d_iname, io_op_config[i].tag)) {
            io_op = io_op_config[i].value;
            break;
        }
    }
    if (unlikely(io_op == OP_MAX)) {
        goto err;
    }
    if (OP_MAX == OP_READ) {
        goto bio_read;
    } else if (OP_MAX == OP_WRITE) {
         goto bio_write;
    }

    /* 确定读的具体是那个节点 */
bio_read:
    if (!strcmp(file->f_path.dentry->d_iname, "bio_read_cnt")) {
        value = 0;
        for (i = 0; i < IO_SIZE_MAX; i++) {
            value += blk_metrics[OP_READ][cycle][i].total_cnt;
        }
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_read_avg_size")) {
        unsigned long flags = 0;
        u64 total_size = 0;
        u64 total_cnt = 0;
        value = 0;
        for (i = 0; i < IO_SIZE_MAX; i++) {
            flags = 0;
            spin_lock_irqsave(&blk_metrics_lock[OP_READ][cycle][i], flags);
            total_size += blk_metrics[OP_READ][cycle][i].total_size;
            total_cnt += blk_metrics[OP_READ][cycle][i].total_cnt;
            spin_unlock_irqrestore(&blk_metrics_lock[OP_READ][cycle][i], flags);
        }
        value = total_size / total_cnt;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_read_size_dist")) {
        for (i = 0; i < IO_SIZE_MAX; i++) {
            seq_printf(seq_filp, "%llu,", blk_metrics[OP_READ][cycle][i].total_cnt);
        }
        seq_printf(seq_filp, "\n");
        return 0;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_read_avg_time")) {
        u64 total_time = 0;
        u64 total_cnt = 0;
        value = 0;
        for (i = 0; i < IO_SIZE_MAX; i++) {
            total_time += blk_metrics[OP_READ][cycle][i].layer[IN_BLOCK].elapse_time;
            total_time += blk_metrics[OP_READ][cycle][i].layer[IN_DRIVER].elapse_time;
            total_cnt += blk_metrics[OP_READ][cycle][i].total_cnt;
        }
        value = total_time / total_cnt;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_read_max_time")) {
        value = 0;
        for (i = 0; i < IO_SIZE_MAX; i++) {
            value = (value > blk_metrics[OP_READ][cycle][i].max_time) ?
                      value : blk_metrics[OP_READ][cycle][i].max_time;
        }
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_read_4k_blk_avg_time")) {
        value = blk_metrics[OP_READ][cycle][IO_SIZE_0_TO_4K].layer[IN_BLOCK].elapse_time /
                blk_metrics[OP_READ][cycle][IO_SIZE_0_TO_4K].total_cnt;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_read_4k_blk_max_time")) {
        value = blk_metrics[OP_READ][cycle][IO_SIZE_0_TO_4K].layer[IN_BLOCK].max_time;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_read_4k_drv_avg_time")) {
        value = blk_metrics[OP_READ][cycle][IO_SIZE_0_TO_4K].layer[IN_DRIVER].elapse_time /
                blk_metrics[OP_READ][cycle][IO_SIZE_0_TO_4K].total_cnt;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_read_4k_drv_max_time")) {
        value = blk_metrics[OP_READ][cycle][IO_SIZE_0_TO_4K].layer[IN_DRIVER].max_time;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_read_512k_blk_avg_time")) {
        value = blk_metrics[OP_READ][cycle][IO_SIZE_128K_TO_512K].layer[IN_BLOCK].elapse_time /
                blk_metrics[OP_READ][cycle][IO_SIZE_128K_TO_512K].total_cnt;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_read_512k_blk_max_time")) {
        value = blk_metrics[OP_READ][cycle][IO_SIZE_128K_TO_512K].layer[IN_BLOCK].max_time;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_read_512k_drv_avg_time")) {
        value = blk_metrics[OP_READ][cycle][IO_SIZE_128K_TO_512K].layer[IN_DRIVER].elapse_time /
                blk_metrics[OP_READ][cycle][IO_SIZE_128K_TO_512K].total_cnt;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_read_512k_drv_max_time")) {
        value = blk_metrics[OP_READ][cycle][IO_SIZE_128K_TO_512K].layer[IN_DRIVER].max_time;
    }

bio_write:
    if (!strcmp(file->f_path.dentry->d_iname, "bio_write_cnt")) {
        value = 0;
        for (i = 0; i < IO_SIZE_MAX; i++) {
            value += blk_metrics[OP_WRITE][cycle][i].total_cnt;
        }
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_write_avg_size")) {
        unsigned long flags = 0;
        u64 total_size = 0;
        u64 total_cnt = 0;
        value = 0;
        for (i = 0; i < IO_SIZE_MAX; i++) {
            flags = 0;
            spin_lock_irqsave(&blk_metrics_lock[OP_WRITE][cycle][i], flags);
            total_size += blk_metrics[OP_WRITE][cycle][i].total_size;
            total_cnt += blk_metrics[OP_WRITE][cycle][i].total_cnt;
            spin_unlock_irqrestore(&blk_metrics_lock[OP_WRITE][cycle][i], flags);
        }
        value = total_size / total_cnt;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_write_size_dist")) {
        for (i = 0; i < IO_SIZE_MAX; i++) {
            seq_printf(seq_filp, "%llu,", blk_metrics[OP_WRITE][cycle][i].total_cnt);
        }
        seq_printf(seq_filp, "\n");
        return 0;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_write_avg_time")) {
        u64 total_time = 0;
        u64 total_cnt = 0;
        value = 0;
        for (i = 0; i < IO_SIZE_MAX; i++) {
            total_time += blk_metrics[OP_WRITE][cycle][i].layer[IN_BLOCK].elapse_time;
            total_time += blk_metrics[OP_WRITE][cycle][i].layer[IN_DRIVER].elapse_time;
            total_cnt += blk_metrics[OP_WRITE][cycle][i].total_cnt;
        }
        value = total_time / total_cnt;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_write_max_time")) {
        value = 0;
        for (i = 0; i < IO_SIZE_MAX; i++) {
            value = (value > blk_metrics[OP_WRITE][cycle][i].max_time) ?
                      value : blk_metrics[OP_WRITE][cycle][i].max_time;
        }
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_write_4k_blk_avg_time")) {
        value = blk_metrics[OP_WRITE][cycle][IO_SIZE_0_TO_4K].layer[IN_BLOCK].elapse_time /
                blk_metrics[OP_WRITE][cycle][IO_SIZE_0_TO_4K].total_cnt;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_write_4k_blk_max_time")) {
        value = blk_metrics[OP_WRITE][cycle][IO_SIZE_0_TO_4K].layer[IN_BLOCK].max_time;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_write_4k_drv_avg_time")) {
        value = blk_metrics[OP_WRITE][cycle][IO_SIZE_0_TO_4K].layer[IN_DRIVER].elapse_time /
                blk_metrics[OP_WRITE][cycle][IO_SIZE_0_TO_4K].total_cnt;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_write_4k_drv_max_time")) {
        value = blk_metrics[OP_WRITE][cycle][IO_SIZE_0_TO_4K].layer[IN_DRIVER].max_time;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_write_512k_blk_avg_time")) {
        value = blk_metrics[OP_WRITE][cycle][IO_SIZE_128K_TO_512K].layer[IN_BLOCK].elapse_time /
                blk_metrics[OP_WRITE][cycle][IO_SIZE_128K_TO_512K].total_cnt;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_write_512k_blk_max_time")) {
        value = blk_metrics[OP_WRITE][cycle][IO_SIZE_128K_TO_512K].layer[IN_BLOCK].max_time;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_write_512k_drv_avg_time")) {
        value = blk_metrics[OP_WRITE][cycle][IO_SIZE_128K_TO_512K].layer[IN_DRIVER].elapse_time /
                blk_metrics[OP_WRITE][cycle][IO_SIZE_128K_TO_512K].total_cnt;
    } else if (!strcmp(file->f_path.dentry->d_iname, "bio_write_512k_drv_max_time")) {
        value = blk_metrics[OP_WRITE][cycle][IO_SIZE_128K_TO_512K].layer[IN_DRIVER].max_time;
    }

    seq_printf(seq_filp, "%llu\n", value);

    return 0;

err:
    io_metrics_print("%s(%d) I don't understand what the operation: %s/%s\n",
    current->comm,current->pid,
    file->f_path.dentry->d_parent->d_iname, file->f_path.dentry->d_iname);
    return -1;
}

int block_metrics_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, block_metrics_proc_show, file);
}

void block_metrics_reset(void)
{
    /* 此处可以优化，如果blk_metrics_struct中有锁成员时 */
    memset(blk_metrics, 0, OP_MAX * CYCLE_MAX * IO_SIZE_MAX
                         * sizeof(struct blk_metrics_struct));
    io_metrics_print("size:%lu\n", OP_MAX * CYCLE_MAX * IO_SIZE_MAX
                              * sizeof(struct blk_metrics_struct));
}

void block_metrics_init(void)
{
    int i, j, k;

    block_metrics_reset();
    for (i = 0; i < OP_MAX; i++) {
        for (j = 0; j < CYCLE_MAX; j++) {
            for (k = 0; k < IO_SIZE_MAX; k++) {
                spin_lock_init(&blk_metrics_lock[i][j][k]);
            }
        }
    }
}
