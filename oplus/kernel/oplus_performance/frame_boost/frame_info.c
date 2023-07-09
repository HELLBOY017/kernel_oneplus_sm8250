// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#include <linux/sched.h>
#include "frame_boost.h"
#include "frame_debug.h"

#define DEFAULT_VUTIL_MARGIN    (0)
#define DEFAULT_FRAME_RATE      (60)
#define DEFAULT_FRAME_INTERVAL  (16666667L)
#define FRAME_MAX_UTIL          (1024)

/**
 * struct frame_info - all information related to frame draw.
 * @frame_rate: frame rate, 60, 90 or 120Hz.
 * @frame_rate_max: set by sf and app can not draw frame more than this rate.
 * @frame_interval: frame length, for example: 16.67ms for 60Hz frame rate.
 * @frame_vutil: virtual utility of this frame, nothing to do with exec
 *         time and only related to delta time from frame start.
 * @need_for_probe: If needs_suppliers is on a list, this indicates if the
 *         suppliers are needed for probe or not.
 * @frame_max_util: max util set from userspace.
 * @frame_min_util: min util set from userspace.
 * @vutil_time2max: time when virtual util set to 1024.
 * @frame_state: indicate the state of frame.
 * @last_compose_time: last client composition time.
 * @vutil_margin: adjust vutil_time2max.
 * @clear_limit: true if max/min should be set to default value in next frame
 *         state changed.
 */
struct frame_info {
	unsigned int frame_rate;
	unsigned int frame_rate_max;
	unsigned int frame_interval;
	unsigned int frame_vutil;
	unsigned int frame_max_util;
	unsigned int frame_min_util;
	unsigned int vutil_time2max;
	unsigned int frame_state;
	int vutil_margin;
	u64 last_compose_time;
	bool clear_limit;
};

static DEFINE_RAW_SPINLOCK(frame_info_lock);
static struct frame_info default_frame_info;

static void __set_frame_rate(unsigned int frame_rate)
{
	struct frame_info *frame_info = NULL;

	frame_info = &default_frame_info;
	frame_info->frame_rate = frame_rate;
	frame_info->frame_interval = NSEC_PER_SEC / frame_rate;
	frame_info->vutil_margin = 0;
	frame_info->vutil_time2max = frame_info->frame_interval / NSEC_PER_MSEC
		+ frame_info->vutil_margin;

	if (unlikely(sysctl_frame_boost_debug))
		trace_printk("frame_rate=%u frame_interval=%u vutil_time2max=%u\n",
			frame_info->frame_rate,
			frame_info->frame_interval,
			frame_info->vutil_time2max);
}

/*
 * get_frame_rate - get current frame rate of system
 */
int get_frame_rate(void)
{
	struct frame_info *frame_info = NULL;
	unsigned long flags;
	unsigned int frame_rate;

	raw_spin_lock_irqsave(&frame_info_lock, flags);
	frame_info = &default_frame_info;
	frame_rate = frame_info->frame_rate;
	raw_spin_unlock_irqrestore(&frame_info_lock, flags);

	return frame_rate;
}

/*
 * set_max_frame_rate - set max frame rate by sf
 * @frame_rate: frame rate
 *
 * Note: must update frame group window size after calling this function
 *
 */
void set_max_frame_rate(unsigned int frame_rate)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&frame_info_lock, flags);
	default_frame_info.frame_rate_max = frame_rate;
	__set_frame_rate(frame_rate);
	raw_spin_unlock_irqrestore(&frame_info_lock, flags);
}

/*
 * set_frame_rate - set frame rate by top app
 * @frame_rate: frame rate
 *
 *  Return: true if update frame rate successfully
 */
bool set_frame_rate(unsigned int frame_rate)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&frame_info_lock, flags);

	if (frame_rate == default_frame_info.frame_rate) {
		raw_spin_unlock_irqrestore(&frame_info_lock, flags);
		return false;
	}

	if (frame_rate > default_frame_info.frame_rate_max) {
		raw_spin_unlock_irqrestore(&frame_info_lock, flags);
		return false;
	}

	__set_frame_rate(frame_rate);
	raw_spin_unlock_irqrestore(&frame_info_lock, flags);

	if (unlikely(sysctl_frame_boost_debug)) {
		struct frame_info *frame_info = &default_frame_info;

		trace_printk("frame_rate=%u frame_interval=%u vutil_time2max=%u\n",
			frame_info->frame_rate,
			frame_info->frame_interval,
			frame_info->vutil_time2max);
	}

	return true;
}

int set_frame_margin(int margin_ms)
{
	struct frame_info *frame_info = NULL;
	unsigned long flags;
	int interval_ms = -1;
	int max_margin = INT_MAX;
	int min_margin = INT_MIN;

	frame_info = &default_frame_info;
	raw_spin_lock_irqsave(&frame_info_lock, flags);

	interval_ms = div_u64(frame_info->frame_interval, NSEC_PER_MSEC);
	max_margin = interval_ms;
	min_margin = -1 * (interval_ms >> 1);
	if (margin_ms < min_margin || margin_ms > max_margin) {
		ofb_err("invalid frame margin %d, max is %d, min is %d",
			margin_ms, min_margin, max_margin);
		raw_spin_unlock_irqrestore(&frame_info_lock, flags);
		return -EINVAL;
	}

	frame_info->vutil_margin = margin_ms;
	frame_info->vutil_time2max = frame_info->frame_interval / NSEC_PER_MSEC + frame_info->vutil_margin;
	raw_spin_unlock_irqrestore(&frame_info_lock, flags);

	return 0;
}

