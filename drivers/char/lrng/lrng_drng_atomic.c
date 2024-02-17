// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
/*
 * LRNG DRNG for atomic contexts
 *
 * Copyright (C) 2022, Stephan Mueller <smueller@chronox.de>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/lrng.h>

#include "lrng_drng_atomic.h"
#include "lrng_drng_chacha20.h"
#include "lrng_es_aux.h"
#include "lrng_es_mgr.h"
#include "lrng_sha.h"

static struct chacha20_state chacha20_atomic = {
	LRNG_CC20_INIT_RFC7539(.block)
};

/*
 * DRNG usable in atomic context. This DRNG will always use the ChaCha20
 * DRNG. It will never benefit from a DRNG switch like the "regular" DRNG. If
 * there was no DRNG switch, the atomic DRNG is identical to the "regular" DRNG.
 *
 * The reason for having this is due to the fact that DRNGs other than
 * the ChaCha20 DRNG may sleep.
 */
static struct lrng_drng lrng_drng_atomic = {
	LRNG_DRNG_STATE_INIT(lrng_drng_atomic,
			     &chacha20_atomic, NULL,
			     &lrng_cc20_drng_cb, &lrng_sha_hash_cb),
	.spin_lock	= __SPIN_LOCK_UNLOCKED(lrng_drng_atomic.spin_lock)
};

void lrng_drng_atomic_reset(void)
{
	unsigned long flags;

	spin_lock_irqsave(&lrng_drng_atomic.spin_lock, flags);
	lrng_drng_reset(&lrng_drng_atomic);
	spin_unlock_irqrestore(&lrng_drng_atomic.spin_lock, flags);
}

void lrng_drng_atomic_force_reseed(void)
{
	lrng_drng_atomic.force_reseed = lrng_drng_atomic.fully_seeded;
}

static bool lrng_drng_atomic_must_reseed(struct lrng_drng *drng)
{
	return (!drng->fully_seeded ||
		atomic_read(&lrng_drng_atomic.requests) == 0 ||
		drng->force_reseed ||
		time_after(jiffies,
			   drng->last_seeded + lrng_drng_reseed_max_time * HZ));
}

void lrng_drng_atomic_seed_drng(struct lrng_drng *regular_drng)
{
	u8 seedbuf[LRNG_DRNG_SECURITY_STRENGTH_BYTES]
						__aligned(LRNG_KCAPI_ALIGN);
	int ret;

	if (!lrng_drng_atomic_must_reseed(&lrng_drng_atomic))
		return;

	/*
	 * Reseed atomic DRNG another DRNG "regular" while this regular DRNG
	 * is reseeded. Therefore, this code operates in non-atomic context and
	 * thus can use the lrng_drng_get function to get random numbers from
	 * the just freshly seeded DRNG.
	 */
	ret = lrng_drng_get(regular_drng, seedbuf, sizeof(seedbuf));

	if (ret < 0) {
		pr_warn("Error generating random numbers for atomic DRNG: %d\n",
			ret);
	} else {
		unsigned long flags;

		spin_lock_irqsave(&lrng_drng_atomic.spin_lock, flags);
		lrng_drng_inject(&lrng_drng_atomic, seedbuf, ret,
				 regular_drng->fully_seeded, "atomic");
		spin_unlock_irqrestore(&lrng_drng_atomic.spin_lock, flags);
	}
	memzero_explicit(&seedbuf, sizeof(seedbuf));
}

void lrng_drng_atomic_seed_es(void)
{
	struct entropy_buf seedbuf __aligned(LRNG_KCAPI_ALIGN);
	struct lrng_drng *drng = &lrng_drng_atomic;
	unsigned long flags;
	u32 requested_bits = LRNG_MIN_SEED_ENTROPY_BITS;

	if (lrng_drng_atomic.fully_seeded)
		return;

	/*
	 * When the LRNG reached the minimally seeded level, leave 256 bits of
	 * entropy for the regular DRNG. In addition ensure that additional
	 * 256 bits are available for the atomic DRNG to get to the fully
	 * seeded stage of the LRNG.
	 */
	if (lrng_state_min_seeded()) {
		u32 avail_bits = lrng_avail_entropy();

		requested_bits =
			(avail_bits >= 2 * LRNG_FULL_SEED_ENTROPY_BITS) ?
			LRNG_FULL_SEED_ENTROPY_BITS : 0;

		if (!requested_bits)
			return;
	}

	pr_debug("atomic DRNG seeding attempt to pull %u bits of entropy directly from entropy sources\n",
		 requested_bits);

	lrng_fill_seed_buffer(&seedbuf, requested_bits);
	spin_lock_irqsave(&drng->spin_lock, flags);
	lrng_drng_inject(&lrng_drng_atomic, (u8 *)&seedbuf, sizeof(seedbuf),
			 lrng_fully_seeded_eb(drng->fully_seeded, &seedbuf),
			 "atomic");
	spin_unlock_irqrestore(&drng->spin_lock, flags);
	lrng_init_ops(&seedbuf);

	/*
	 * Do not call lrng_init_ops(seedbuf) here as the atomic DRNG does not
	 * serve common users.
	 */
	memzero_explicit(&seedbuf, sizeof(seedbuf));
}

static void lrng_drng_atomic_get(u8 *outbuf, u32 outbuflen)
{
	struct lrng_drng *drng = &lrng_drng_atomic;
	unsigned long flags;

	if (!outbuf || !outbuflen)
		return;

	outbuflen = min_t(size_t, outbuflen, INT_MAX);

	while (outbuflen) {
		u32 todo = min_t(u32, outbuflen, LRNG_DRNG_MAX_REQSIZE);
		int ret;

		atomic_dec(&drng->requests);

		spin_lock_irqsave(&drng->spin_lock, flags);
		ret = drng->drng_cb->drng_generate(drng->drng, outbuf, todo);
		spin_unlock_irqrestore(&drng->spin_lock, flags);
		if (ret <= 0) {
			pr_warn("getting random data from DRNG failed (%d)\n",
				ret);
			return;
		}
		outbuflen -= ret;
		outbuf += ret;
	}
}

void lrng_get_random_bytes(void *buf, int nbytes)
{
	lrng_drng_atomic_get((u8 *)buf, (u32)nbytes);
	lrng_debug_report_seedlevel("lrng_get_random_bytes");
}
EXPORT_SYMBOL(lrng_get_random_bytes);
