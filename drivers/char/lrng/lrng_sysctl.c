// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
/*
 * LRNG sysctl interfaces
 *
 * Copyright (C) 2022, Stephan Mueller <smueller@chronox.de>
 */

#include <linux/lrng.h>
#include <linux/proc_fs.h>
#include <linux/sysctl.h>
#include <linux/uuid.h>

#include "lrng_drng_mgr.h"
#include "lrng_es_mgr.h"
#include "lrng_sysctl.h"

/*
 * This function is used to return both the bootid UUID, and random
 * UUID.  The difference is in whether table->data is NULL; if it is,
 * then a new UUID is generated and returned to the user.
 *
 * If the user accesses this via the proc interface, the UUID will be
 * returned as an ASCII string in the standard UUID format; if via the
 * sysctl system call, as 16 bytes of binary data.
 */
static int lrng_sysctl_do_uuid(struct ctl_table *table, int write,
			       void *buffer, size_t *lenp, loff_t *ppos)
{
	struct ctl_table fake_table;
	unsigned char buf[64], tmp_uuid[16], *uuid;

	uuid = table->data;
	if (!uuid) {
		uuid = tmp_uuid;
		generate_random_uuid(uuid);
	} else {
		static DEFINE_SPINLOCK(bootid_spinlock);

		spin_lock(&bootid_spinlock);
		if (!uuid[8])
			generate_random_uuid(uuid);
		spin_unlock(&bootid_spinlock);
	}

	sprintf(buf, "%pU", uuid);

	fake_table.data = buf;
	fake_table.maxlen = sizeof(buf);

	return proc_dostring(&fake_table, write, buffer, lenp, ppos);
}

static int lrng_sysctl_do_entropy(struct ctl_table *table, int write,
				void *buffer, size_t *lenp, loff_t *ppos)
{
	struct ctl_table fake_table;
	int entropy_count;

	entropy_count = lrng_avail_entropy();

	fake_table.data = &entropy_count;
	fake_table.maxlen = sizeof(entropy_count);

	return proc_dointvec(&fake_table, write, buffer, lenp, ppos);
}

static int lrng_sysctl_do_poolsize(struct ctl_table *table, int write,
				   void *buffer, size_t *lenp, loff_t *ppos)
{
	struct ctl_table fake_table;
	u32 i, entropy_count = 0;

	for_each_lrng_es(i)
		entropy_count += lrng_es[i]->max_entropy();

	fake_table.data = &entropy_count;
	fake_table.maxlen = sizeof(entropy_count);

	return proc_dointvec(&fake_table, write, buffer, lenp, ppos);
}

static int lrng_min_write_thresh;
static int lrng_max_write_thresh = (LRNG_WRITE_WAKEUP_ENTROPY << 3);
static char lrng_sysctl_bootid[16];
static int lrng_drng_reseed_max_min;

void lrng_sysctl_update_max_write_thresh(u32 new_digestsize)
{
	lrng_max_write_thresh = (int)new_digestsize;
	/* Ensure that changes to the global variable are visible */
	mb();
}

struct ctl_table random_table[] = {
	{
		.procname	= "poolsize",
		.maxlen		= sizeof(int),
		.mode		= 0444,
		.proc_handler	= lrng_sysctl_do_poolsize,
	},
	{
		.procname	= "entropy_avail",
		.maxlen		= sizeof(int),
		.mode		= 0444,
		.proc_handler	= lrng_sysctl_do_entropy,
	},
	{
		.procname	= "write_wakeup_threshold",
		.data		= &lrng_write_wakeup_bits,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_minmax,
		.extra1		= &lrng_min_write_thresh,
		.extra2		= &lrng_max_write_thresh,
	},
	{
		.procname	= "boot_id",
		.data		= &lrng_sysctl_bootid,
		.maxlen		= 16,
		.mode		= 0444,
		.proc_handler	= lrng_sysctl_do_uuid,
	},
	{
		.procname	= "uuid",
		.maxlen		= 16,
		.mode		= 0444,
		.proc_handler	= lrng_sysctl_do_uuid,
	},
	{
		.procname       = "urandom_min_reseed_secs",
		.data           = &lrng_drng_reseed_max_time,
		.maxlen         = sizeof(int),
		.mode           = 0644,
		.proc_handler   = proc_dointvec,
		.extra1		= &lrng_drng_reseed_max_min,
	},
	{ }
};

