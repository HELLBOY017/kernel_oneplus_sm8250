// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
/*
 * LRNG proc and sysctl interfaces
 *
 * Copyright (C) 2016 - 2021, Stephan Mueller <smueller@chronox.de>
 */

#include <linux/lrng.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sysctl.h>
#include <linux/uuid.h>

#include "lrng_internal.h"
#include "lrng_es_irq.h"

/*
 * This function is used to return both the bootid UUID, and random
 * UUID.  The difference is in whether table->data is NULL; if it is,
 * then a new UUID is generated and returned to the user.
 *
 * If the user accesses this via the proc interface, the UUID will be
 * returned as an ASCII string in the standard UUID format; if via the
 * sysctl system call, as 16 bytes of binary data.
 */
static int lrng_proc_do_uuid(struct ctl_table *table, int write,
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

static int lrng_proc_do_entropy(struct ctl_table *table, int write,
				void *buffer, size_t *lenp, loff_t *ppos)
{
	struct ctl_table fake_table;
	int entropy_count;

	entropy_count = lrng_avail_entropy();

	fake_table.data = &entropy_count;
	fake_table.maxlen = sizeof(entropy_count);

	return proc_dointvec(&fake_table, write, buffer, lenp, ppos);
}

static int lrng_proc_do_poolsize(struct ctl_table *table, int write,
				 void *buffer, size_t *lenp, loff_t *ppos)
{
	struct ctl_table fake_table;
	int entropy_count;

	/* LRNG can at most retain entropy in per-CPU pools and aux pool */
	entropy_count = lrng_get_digestsize() + lrng_pcpu_avail_pool_size();

	fake_table.data = &entropy_count;
	fake_table.maxlen = sizeof(entropy_count);

	return proc_dointvec(&fake_table, write, buffer, lenp, ppos);
}

static int lrng_min_write_thresh;
static int lrng_max_write_thresh = LRNG_MAX_DIGESTSIZE;
static char lrng_sysctl_bootid[16];
static int lrng_drng_reseed_max_min;

struct ctl_table random_table[] = {
	{
		.procname	= "poolsize",
		.maxlen		= sizeof(int),
		.mode		= 0444,
		.proc_handler	= lrng_proc_do_poolsize,
	},
	{
		.procname	= "entropy_avail",
		.maxlen		= sizeof(int),
		.mode		= 0444,
		.proc_handler	= lrng_proc_do_entropy,
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
		.proc_handler	= lrng_proc_do_uuid,
	},
	{
		.procname	= "uuid",
		.maxlen		= 16,
		.mode		= 0444,
		.proc_handler	= lrng_proc_do_uuid,
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

/* Number of online DRNGs */
static u32 numa_drngs = 1;

void lrng_pool_inc_numa_node(void)
{
	numa_drngs++;
}

static int lrng_proc_type_show(struct seq_file *m, void *v)
{
	struct lrng_drng *lrng_drng_init = lrng_drng_init_instance();
	unsigned long flags = 0;
	unsigned char buf[390];

	lrng_drng_lock(lrng_drng_init, &flags);
	snprintf(buf, sizeof(buf),
		 "DRNG name: %s\n"
		 "Hash for reading entropy pool: %s\n"
		 "Hash for operating aux entropy pool: %s\n"
		 "LRNG security strength in bits: %d\n"
		 "per-CPU interrupt collection size: %u\n"
		 "number of DRNG instances: %u\n"
		 "Standards compliance: %s%s\n"
		 "High-resolution timer: %s\n"
		 "LRNG minimally seeded: %s\n"
		 "LRNG fully seeded: %s\n"
		 "Continuous compression: %s\n",
		 lrng_drng_init->crypto_cb->lrng_drng_name(),
		 lrng_drng_init->crypto_cb->lrng_hash_name(),
		 lrng_drng_init->crypto_cb->lrng_hash_name(),
		 lrng_security_strength(),
		 LRNG_DATA_NUM_VALUES,
		 numa_drngs,
		 lrng_sp80090b_compliant() ? "SP800-90B " : "",
		 lrng_sp80090c_compliant() ? "SP800-90C " : "",
		 lrng_pool_highres_timer() ? "true" : "false",
		 lrng_state_min_seeded() ? "true" : "false",
		 lrng_state_fully_seeded() ? "true" : "false",
		 lrng_pcpu_continuous_compression_state() ? "true" : "false");
	lrng_drng_unlock(lrng_drng_init, &flags);

	seq_write(m, buf, strlen(buf));

	return 0;
}

static int __init lrng_proc_type_init(void)
{
	proc_create_single("lrng_type", 0444, NULL, &lrng_proc_type_show);
	return 0;
}

module_init(lrng_proc_type_init);
