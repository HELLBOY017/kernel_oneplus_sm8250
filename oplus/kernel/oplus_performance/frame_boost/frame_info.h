/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#ifndef _OPLUS_FRAME_INFO_H
#define _OPLUS_FRAME_INFO_H

#define FRAME_START	(1 << 0)
#define FRAME_END	(1 << 1)

int frame_info_init(void);

void set_frame_state(unsigned int state);
int set_frame_util_min(int min_util, bool clear);

bool set_frame_rate(unsigned int frame_rate);
void set_max_frame_rate(unsigned int frame_rate);
bool is_high_frame_rate(void);
unsigned int get_frame_rate(void);
int set_frame_margin(int margin_ms);
bool check_last_compose_time(bool composition);

unsigned long get_frame_vutil(u64 delta);
unsigned long get_frame_putil(u64 delta, unsigned int frame_zone);

unsigned long frame_uclamp(unsigned long util);
#endif /* _OPLUS_FRAME_INFO_H */
