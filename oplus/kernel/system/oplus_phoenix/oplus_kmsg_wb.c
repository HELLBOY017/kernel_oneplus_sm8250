/*
 * This program is used to reserve kernel log to block device
 */
#include "oplus_phoenix.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/buffer_head.h>
#include <linux/blkdev.h>
#include <linux/kmsg_dump.h>
#include <linux/mount.h>
#include <linux/kdev_t.h>
#include <linux/uio.h>
#include <soc/oplus/system/boot_mode.h>
#include <soc/oplus/system/oplus_project.h>
#include <linux/version.h>

#define oprkl_info_print(fmt,arg...) \
pr_err("[op_kernel_log] "fmt, ##arg)

#define oprkl_err_print(fmt,arg...) \
pr_err("[op_kernel_log] "fmt, ##arg)

#define RETRY_COUNT_FOR_GET_DEVICE 50
#define WAITING_FOR_GET_DEVICE     100

#define PER_LOOP_MSEC              500 //check log update once per 0.5 sec
#define MAX_STOP_CNT               ((180 * 1000) / PER_LOOP_MSEC)  //180 sec
#define MAX_FINISH_CNT             10   // save exit log count/blocks

#define BUF_SIZE_OF_LINE           2048
#define WB_BLOCK_SIZE              4096

#define UEFI_HEAD_OFFSET           0
#define HEADER_BLOCK               1   //1*4096

#define KERNEL_HEAD_OFFSET         1024*1024*4 //reserve 0~ 4Mb for uefi log thread use
#define PER_LOG_BLOCKS             512 //per log has 512*4096 = 2M space
#define MAX_LOG_NUM                8

#define NORAML_BOOT_WAIT_SS_TIME   50  // wait system server start time again, 50s+180s < 240s
#define ABNORAML_BOOT_WAIT_SS_TIME 120 // wait system server start time again for abnormal boot

static struct task_struct *tsk;
const char LOG_THREAD_MAGIC[] = "PHOENIX2.0";
const char phoenix2_feedback_magic = 'x';
const char phoenix2_monitor_magic  = 'y';
const struct file_operations f_op = {.fsync = blkdev_fsync};

static signed long long wb_start_offset = KERNEL_HEAD_OFFSET;
static int write_block_cnt   = 0;
static int line_count        = 0;
static int total_write_times = 0;
static int wait_system_server_start_time = NORAML_BOOT_WAIT_SS_TIME;


struct kernel_log_thread_header
{
    /// magic number identify kernel log
	uint64_t bootcount;
	char magic[12];       // magic is PHOENIX2.0
	char is_monitoring;   // =='y',is normal mode need monitor
	char is_needfeedback; // =='x', need upload to DCS
	uint64_t last_bootcount;
	uint64_t nobootcount;
};


struct uefi_log_thread_header
{
    /// magic number identify uefi log
	uint64_t bootcount;
	char magic[12];
	char is_monitoring;   // ==1,is normal mode need monitor
	char is_needfeedback; // ==1, need upload to DCS
	uint64_t no_bootcount;
};

int wb_kernel_log(struct block_device *bdev, char *buf, char *line_buf)
{
	struct file dev_map_file;
	struct kmsg_dumper kmsg_dumper;
	struct kvec iov;
	struct iov_iter iter;
	struct kiocb kiocb;

	bool isforcewb = false; // write once per 0.5 sec PER_LOOP_MSEC
	bool isfull = false;    // buf is full

	int temp_size = 0;
	int loop_cnt = 0;
	size_t len = 0;
	int ret = 0;
	int old_block_cnt = 0;
	int boot_res = 1;       // 0: init success 1: fail

	memset(&dev_map_file, 0, sizeof(struct file));
	dev_map_file.f_mapping = bdev->bd_inode->i_mapping;
	dev_map_file.f_flags = O_DSYNC | __O_SYNC | O_NOATIME;
	dev_map_file.f_inode = bdev->bd_inode;

	init_sync_kiocb(&kiocb, &dev_map_file);
	kiocb.ki_pos = wb_start_offset; //start wb offset

	memset(&kmsg_dumper, 0, sizeof(struct kmsg_dumper));
	kmsg_dumper.active = true;
	memset(buf, 0, WB_BLOCK_SIZE);
	oprkl_info_print("Start write back log\n");

	while (loop_cnt < MAX_STOP_CNT)  // 180S timeout
	{
		while (kmsg_dump_get_line(&kmsg_dumper, true, line_buf, BUF_SIZE_OF_LINE, &len))
		{
			if(((temp_size + len) >= WB_BLOCK_SIZE)) {
				isfull = true;
			}
			if (isfull || isforcewb) {
				total_write_times++; // count real write back times
				iov.iov_base = (void *)buf;
				iov.iov_len = temp_size;
				#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
				iov_iter_kvec(&iter, WRITE | ITER_KVEC, &iov, 0, temp_size);
				#else
				iov_iter_kvec(&iter, WRITE, &iov, 0, temp_size);
				#endif
				ret = generic_write_checks(&kiocb, &iter);
				if (likely(ret > 0))
					ret = generic_perform_write(&dev_map_file, &iter, kiocb.ki_pos);
				else {
					oprkl_err_print("generic_write_checks failed\n");
					return 1;
				}
				if (likely(ret > 0)) {
					dev_map_file.f_op = &f_op;
					if(isfull) {
						kiocb.ki_pos += temp_size; //shfit offset of write back size
					}
					ret = generic_write_sync(&kiocb, ret);
					if (ret < 0) {
						oprkl_err_print("Write sync failed\n");
						return 1;
					}
				} else {
					oprkl_err_print("generic_perform_write failed\n");
					return 1;
				}
				if(isfull) {
					write_block_cnt ++;
					if (write_block_cnt >= PER_LOG_BLOCKS) { //it will record 2M
						oprkl_err_print("system_server init not ready before log full\n");
						goto EXIT;
					}
					memset(buf, 0, WB_BLOCK_SIZE);
					temp_size = 0;
				}
				if (phx_is_system_server_init_start()) {
					oprkl_info_print("system_server init ready, stop record\n");
					//it mean kernel ~ native boot complete
					boot_res = 0;
					goto FINISH;
				}
			}
			isfull = false;
			isforcewb = false;

			memcpy(buf + temp_size, line_buf, len);
			temp_size += len;
			memset(line_buf, 0, BUF_SIZE_OF_LINE);
			line_count++;
			len = 0;
		}

		loop_cnt++;
		msleep_interruptible(PER_LOOP_MSEC); // update once per 0.5 sec
		isforcewb = true;
	}
	oprkl_info_print("system_server init not ready before timeout\n");

FINISH:
	// save finished log to op5
	oprkl_info_print("Stop record line_count: %d write_block_cnt: %d, total_write_times: %d\n", line_count, write_block_cnt, total_write_times);

	old_block_cnt = write_block_cnt;
	loop_cnt  = 0;
	isfull    = false;
	isforcewb = false;

	while (loop_cnt < MAX_FINISH_CNT)
	{
		while (kmsg_dump_get_line(&kmsg_dumper, true, line_buf, BUF_SIZE_OF_LINE, &len))
		{
			if(((temp_size + len) >= WB_BLOCK_SIZE)) {
				isfull = true;
			}

			if (isfull || isforcewb) {
				total_write_times++;
				iov.iov_base = (void *)buf;
				iov.iov_len = temp_size;
				#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
				iov_iter_kvec(&iter, WRITE | ITER_KVEC, &iov, 0, temp_size);
				#else
				iov_iter_kvec(&iter, WRITE, &iov, 0, temp_size);
				#endif

				ret = generic_write_checks(&kiocb, &iter);
				if (unlikely(ret <= 0)) {
					oprkl_err_print("generic_write_checks failed\n");
					return 1;
				}

				ret = generic_perform_write(&dev_map_file, &iter, kiocb.ki_pos);
				if (unlikely(ret <= 0)) {
					oprkl_err_print("generic_perform_write failed\n");
					return 1;
				}

				dev_map_file.f_op = &f_op;
				if(isfull) {
					kiocb.ki_pos += temp_size; //shfit offset of write back size
				}
				ret = generic_write_sync(&kiocb, ret);
				if (ret < 0) {
					oprkl_err_print("Write sync failed\n");
					return 1;
				}

				if(isfull) {
					memset(buf, 0, WB_BLOCK_SIZE);
					temp_size = 0;

					//check if reach to max 2MB
					if (++write_block_cnt >= PER_LOG_BLOCKS) {
						oprkl_err_print("op5 blocks are full, exit\n");
						goto EXIT;
					}
					// support MAX_FINISH_CNT blocks for exit log
					if ((write_block_cnt - old_block_cnt) >= MAX_FINISH_CNT) {
						oprkl_err_print("finish log reach to 10 blocks, exit\n");
						goto EXIT;
					}
				}
			}

			isfull = false;
			isforcewb = false;

			memcpy(buf + temp_size, line_buf, len);
			temp_size += len;
			memset(line_buf, 0, BUF_SIZE_OF_LINE);
			line_count++;
			len = 0;
		}

		loop_cnt++;
		msleep_interruptible(PER_LOOP_MSEC);
		isforcewb = true;
	}

EXIT:
	oprkl_info_print("Finish! Stop record line_count: %d write_block_cnt: %d, total_write_times: %d\n", line_count, write_block_cnt, total_write_times);
	filp_close(&dev_map_file, NULL);
	return boot_res;
}

int read_header(struct block_device *bdev, void *head, int len, int pos) {
	struct file dev_map_file;
	struct kiocb kiocb;
	struct iov_iter iter;
	struct kvec iov;
	int read_size = 0;

	memset(&dev_map_file, 0, sizeof(struct file));

	dev_map_file.f_mapping = bdev->bd_inode->i_mapping;
	dev_map_file.f_flags = O_DSYNC | __O_SYNC | O_NOATIME;
	dev_map_file.f_inode = bdev->bd_inode;

	init_sync_kiocb(&kiocb, &dev_map_file);
	kiocb.ki_pos = pos; //start header offset
	iov.iov_base = head;
	iov.iov_len = len;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
	iov_iter_kvec(&iter, READ | ITER_KVEC, &iov, 1, len);
#else
	iov_iter_kvec(&iter, READ, &iov, 1, len);
#endif

	read_size = generic_file_read_iter(&kiocb, &iter);
	oprkl_info_print("read_header read_size %d\n", read_size);
	filp_close(&dev_map_file, NULL);

	if (read_size <= 0) {
		oprkl_err_print("read header failed\n");
		return 1;
	}
	return 0;
}

int write_header(struct block_device *bdev, void *head, int len, int pos) {
	struct file dev_map_file;
	struct kiocb kiocb;
	struct iov_iter iter;
	struct kvec iov;
	int ret = 0;

	memset(&dev_map_file, 0, sizeof(struct file));

	dev_map_file.f_mapping = bdev->bd_inode->i_mapping;
	dev_map_file.f_flags = O_DSYNC | __O_SYNC | O_NOATIME;
	dev_map_file.f_inode = bdev->bd_inode;

	init_sync_kiocb(&kiocb, &dev_map_file);
	kiocb.ki_pos = pos; //start header offset
	iov.iov_base = head;
	iov.iov_len = len;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
	iov_iter_kvec(&iter, WRITE | ITER_KVEC, &iov, 1, len);
#else
	iov_iter_kvec(&iter, WRITE, &iov, 1, len);
#endif

	ret = generic_write_checks(&kiocb, &iter);
	if (ret > 0)
		ret = generic_perform_write(&dev_map_file, &iter, kiocb.ki_pos);
	if (ret > 0) {
		dev_map_file.f_op = &f_op;
		kiocb.ki_pos += ret;
		ret = generic_write_sync(&kiocb, ret);
		if (ret < 0) {
			filp_close(&dev_map_file, NULL);
			oprkl_err_print("Write sync failed\n");
			return 1;
		}
	}
	filp_close(&dev_map_file, NULL);
	return 0;
}

struct block_device *get_reserve_partition_bdev(void)
{
	struct block_device *bdev = NULL;
	int retry_wait_for_device = RETRY_COUNT_FOR_GET_DEVICE;
	dev_t dev;

	while(retry_wait_for_device--) {
		dev = name_to_dev_t("PARTLABEL=oplusreserve5");
		if(dev != 0) {
			bdev = blkdev_get_by_dev(dev, FMODE_READ | FMODE_WRITE, NULL);
			oprkl_err_print("success to get dev block\n");
			return bdev;
		}
		dev = name_to_dev_t("PARTLABEL=opporeserve5");
		if(dev != 0) {
			bdev = blkdev_get_by_dev(dev, FMODE_READ | FMODE_WRITE,NULL);
			oprkl_err_print("success to get dev block\n");
			return bdev;
		}
		oprkl_info_print("Failed to get dev block, retry %d\n", retry_wait_for_device);
		msleep_interruptible(WAITING_FOR_GET_DEVICE);
	}
	oprkl_err_print("Failed to get dev block final\n");
	return NULL;
}

int is_need_monitored(void)
{
	int eng_ver = 0;

	if (phx_is_userdebug())
	{
		return 0;
	}

	eng_ver = get_eng_version();
	if ((eng_ver == AGING) || (eng_ver == HIGH_TEMP_AGING))
	{
		return 0;
	}

	return phx_get_normal_mode();
}

int init_header(struct block_device *bdev, struct kernel_log_thread_header *head)
{
	int ret = 0;

	memset(head, 0, sizeof(struct kernel_log_thread_header));
	ret = read_header(bdev, head, sizeof(struct kernel_log_thread_header), KERNEL_HEAD_OFFSET);
	if(ret)
		return ret;

	oprkl_info_print("Now read LOG_THREAD_MAGIC: %x %x %x\n", head->magic[0], head->magic[1], head->magic[2]);

	if (memcmp(head->magic, LOG_THREAD_MAGIC, strlen(LOG_THREAD_MAGIC)) == 0) {
		oprkl_info_print("This is initialized header\n");
		if (head->is_monitoring == phoenix2_monitor_magic) {
			oprkl_err_print("Last boot, not complete need feedback\n");
			head->is_needfeedback = phoenix2_feedback_magic;
			head->nobootcount = head->last_bootcount;
			head->is_monitoring = 0;
			head->last_bootcount = 0;
		}
		head->bootcount++;
	} else {
		oprkl_info_print("No match ,The boot is first boot, need init\n");
		memset(head, 0, sizeof(struct kernel_log_thread_header));
		memcpy(head->magic, LOG_THREAD_MAGIC, strlen(LOG_THREAD_MAGIC));
	}
	if (is_need_monitored()) {
		oprkl_info_print("The boot is normal boot, need monitor\n");
		head->is_monitoring = phoenix2_monitor_magic;
		head->last_bootcount = head->bootcount;
	}
	ret = write_header(bdev, head, sizeof(struct kernel_log_thread_header), KERNEL_HEAD_OFFSET);

	return ret;
}

int clear_monitor_header_flag(struct block_device *bdev, struct kernel_log_thread_header *head)
{
	int ret = 0;

	memset(head, 0, sizeof(struct kernel_log_thread_header));
	ret = read_header(bdev, head, sizeof(struct kernel_log_thread_header), KERNEL_HEAD_OFFSET);
	if (ret) {
		oprkl_info_print("Record completed, But read header fail\n");
		return ret;
	}
	head->is_monitoring = 0;
	head->last_bootcount = 0;
	ret = write_header(bdev, head, sizeof(struct kernel_log_thread_header), KERNEL_HEAD_OFFSET);
	if (ret) {
		oprkl_info_print("Record completed, But write header fail\n");
		return ret;
	}
	oprkl_info_print("Record completed, clear monitor flag\n");
	return 0;
}

static int reserve_log_main(void *arg)
{
	struct block_device *bdev = NULL;
	struct kernel_log_thread_header kernel_head;
	struct uefi_log_thread_header uefi_head;
	char *data_buf = NULL;
	char *line_buf = NULL;
	int ret = 0;

	oprkl_info_print("Start %s pid:%d, name:%s\n", __func__, current->pid, current->comm);
	bdev = get_reserve_partition_bdev();
	if (!bdev) {
		goto GETPATH_FAIL;
	}

	data_buf = (char *)kzalloc(WB_BLOCK_SIZE * sizeof(char), GFP_KERNEL);
	if (!data_buf) {
		oprkl_err_print("Alloc data_buf failed\n");
		goto EXIT;
	}

	line_buf = (char *)kzalloc(BUF_SIZE_OF_LINE * sizeof(char), GFP_KERNEL);
	if (!line_buf) {
		oprkl_err_print("Alloc line_buf failed\n");
		goto EXIT;
	}

	if(init_header(bdev, &kernel_head)) {
		oprkl_err_print("init_header failed\n");
		goto EXIT;
	}

	if(read_header(bdev, &uefi_head, sizeof(struct uefi_log_thread_header), UEFI_HEAD_OFFSET)) {
		oprkl_err_print("read uefi_log_thread_header failed\n");
		goto EXIT;
	}
	oprkl_info_print("Now kernel boot count %d and uefi boot count %d, is_monitoring %d, log number:[%d / 8]\n", kernel_head.bootcount, uefi_head.bootcount, kernel_head.is_monitoring, kernel_head.bootcount % MAX_LOG_NUM);
	wb_start_offset += (HEADER_BLOCK + (kernel_head.bootcount % MAX_LOG_NUM) * PER_LOG_BLOCKS) * WB_BLOCK_SIZE;
	oprkl_info_print("Write back start offset %llu\n", wb_start_offset);

	ret = wb_kernel_log(bdev, data_buf, line_buf);
	if (!ret && phx_is_system_server_init_start())
	{
		if (clear_monitor_header_flag(bdev, &kernel_head))
		{
			oprkl_err_print("clear_monitor_header_flag failed\n");
		}
	}
	else
	{
		/* give abnormal boot another timeslice */
		if (phx_is_long_time()) {
			wait_system_server_start_time = ABNORAML_BOOT_WAIT_SS_TIME;
		}
		oprkl_info_print("wait again system server start time : %d\n", wait_system_server_start_time);
		schedule_timeout_interruptible(wait_system_server_start_time * HZ);
		if (phx_is_system_server_init_start())
		{
			if (clear_monitor_header_flag(bdev, &kernel_head))
			{
				oprkl_err_print("clear_monitor_header_flag failed\n");
			}
		}
	}

EXIT:
	if (data_buf) {
		kfree(data_buf);
		data_buf = NULL;
		oprkl_info_print("Free data_buf buffer\n");
	}

	if (line_buf) {
		kfree(line_buf);
		line_buf = NULL;
		oprkl_info_print("Free line_buf buffer\n");
	}

	if (bdev) {
		blkdev_put(bdev, FMODE_READ | FMODE_WRITE);
		bdev = NULL;
		oprkl_info_print("Put device\n");
	}

GETPATH_FAIL:
	oprkl_info_print("End %s\n", __func__);
	return 0;
}

static int __init kernel_log_wb_int(void)
{
	oprkl_info_print("kernel_log_wb_int\n");
	tsk = kthread_run(reserve_log_main, NULL, "op_kernel_log");
	if (!tsk)
		oprkl_err_print("kthread init failed\n");
	oprkl_info_print("kthread init done\n");

	return 0;

}

static void __exit kernel_log_wb_exit(void)
{
	oprkl_info_print("bye bye\n");
}

late_initcall(kernel_log_wb_int);
module_exit(kernel_log_wb_exit);
MODULE_LICENSE("GPL v2");
MODULE_LICENSE("Dual BSD/GPL");
