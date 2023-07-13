// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
/*
 * LRNG Slow Entropy Source: Interrupt data collection
 *
 * Copyright (C) 2022, Stephan Mueller <smueller@chronox.de>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/gcd.h>
#include <linux/module.h>

#include "lrng_es_irq.h"
#include "lrng_es_sched.h"
#include "lrng_es_timer_common.h"
#include "lrng_health.h"

/* Is high-resolution timer present? */
static bool lrng_highres_timer_val = false;

/* Number of time stamps analyzed to calculate a GCD */
#define LRNG_GCD_WINDOW_SIZE	100
static u32 lrng_gcd_history[LRNG_GCD_WINDOW_SIZE];
static atomic_t lrng_gcd_history_ptr = ATOMIC_INIT(-1);

/* The common divisor for all timestamps */
static u32 lrng_gcd_timer = 0;

bool lrng_gcd_tested(void)
{
	return (lrng_gcd_timer != 0);
}

u32 lrng_gcd_get(void)
{
	return lrng_gcd_timer;
}

/* Set the GCD for use in IRQ ES - if 0, the GCD calculation is restarted. */
void lrng_gcd_set(u32 running_gcd)
{
	lrng_gcd_timer = running_gcd;
	/* Ensure that update to global variable lrng_gcd_timer is visible */
	mb();
}

static void lrng_gcd_set_check(u32 running_gcd)
{
	if (!lrng_gcd_tested()) {
		lrng_gcd_set(running_gcd);
		pr_debug("Setting GCD to %u\n", running_gcd);
	}
}

u32 lrng_gcd_analyze(u32 *history, size_t nelem)
{
	u32 running_gcd = 0;
	size_t i;

	/* Now perform the analysis on the accumulated time data. */
	for (i = 0; i < nelem; i++) {
		/*
		 * NOTE: this would be the place to add more analysis on the
		 * appropriateness of the timer like checking the presence
		 * of sufficient variations in the timer.
		 */

		/*
		 * This calculates the gcd of all the time values. that is
		 * gcd(time_1, time_2, ..., time_nelem)
		 *
		 * Some timers increment by a fixed (non-1) amount each step.
		 * This code checks for such increments, and allows the library
		 * to output the number of such changes have occurred.
		 */
		running_gcd = (u32)gcd(history[i], running_gcd);

		/* Zeroize data */
		history[i] = 0;
	}

	return running_gcd;
}

void lrng_gcd_add_value(u32 time)
{
	u32 ptr = (u32)atomic_inc_return_relaxed(&lrng_gcd_history_ptr);

	if (ptr < LRNG_GCD_WINDOW_SIZE) {
		lrng_gcd_history[ptr] = time;
	} else if (ptr == LRNG_GCD_WINDOW_SIZE) {
		u32 gcd = lrng_gcd_analyze(lrng_gcd_history,
					   LRNG_GCD_WINDOW_SIZE);

		if (!gcd)
			gcd = 1;

		/*
		 * Ensure that we have variations in the time stamp below the
		 * given value. This is just a safety measure to prevent the GCD
		 * becoming too large.
		 */
		if (gcd >= 1000) {
			pr_warn("calculated GCD is larger than expected: %u\n",
				gcd);
			gcd = 1000;
		}

		/*  Adjust all deltas by the observed (small) common factor. */
		lrng_gcd_set_check(gcd);
		atomic_set(&lrng_gcd_history_ptr, 0);
	}
}

/* Return boolean whether LRNG identified presence of high-resolution timer */
bool lrng_highres_timer(void)
{
	return lrng_highres_timer_val;
}

static int __init lrng_init_time_source(void)
{
	if ((random_get_entropy() & LRNG_DATA_SLOTSIZE_MASK) ||
	    (random_get_entropy() & LRNG_DATA_SLOTSIZE_MASK)) {
		/*
		 * As the highres timer is identified here, previous interrupts
		 * obtained during boot time are treated like a lowres-timer
		 * would have been present.
		 */
		lrng_highres_timer_val = true;
	} else {
		lrng_health_disable();
		lrng_highres_timer_val = false;
	}

	lrng_irq_es_init(lrng_highres_timer_val);
	lrng_sched_es_init(lrng_highres_timer_val);

	/* Ensure that changes to global variables are visible */
	mb();

	return 0;
}
core_initcall(lrng_init_time_source);
