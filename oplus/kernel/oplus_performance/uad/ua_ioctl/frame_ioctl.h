/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#ifndef _FRAME_IOCTL_H_
#define _FRAME_IOCTL_H_
#define FRAMEBOOST_PROC_NODE "oplus_frame_boost"
#define INVALID_VAL (INT_MIN)

enum BoostStage {
	BOOST_NONE = 0,
	BOOST_ALL_STAGE = 1,
	BOOST_QUEUE_BUFFER_TIMEOUT,
	BOOST_DEADLINE_NOTIFY,
	BOOST_DEADLINE_EXCEEDED,

	BOOST_INPUT_ANIMATE = 100,
	BOOST_ANIMATE_TRAVERSAL,
	BOOST_TRAVERSAL_DRAW,
	BOOST_DRAW_SYNC_QUEUE,
	BOOST_SYNC_QUEUE_SYNC_START,
	BOOST_SYNC_START_ISSUE_DRAW,
	BOOST_ISSUE_DRAW_FLUSH,
	BOOST_FLUSH_COMPLETE,
	BOOST_RT_SYNC,

	BOOST_FRAME_START = 200,
	BOOST_MOVE_FG,
	BOOST_MOVE_BG,
	BOOST_OBTAIN_VIEW,
	BOOST_FRAME_END,
	BOOST_SET_HWUITASK,
	BOOST_INPUT_TIMEOUT = 206,
	BOOST_ANIMATION_TIMEOUT = 207,
	BOOST_INSETS_ANIMATION_TIMEOUT = 208,
	BOOST_TRAVERSAL_TIMEOUT = 209,
	BOOST_COMMIT_TIMEOUT = 210,
	FRAME_BOOST_END = 211,
	BOOST_FRAME_TIMEOUT = 212,
	BOOST_SET_RENDER_THREAD,
	BOOST_ADD_FRAME_TASK,

	BOOST_MSG_TRANS_START = 214,
	BOOST_CLIENT_COMPOSITION = 215,
	BOOST_SF_EXECUTE = 216,
};

enum ComposeStage     {
	NONE = 0,
	INVALIDATE,
	PRIMARY_PREPARE_FRAME,
	PRIMARY_FINISH_FRAME,
	PRIMARY_POST_FRAME,
	OTHER_PREPARE_FRAME,
	OTHER_FINISH_FRAME,
	OTHER_POST_FRAME,
	POST_COMP,
	GPU_BOOST_HINT_START = 100,
	GPU_BOOST_HINT_END,
};

enum ofb_ctrl_cmd_id {
	SET_FPS = 1,
	BOOST_HIT,
	END_FRAME,
	SF_FRAME_MISSED,
	SF_COMPOSE_HINT,
	IS_HWUI_RT,
	SET_TASK_TAGGING,
	SET_SF_MSG_TRANS,
	RELATED_SCHED_CONFIG,
	BOOST_STUNE,
	BOOST_STUNE_GPU,
	CMD_ID_MAX
};

struct ofb_ctrl_data {
	pid_t pid;
	pid_t tid;
	pid_t hwtid1;
	pid_t hwtid2;
	int stage;

	int64_t vsyncNs;
	int capacity_need;
	int related_width;
	int related_depth;
};

struct ofb_stune_data {
	int level;
	int boost_freq;
	int boost_migr;
	int vutil_margin;
	int util_frame_rate;
	int util_min_threshold;
	int util_min_obtain_view;
	int util_min_timeout;
	int ed_task_boost_mid_duration;
	int ed_task_boost_mid_util;
	int ed_task_boost_max_duration;
	int ed_task_boost_max_util;
	int ed_task_boost_timeout_duration;
	int boost_sf_freq_nongpu;
	int boost_sf_migr_nongpu;
	int boost_sf_freq_gpu;
	int boost_sf_migr_gpu;
};


#define OFB_MAGIC 0XDE
#define CMD_ID_SET_FPS \
	_IOWR(OFB_MAGIC, SET_FPS, struct ofb_ctrl_data)
#define CMD_ID_BOOST_HIT \
	_IOWR(OFB_MAGIC, BOOST_HIT, struct ofb_ctrl_data)
#define CMD_ID_END_FRAME \
	_IOWR(OFB_MAGIC, END_FRAME, struct ofb_ctrl_data)
#define CMD_ID_SF_FRAME_MISSED \
	_IOWR(OFB_MAGIC, SF_FRAME_MISSED, struct ofb_ctrl_data)
#define CMD_ID_SF_COMPOSE_HINT \
	_IOWR(OFB_MAGIC, SF_COMPOSE_HINT, struct ofb_ctrl_data)
#define CMD_ID_IS_HWUI_RT \
	_IOWR(OFB_MAGIC, IS_HWUI_RT, struct ofb_ctrl_data)
#define CMD_ID_SET_TASK_TAGGING \
	_IOWR(OFB_MAGIC, SET_TASK_TAGGING, struct ofb_ctrl_data)
#define CMD_ID_SET_SF_MSG_TRANS \
	_IOWR(OFB_MAGIC, SET_SF_MSG_TRANS, struct ofb_ctrl_data)
#define CMD_ID_RELATED_SCHED_CONFIG \
	_IOWR(OFB_MAGIC, RELATED_SCHED_CONFIG, struct ofb_ctrl_data)
#define CMD_ID_BOOST_STUNE \
	_IOWR(OFB_MAGIC, BOOST_STUNE, struct ofb_stune_data)
#define CMD_ID_BOOST_STUNE_GPU \
	_IOWR(OFB_MAGIC, BOOST_STUNE_GPU, struct ofb_stune_data)

enum ofb_ctrl_extra_cmd_id {
	SET_TASK_PREFERED_CLUSTER = 1,
	ADD_TASK_TO_GROUP = 2,
	NOTIFY_FRAME_START = 3,
};

struct ofb_ctrl_cluster {
	pid_t tid;
	int cluster_id;
	char reserved[32];
};

#define MAX_KEY_THREAD_NUM	20
struct ofb_key_thread_info {
	pid_t tid[MAX_KEY_THREAD_NUM];
	unsigned int thread_num;
	unsigned int add; /* 1 add, 0 remove */
};

struct ofb_frame_util_info {
	unsigned long frame_scale;
	unsigned int frame_busy;
};

#define OFB_EXTRA_MAGIC 0XDF
#define CMD_ID_SET_TASK_PREFERED_CLUSTER \
	_IOWR(OFB_EXTRA_MAGIC, SET_TASK_PREFERED_CLUSTER, struct ofb_ctrl_cluster)
#define CMD_ID_ADD_TASK_TO_GROUP \
	_IOWR(OFB_EXTRA_MAGIC, ADD_TASK_TO_GROUP, struct ofb_key_thread_info)
#define CMD_ID_NOTIFY_FRAME_START \
	_IOWR(OFB_EXTRA_MAGIC, NOTIFY_FRAME_START, struct ofb_frame_util_info)

int frame_ioctl_init(void);
int frame_ioctl_exit(void);
#endif /* _FRAME_IOCTL_H_ */
