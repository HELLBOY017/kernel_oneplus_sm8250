/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#ifndef _DISCARD_H
#define _DISCARD_H
//#include "ext4.h"

#define MAX_DISCARD_GRANLULARITY	512
//#define MAX_DISCARD_BLOCKS(sbi)		BLKS_PER_SEC(sbi)
#define DEF_MAX_DISCARD_REQUEST		8	/* issue 8 discards per round */
#define DEF_MAX_DISCARD_LEN		512	/* Max. 2MB per discard */
#define DEF_MIN_DISCARD_ISSUE_TIME	50	/* 50 ms, if exists */
#define DEF_MID_DISCARD_ISSUE_TIME	500	/* 500 ms, if device busy */
#define DEF_MAX_DISCARD_ISSUE_TIME	60000	/* 60 s, if no candidates */
#define DEF_DISCARD_URGENT_UTIL		20	/* do more discard if free blocks under 20% */
#define DEF_CP_INTERVAL			60	/* 60 secs */
#define DEF_IDLE_INTERVAL		5	/* 5 secs */

/* default discard granularity of inner discard thread, unit: block count */
#define DEFAULT_DISCARD_GRANULARITY		1

enum {
	DPOLICY_BG,
	DPOLICY_FORCE,
	DPOLICY_FSTRIM,
	DPOLICY_UMOUNT,
	MAX_DPOLICY,
};

int ext4_seq_discard_info_show(struct seq_file *seq, void *v);
#endif
