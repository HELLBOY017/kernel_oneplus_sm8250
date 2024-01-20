#include "io_metrics_entry.h"
#include <trace/events/block.h>
#include "f2fs_metrics.h"
#include "block_metrics.h"
#include "ufs_metrics.h"
#include "procfs.h"
#include "abnormal_io.h"

bool io_metrics_enabled = false;
bool io_metrics_debug_enabled = false;

static void io_metrics_register_tracepoints(void)
{
    /* filesystem layer */
    f2fs_register_tracepoint_probes();
    /* block layer */
    block_register_tracepoint_probes();
    /* ufs layer */
    ufs_register_tracepoint_probes();
}

static void io_metrics_unregister_tracepoints(void)
{
    /* filesystem layer */
    f2fs_unregister_tracepoint_probes();
    /* block layer */
    block_unregister_tracepoint_probes();
    /* ufs layer */
    ufs_unregister_tracepoint_probes();
    tracepoint_synchronize_unregister();

    return;
}

static int __init io_metrics_init(void)
{
    io_metrics_print("Startting...\n");
    io_metrics_enabled = false;
    f2fs_metrics_init();
    block_metrics_init();
    ufs_metrics_reset();
    io_metrics_register_tracepoints();
    if (io_metrics_procfs_init())
    {
        io_metrics_print("io_metrics_procfs_init failed\n");
    }
    io_metrics_enabled = true;
    io_metrics_print("Start OK\n");
    return 0;
}

static void __exit io_metrics_exit(void)
{
    io_metrics_enabled = false;
    io_metrics_print("io_metrics_exit\n");
    io_metrics_unregister_tracepoints();
    io_metrics_procfs_exit();
}

module_init(io_metrics_init);
module_exit(io_metrics_exit);

MODULE_DESCRIPTION("oplus_bsp_storage_io_metrics");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL v2");
