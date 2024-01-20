#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/err.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/time64.h>
#include <linux/sched/clock.h>

#define LOG_BUFFER_SIZE 1048576
#define LOG_LINE_MAX    1024
#define CLEAR_STORAGE_LOG "clear storage log"
#define STORAGE_LOG_HEAD "storage log begin:\n"

struct storage_log_data {
    char* buf; /* ring buffer */
    loff_t pos;
    ssize_t len;
    ssize_t head_len;
    ssize_t show_len;
    spinlock_t lock;
    struct proc_dir_entry *storage;
    struct proc_dir_entry *buf_log;
};

struct storage_log_data *log_data;

#define STORAGE_LOG_HEAD_LEN log_data->head_len

static size_t add_timestamp(char *buf)
{
    u64 ts = local_clock();
    unsigned long rem_nsec = do_div(ts, 1000000000);

    return sprintf(buf, "[%5lu.%06lu] ", (unsigned long)ts, rem_nsec / 1000);
}

#define STORAGE_LOG_WRITE(buffer, count, flags)                                  \
    do {                                                                         \
        char _timestamp[64];                                                     \
        ssize_t _len = add_timestamp(_timestamp);                                \
        spin_lock_irqsave(&log_data->lock, flags);                               \
        if (log_data->pos + count + _len < LOG_BUFFER_SIZE) {                    \
            memcpy(log_data->buf + log_data->pos, _timestamp, _len);              \
            log_data->pos += _len;                                                \
            memcpy(log_data->buf + log_data->pos, buffer, count);                  \
            log_data->pos += count;                                                \
        } else {                                                                  \
            memcpy(log_data->buf, _timestamp, _len);                              \
            memcpy(log_data->buf + _len , buffer, count);                         \
            log_data->pos = _len + count;                                         \
        }                                                                         \
        log_data->len = min_t(size_t, (log_data->len + _len + count), LOG_BUFFER_SIZE); \
        spin_unlock_irqrestore(&log_data->lock, flags);                                 \
    } while (0)

static void* log_start(struct seq_file *m, loff_t *pos)
{
    return (*pos >= log_data->len) ? NULL : log_data;
}

static void* log_next(struct seq_file *m, void *v, loff_t *pos)
{
    *pos += log_data->show_len;
    return (*pos >= log_data->len) ? NULL : log_data;
}

static int log_show(struct seq_file *m, void *v)
{
    ssize_t len;

    pr_debug("log show, size:%d, count:%d, read pos:%d, log pos:%d, log len:%d\n",
            m->size, m->count, m->read_pos, log_data->pos, log_data->len);

    len = min_t(size_t, m->size, (log_data->len - m->read_pos));
    memcpy(m->buf, log_data->buf + m->read_pos, len);
    m->count = len;
    log_data->show_len = len;
    return 0;
}

static void log_stop(struct seq_file *m, void *v)
{
    log_data->show_len = 0;
}

static struct seq_operations log_ops = {
    .start = log_start,
    .stop = log_stop,
    .next  = log_next,
    .show = log_show
};

static int log_open(struct inode *inode, struct file *file)
{
    int err = -1;

    err = seq_open(file, &log_ops);
    if (!err)
        ((struct seq_file *)file->private_data)->private = log_data;

    return err;
}

static ssize_t log_write(struct file *file,
               const char __user *buf,
               size_t count,
               loff_t *pos)
{
    unsigned long flags;
    ssize_t len;
    static char data_buf[LOG_LINE_MAX];

    if (!count)
        return 0;

    if (count > LOG_LINE_MAX) {
        pr_err("count(%d) is larger than log line max(%d)\n", count, LOG_LINE_MAX);
        return -1;
    }

    len = min_t(size_t, count, LOG_LINE_MAX);
    if (copy_from_user(data_buf, buf, len)) {
        pr_err("log write, copy data failed, count:%u\n", count);
        return -1;
    }

    pr_debug("log write, count:%u, len:%u, log pos:%u, log len:%u\n",
            count, len, log_data->pos, log_data->len);

    if (strncmp(CLEAR_STORAGE_LOG, data_buf, strlen(CLEAR_STORAGE_LOG)) == 0) {
        pr_err("clear all storage log\n");
        spin_lock_irqsave(&log_data->lock, flags);
        log_data->len = STORAGE_LOG_HEAD_LEN;
        log_data->pos = STORAGE_LOG_HEAD_LEN;
        spin_unlock_irqrestore(&log_data->lock, flags);
        return count;
    }

    STORAGE_LOG_WRITE(data_buf, len, flags);
    return len;
}

static const struct proc_ops log_fops = {
    .proc_open = log_open,
    .proc_read = seq_read,
    .proc_write = log_write,
    .proc_release = seq_release,
    .proc_lseek = default_llseek,
};

int pr_storage(const char *fmt, ...)
{
    static char text_buf[LOG_LINE_MAX];
    va_list args;
    int count;
    unsigned long flags;

    va_start(args, fmt);
    count = vsnprintf(text_buf, LOG_LINE_MAX, fmt, args);
    va_end(args);

    STORAGE_LOG_WRITE(text_buf, count, flags);
    return count;
}

EXPORT_SYMBOL_GPL(pr_storage);

static void log_data_init(void)
{
    log_data->head_len = strlen(STORAGE_LOG_HEAD);
    log_data->len = log_data->head_len;
    log_data->pos = log_data->head_len;

    spin_lock_init(&log_data->lock);
}

static int __init storage_log_init(void)
{
    log_data = kmalloc(sizeof(struct storage_log_data), GFP_KERNEL);
    if (!log_data) {
        pr_err("kmalloc error, storage_log init failed\n");
        goto out;
    }

    log_data_init();

    log_data->buf = vmalloc(LOG_BUFFER_SIZE);
    if (!log_data->buf) {
        pr_err("vmalloc error, storage_log init failed\n");
        goto out_kfree;
    }
    memcpy(log_data->buf, STORAGE_LOG_HEAD, STORAGE_LOG_HEAD_LEN);

    log_data->storage = proc_mkdir("storage", NULL);
    if (!log_data->storage) {
        pr_err("create storage error, storage init failed\n");
        goto out_vfree;
    }

    log_data->buf_log = proc_create_data("buf_log", S_IRWXUGO, log_data->storage,
            &log_fops, log_data);
    if (!log_data->buf_log) {
        pr_err("create buf log error, buf_log init failed\n");
        goto out_remove;
    }

    pr_info("storage_log init succeed\n");
    return 0;

out_remove:
    remove_proc_entry("storage", NULL);
out_vfree:
    vfree(log_data->buf);
out_kfree:
    kfree(log_data);
out:
    return -1;
}

static void __exit storage_log_exit(void)
{
    remove_proc_entry("buf_log", log_data->storage);
    remove_proc_entry("storage", NULL);

    vfree(log_data->buf);
    kfree(log_data);

    pr_info("storage_log exit succeed\n");
}

module_init(storage_log_init);
module_exit(storage_log_exit);
MODULE_LICENSE("GPL v2");
