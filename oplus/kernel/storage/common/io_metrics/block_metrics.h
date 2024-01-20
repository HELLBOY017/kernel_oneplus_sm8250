#ifndef __BLOCK_METRICS_H__
#define __BLOCK_METRICS_H__

#include <linux/fs.h>

#define IO_SIZE_4K_TO_32K_MASK       4096
#define IO_SIZE_32K_TO_128K_MASK     32768
#define IO_SIZE_128K_TO_512K_MASK    131072
#define IO_SIZE_512K_TO_MAX_MASK     524288

/* IO大小分布 */
enum io_range {
    IO_SIZE_0_TO_4K = 0,  /* (0, 4K]      */
    IO_SIZE_4K_TO_32K,    /* (4K, 32K]    */
    IO_SIZE_32K_TO_128K,  /* (32K, 128K]  */
    IO_SIZE_128K_TO_512K, /* (128K, 512K) */
    IO_SIZE_512K_TO_MAX, /*  [512K, +∞)   */
    IO_SIZE_MAX /* 记录总的流量,不分大小 */
};

/* IO操作类型 */
enum io_op_type {
    OP_READ = 0,
    OP_WRITE,
#if 0
    OP_RAHEAD,
    OP_WRITE_SYNC,
    OP_READ_META,
    OP_WRITE_META,
    OP_DISCARD,
    OP_SECURE_ERASE,
    OP_FLUSH,
#endif
    OP_MAX
};

/* 不同层IO的耗时 */
enum layer_type {
    IN_DRIVER = 0,/* IO在driver层的耗时 */
    IN_BLOCK,     /* IO在block层的耗时  */
    LAYER_MAX
};

//Ensure cache line alignment
struct blk_metrics_struct {
    /* 开始统计的时间戳 */
    u64 timestamp;
    /* IO计数 */
    u64 total_cnt;
    /* IO总的大小 */
    u64 total_size;
    /* block+driver的最大耗时 */
    u64 max_time;
    struct {
        /* 累计耗时 */
        u64 elapse_time;
        /* 最大耗时 */
        u64 max_time;
    } layer[LAYER_MAX];//对block、driver层分别统计
};

extern bool block_rq_issue_enabled;
extern bool block_rq_complete_enabled;
extern struct blk_metrics_struct blk_metrics[OP_MAX][CYCLE_MAX][IO_SIZE_MAX];
extern spinlock_t blk_metrics_lock[OP_MAX][CYCLE_MAX][IO_SIZE_MAX];

void block_register_tracepoint_probes(void);
void block_unregister_tracepoint_probes(void);
int block_metrics_proc_open(struct inode *inode, struct file *file);
void block_metrics_reset(void);
void block_metrics_init(void);

#endif /* __BLOCK_METRICS_H__ */