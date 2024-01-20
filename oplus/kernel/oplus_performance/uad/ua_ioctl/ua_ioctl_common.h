/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#ifndef _UA_IOCTL_COMMON_H_
#define _UA_IOCTL_COMMON_H_

enum cpu_ioctrl_cmd {
	SET_UA_STATUS = 1,
	GET_UTIL_INFO,

	CPU_CTRL_CMD_MAX
};

enum ua_status {
	UA_AUDIO_ON = 0,
	UA_AUDIO_OFF,
	UA_AUDIO_UREGNT,

	UA_MAX_STATUS
};

struct ua_info_data {
	int status;
	unsigned long frame_prev_util_scale;
	unsigned long frame_curr_util_scale;
};

#define CPU_CTRL_MAGIC 'o'

#define CPU_CTRL_SET_UA_STATUS \
	_IOWR(CPU_CTRL_MAGIC, SET_UA_STATUS, struct ua_info_data)
#define CPU_CTRL_GET_UTIL_INFO \
	_IOR(CPU_CTRL_MAGIC, GET_UTIL_INFO, struct ua_info_data)

#endif /* _UA_IOCTL_COMMON_H_ */