bool is_high_frame_rate(void)
{
	return default_frame_info.frame_rate > DEFAULT_FRAME_RATE;
}

/*
 * set_frame_util_min - set minimal utility clamp value
 * @min_util: minimal utility
 */
int set_frame_util_min(int min_util, bool clear)
{
	struct frame_info *frame_info = NULL;
	unsigned long flags;

	if (min_util < 0 || min_util > FRAME_MAX_UTIL)
		return -EINVAL;

	if (unlikely(sysctl_frame_boost_debug)) {
		frame_min_util_systrace_c(min_util);
	}
	frame_info = &default_frame_info;
	raw_spin_lock_irqsave(&frame_info_lock, flags);
	frame_info->frame_min_util = min_util;
	frame_info->clear_limit = clear;
	raw_spin_unlock_irqrestore(&frame_info_lock, flags);

	return 0;
}

/*
 * set_frame_state - set frame state when switch fg/bg, receive vsync-app or
 *            out of valid frame range (default set to 2 frame length)
 * @delta: frame state.
 *
 * switch fg/bg---------FRAME_END
 * receive vsync-app----FRAME_START
 * extra long frame-----FRAME_END
 */
void set_frame_state(unsigned int state)
{
	struct frame_info *frame_info = NULL;
	unsigned long flags;

	frame_info = &default_frame_info;

	raw_spin_lock_irqsave(&frame_info_lock, flags);
	frame_info->frame_state = state;
	if (unlikely(frame_info->clear_limit)) {
		frame_info->frame_max_util = FRAME_MAX_UTIL;
		frame_info->frame_min_util = 0;
		frame_info->clear_limit = false;
	}
	raw_spin_unlock_irqrestore(&frame_info_lock, flags);
}

/*
 * get_frame_vutil - calculate frame virtual util using delta
 *             time from frame start
 * @delta: delta time (nano sec).
 *
 * We use parabola to emulate the relationship between delta and virtual load
 * we have 2 know point in the parabola, one is (0,0) and the other is
 * (max_time, max_vutil) or (1.25 frame length, 1024), so it is easy to figure
 * out the function as the following:
 * virtual utility = f(delta)
 *    = delta * delta + (max_vutil/max_time - max_time) * delta
 *    = delta * (delta + max_vutil/max_time - max_time)
 *
 * Return: virtual utility
 */
unsigned long get_frame_vutil(u64 delta)
{
	unsigned long vutil = 0;
	int delta_ms = -1, max_time = -1;
	int tmp;
	struct frame_info *frame_info;
	unsigned long flags;

	frame_info = &default_frame_info;
	raw_spin_lock_irqsave(&frame_info_lock, flags);
	if (frame_info->frame_state == FRAME_END)
		goto out;

	delta_ms = div_u64(delta, NSEC_PER_MSEC);
	max_time = frame_info->vutil_time2max;

	if (max_time <= 0 || delta_ms > max_time) {
		vutil = FRAME_MAX_UTIL;
		goto out;
	}

	tmp = delta_ms + FRAME_MAX_UTIL / max_time;
	if (tmp <= max_time)
		goto out;

	vutil = delta_ms * (tmp - max_time);
out:
	raw_spin_unlock_irqrestore(&frame_info_lock, flags);
	return vutil;
}

/*
 * get_frame_util - calculate frame physical util using delta
 * @delta: delta time (nano sec).
 *
 * Return: physical utility
 */
unsigned long get_frame_putil(u64 delta, unsigned int frame_zone)
{
	struct frame_info *frame_info;
	unsigned long util = 0;
	unsigned long frame_interval = 0;

	frame_info = &default_frame_info;
	frame_interval = (frame_zone & FRAME_ZONE) ?
		frame_info->frame_interval : DEFAULT_FRAME_INTERVAL;

	if (frame_interval > 0)
		util = div_u64((delta << SCHED_CAPACITY_SHIFT), frame_interval);

	return util;
}

unsigned long frame_uclamp(unsigned long util)
{
	struct frame_info *frame_info = &default_frame_info;
	unsigned long clamp_util;

	if (unlikely(frame_info->frame_min_util > frame_info->frame_max_util))
		return util;

	clamp_util = max_t(unsigned long, frame_info->frame_min_util, util);

	return min_t(unsigned long, clamp_util, frame_info->frame_max_util);
}

bool check_last_compose_time(bool composition)
{
	struct frame_info *frame_info = &default_frame_info;
	unsigned long flags;
	u64 now;

	now = fbg_ktime_get_ns();
	if (composition) {
		raw_spin_lock_irqsave(&frame_info_lock, flags);
		frame_info->last_compose_time = now;
		raw_spin_unlock_irqrestore(&frame_info_lock, flags);
	}

	return (now - frame_info->last_compose_time) <= frame_info->frame_interval;
}

int frame_info_init(void)
{
	struct frame_info *frame_info = NULL;

	frame_info = &default_frame_info;
	memset(frame_info, 0, sizeof(struct frame_info));

	frame_info->frame_rate = DEFAULT_FRAME_RATE;
	frame_info->frame_interval = DEFAULT_FRAME_INTERVAL;
	frame_info->frame_rate_max = DEFAULT_FRAME_RATE;
	frame_info->frame_max_util = FRAME_MAX_UTIL;
	frame_info->frame_min_util = 0;
	frame_info->vutil_margin = DEFAULT_VUTIL_MARGIN;
	frame_info->vutil_time2max = frame_info->frame_interval / NSEC_PER_MSEC +
		frame_info->vutil_margin;
	frame_info->frame_state = FRAME_END;
	frame_info->clear_limit = false;
	frame_info->last_compose_time = 0;

	return 0;
}
