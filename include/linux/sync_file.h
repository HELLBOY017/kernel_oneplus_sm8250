/*
 * include/linux/sync_file.h
 *
 * Copyright (C) 2012 Google, Inc.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_SYNC_FILE_H
#define _LINUX_SYNC_FILE_H

#include <linux/types.h>
#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/dma-fence.h>
#include <linux/dma-fence-array.h>

/**
 * struct sync_file - sync file to export to the userspace
 * @file:		file representing this fence
 * @sync_file_list:	membership in global file list
 * @wq:			wait queue for fence signaling
 * @flags:		flags for the sync_file
 * @fence:		fence with the fences in the sync_file
 * @cb:			fence callback information
 *
 * flags:
 * POLL_ENABLED: whether userspace is currently poll()'ing or not
 */
struct sync_file {
	struct file		*file;
#ifdef CONFIG_DEBUG_FS
	struct list_head	sync_file_list;
#endif

	wait_queue_head_t	wq;
	unsigned long		flags;

	struct dma_fence	*fence;
	struct dma_fence_cb cb;
};

#define POLL_ENABLED 0

struct sync_file *sync_file_create(struct dma_fence *fence);
struct dma_fence *sync_file_get_fence(int fd);

#endif /* _LINUX_SYNC_H */
