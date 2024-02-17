// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
/*
 * LRNG Slow Entropy Source: Interrupt data collection
 *
 * Copyright (C) 2022, Stephan Mueller <smueller@chronox.de>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <asm/irq_regs.h>
#include <asm/ptrace.h>
#include <crypto/hash.h>
#include <linux/gcd.h>
#include <linux/module.h>
#include <linux/random.h>

#include "lrng_es_aux.h"
#include "lrng_es_irq.h"
#include "lrng_es_timer_common.h"
#include "lrng_health.h"
#include "lrng_numa.h"
#include "lrng_testing.h"

/*
 * Number of interrupts to be recorded to assume that DRNG security strength
 * bits of entropy are received.
 * Note: a value below the DRNG security strength should not be defined as this
 *	 may imply the DRNG can never be fully seeded in case other noise
 *	 sources are unavailable.
 */
#define LRNG_IRQ_ENTROPY_BITS LRNG_UINT32_C(CONFIG_LRNG_IRQ_ENTROPY_RATE)


/* Number of interrupts required for LRNG_DRNG_SECURITY_STRENGTH_BITS entropy */
static u32 lrng_irq_entropy_bits = LRNG_IRQ_ENTROPY_BITS;

static u32 irq_entropy __read_mostly = LRNG_IRQ_ENTROPY_BITS;
#ifdef CONFIG_LRNG_RUNTIME_ES_CONFIG
module_param(irq_entropy, uint, 0444);
MODULE_PARM_DESC(irq_entropy,
		 "How many interrupts must be collected for obtaining 256 bits of entropy\n");
#endif

/* Per-CPU array holding concatenated IRQ entropy events */
static DEFINE_PER_CPU(u32 [LRNG_DATA_ARRAY_SIZE], lrng_irq_array)
						__aligned(LRNG_KCAPI_ALIGN);
static DEFINE_PER_CPU(u32, lrng_irq_array_ptr) = 0;
static DEFINE_PER_CPU(atomic_t, lrng_irq_array_irqs) = ATOMIC_INIT(0);

/*
 * The entropy collection is performed by executing the following steps:
 * 1. fill up the per-CPU array holding the time stamps
 * 2. once the per-CPU array is full, a compression of the data into
 *    the entropy pool is performed - this happens in interrupt context
 *
 * If step 2 is not desired in interrupt context, the following boolean
 * needs to be set to false. This implies that old entropy data in the
 * per-CPU array collected since the last DRNG reseed is overwritten with
 * new entropy data instead of retaining the entropy with the compression
 * operation.
 *
 * Impact on entropy:
 *
 * If continuous compression is enabled, the maximum entropy that is collected
 * per CPU between DRNG reseeds is equal to the digest size of the used hash.
 *
 * If continuous compression is disabled, the maximum number of entropy events
 * that can be collected per CPU is equal to LRNG_DATA_ARRAY_SIZE. This amount
 * of events is converted into an entropy statement which then represents the
 * maximum amount of entropy collectible per CPU between DRNG reseeds.
 */
static bool lrng_irq_continuous_compression __read_mostly =
			IS_ENABLED(CONFIG_LRNG_ENABLE_CONTINUOUS_COMPRESSION);

#ifdef CONFIG_LRNG_SWITCHABLE_CONTINUOUS_COMPRESSION
module_param(lrng_irq_continuous_compression, bool, 0444);
MODULE_PARM_DESC(lrng_irq_continuous_compression,
		 "Perform entropy compression if per-CPU entropy data array is full\n");
#endif

/*
 * Per-CPU entropy pool with compressed entropy event
 *
 * The per-CPU entropy pool is defined as the hash state. New data is simply
 * inserted into the entropy pool by performing a hash update operation.
 * To read the entropy pool, a hash final must be invoked. However, before
 * the entropy pool is released again after a hash final, the hash init must
 * be performed.
 */
static DEFINE_PER_CPU(u8 [LRNG_POOL_SIZE], lrng_irq_pool)
						__aligned(LRNG_KCAPI_ALIGN);
/*
 * Lock to allow other CPUs to read the pool - as this is only done during
 * reseed which is infrequent, this lock is hardly contended.
 */
static DEFINE_PER_CPU(spinlock_t, lrng_irq_lock);
static DEFINE_PER_CPU(bool, lrng_irq_lock_init) = false;

static bool lrng_irq_pool_online(int cpu)
{
	return per_cpu(lrng_irq_lock_init, cpu);
}

static void __init lrng_irq_check_compression_state(void)
{
	/* One pool must hold sufficient entropy for disabled compression */
	if (!lrng_irq_continuous_compression) {
		u32 max_ent = min_t(u32, lrng_get_digestsize(),
				    lrng_data_to_entropy(LRNG_DATA_NUM_VALUES,
							lrng_irq_entropy_bits));
		if (max_ent < lrng_security_strength()) {
			pr_warn("Force continuous compression operation to ensure LRNG can hold enough entropy\n");
			lrng_irq_continuous_compression = true;
		}
	}
}

void __init lrng_irq_es_init(bool highres_timer)
{
	/* Set a minimum number of interrupts that must be collected */
	irq_entropy = max_t(u32, LRNG_IRQ_ENTROPY_BITS, irq_entropy);

	if (highres_timer) {
		lrng_irq_entropy_bits = irq_entropy;
	} else {
		u32 new_entropy = irq_entropy * LRNG_ES_OVERSAMPLING_FACTOR;

		lrng_irq_entropy_bits = (irq_entropy < new_entropy) ?
					 new_entropy : irq_entropy;
		pr_warn("operating without high-resolution timer and applying IRQ oversampling factor %u\n",
			LRNG_ES_OVERSAMPLING_FACTOR);
	}

	lrng_irq_check_compression_state();
}

/*
 * Reset all per-CPU pools - reset entropy estimator but leave the pool data
 * that may or may not have entropy unchanged.
 */
static void lrng_irq_reset(void)
{
	int cpu;

	/* Trigger GCD calculation anew. */
	lrng_gcd_set(0);

	for_each_online_cpu(cpu)
		atomic_set(per_cpu_ptr(&lrng_irq_array_irqs, cpu), 0);
}

static u32 lrng_irq_avail_pool_size(void)
{
	u32 max_size = 0, max_pool = lrng_get_digestsize();
	int cpu;

	if (!lrng_irq_continuous_compression)
		max_pool = min_t(u32, max_pool, LRNG_DATA_NUM_VALUES);

	for_each_online_cpu(cpu) {
		if (lrng_irq_pool_online(cpu))
			max_size += max_pool;
	}

	return max_size;
}

/* Return entropy of unused IRQs present in all per-CPU pools. */
static u32 lrng_irq_avail_entropy(u32 __unused)
{
	u32 digestsize_irqs, irq = 0;
	int cpu;

	/* Only deliver entropy when SP800-90B self test is completed */
	if (!lrng_sp80090b_startup_complete_es(lrng_int_es_irq))
		return 0;

	/* Obtain the cap of maximum numbers of IRQs we count */
	digestsize_irqs = lrng_entropy_to_data(lrng_get_digestsize(),
					       lrng_irq_entropy_bits);
	if (!lrng_irq_continuous_compression) {
		/* Cap to max. number of IRQs the array can hold */
		digestsize_irqs = min_t(u32, digestsize_irqs,
					LRNG_DATA_NUM_VALUES);
	}

	for_each_online_cpu(cpu) {
		if (!lrng_irq_pool_online(cpu))
			continue;
		irq += min_t(u32, digestsize_irqs,
			     atomic_read_u32(per_cpu_ptr(&lrng_irq_array_irqs,
							 cpu)));
	}

	/* Consider oversampling rate */
	return lrng_reduce_by_osr(lrng_data_to_entropy(irq,
						       lrng_irq_entropy_bits));
}

/*
 * Trigger a switch of the hash implementation for the per-CPU pool.
 *
 * For each per-CPU pool, obtain the message digest with the old hash
 * implementation, initialize the per-CPU pool again with the new hash
 * implementation and inject the message digest into the new state.
 *
 * Assumption: the caller must guarantee that the new_cb is available during the
 * entire operation (e.g. it must hold the lock against pointer updating).
 */
static int
lrng_irq_switch_hash(struct lrng_drng *drng, int node,
		     const struct lrng_hash_cb *new_cb, void *new_hash,
		     const struct lrng_hash_cb *old_cb)
{
	u8 digest[LRNG_MAX_DIGESTSIZE];
	u32 digestsize_irqs, found_irqs;
	int ret = 0, cpu;

	if (!IS_ENABLED(CONFIG_LRNG_SWITCH))
		return -EOPNOTSUPP;

	for_each_online_cpu(cpu) {
		struct shash_desc *pcpu_shash;

		/*
		 * Only switch the per-CPU pools for the current node because
		 * the hash_cb only applies NUMA-node-wide.
		 */
		if (cpu_to_node(cpu) != node || !lrng_irq_pool_online(cpu))
			continue;

		pcpu_shash = (struct shash_desc *)per_cpu_ptr(lrng_irq_pool,
							      cpu);

		digestsize_irqs = old_cb->hash_digestsize(pcpu_shash);
		digestsize_irqs = lrng_entropy_to_data(digestsize_irqs << 3,
						       lrng_irq_entropy_bits);

		if (pcpu_shash->tfm == new_hash)
			continue;

		/* Get the per-CPU pool hash with old digest ... */
		ret = old_cb->hash_final(pcpu_shash, digest) ?:
		      /* ... re-initialize the hash with the new digest ... */
		      new_cb->hash_init(pcpu_shash, new_hash) ?:
		      /*
		       * ... feed the old hash into the new state. We may feed
		       * uninitialized memory into the new state, but this is
		       * considered no issue and even good as we have some more
		       * uncertainty here.
		       */
		      new_cb->hash_update(pcpu_shash, digest,
					       sizeof(digest));
		if (ret)
			goto out;

		/*
		 * In case the new digest is larger than the old one, cap
		 * the available entropy to the old message digest used to
		 * process the existing data.
		 */
		found_irqs = atomic_xchg_relaxed(
				per_cpu_ptr(&lrng_irq_array_irqs, cpu), 0);
		found_irqs = min_t(u32, found_irqs, digestsize_irqs);
		atomic_add_return_relaxed(found_irqs,
				per_cpu_ptr(&lrng_irq_array_irqs, cpu));

		pr_debug("Re-initialize per-CPU entropy pool for CPU %d on NUMA node %d with hash %s\n",
			 cpu, node, new_cb->hash_name());
	}

out:
	memzero_explicit(digest, sizeof(digest));
	return ret;
}

/*
 * When reading the per-CPU message digest, make sure we use the crypto
 * callbacks defined for the NUMA node the per-CPU pool is defined for because
 * the LRNG crypto switch support is only atomic per NUMA node.
 */
static u32
lrng_irq_pool_hash_one(const struct lrng_hash_cb *pcpu_hash_cb,
		       void *pcpu_hash, int cpu, u8 *digest, u32 *digestsize)
{
	struct shash_desc *pcpu_shash =
		(struct shash_desc *)per_cpu_ptr(lrng_irq_pool, cpu);
	spinlock_t *lock = per_cpu_ptr(&lrng_irq_lock, cpu);
	unsigned long flags;
	u32 digestsize_irqs, found_irqs;

	/* Lock guarding against reading / writing to per-CPU pool */
	spin_lock_irqsave(lock, flags);

	*digestsize = pcpu_hash_cb->hash_digestsize(pcpu_hash);
	digestsize_irqs = lrng_entropy_to_data(*digestsize << 3,
					       lrng_irq_entropy_bits);

	/* Obtain entropy statement like for the entropy pool */
	found_irqs = atomic_xchg_relaxed(
				per_cpu_ptr(&lrng_irq_array_irqs, cpu), 0);
	/* Cap to maximum amount of data we can hold in hash */
	found_irqs = min_t(u32, found_irqs, digestsize_irqs);

	/* Cap to maximum amount of data we can hold in array */
	if (!lrng_irq_continuous_compression)
		found_irqs = min_t(u32, found_irqs, LRNG_DATA_NUM_VALUES);

	/* Store all not-yet compressed data in data array into hash, ... */
	if (pcpu_hash_cb->hash_update(pcpu_shash,
				(u8 *)per_cpu_ptr(lrng_irq_array, cpu),
				LRNG_DATA_ARRAY_SIZE * sizeof(u32)) ?:
	    /* ... get the per-CPU pool digest, ... */
	    pcpu_hash_cb->hash_final(pcpu_shash, digest) ?:
	    /* ... re-initialize the hash, ... */
	    pcpu_hash_cb->hash_init(pcpu_shash, pcpu_hash) ?:
	    /* ... feed the old hash into the new state. */
	    pcpu_hash_cb->hash_update(pcpu_shash, digest, *digestsize))
		found_irqs = 0;

	spin_unlock_irqrestore(lock, flags);
	return found_irqs;
}

/*
 * Hash all per-CPU pools and return the digest to be used as seed data for
 * seeding a DRNG. The caller must guarantee backtracking resistance.
 * The function will only copy as much data as entropy is available into the
 * caller-provided output buffer.
 *
 * This function handles the translation from the number of received interrupts
 * into an entropy statement. The conversion depends on LRNG_IRQ_ENTROPY_BITS
 * which defines how many interrupts must be received to obtain 256 bits of
 * entropy. With this value, the function lrng_data_to_entropy converts a given
 * data size (received interrupts, requested amount of data, etc.) into an
 * entropy statement. lrng_entropy_to_data does the reverse.
 *
 * @eb: entropy buffer to store entropy
 * @requested_bits: Requested amount of entropy
 * @fully_seeded: indicator whether LRNG is fully seeded
 */
static void lrng_irq_pool_hash(struct entropy_buf *eb, u32 requested_bits,
			       bool fully_seeded)
{
	SHASH_DESC_ON_STACK(shash, NULL);
	const struct lrng_hash_cb *hash_cb;
	struct lrng_drng **lrng_drng = lrng_drng_instances();
	struct lrng_drng *drng = lrng_drng_init_instance();
	u8 digest[LRNG_MAX_DIGESTSIZE];
	unsigned long flags, flags2;
	u32 found_irqs, collected_irqs = 0, collected_ent_bits, requested_irqs,
	    returned_ent_bits;
	int ret, cpu;
	void *hash;

	/* Only deliver entropy when SP800-90B self test is completed */
	if (!lrng_sp80090b_startup_complete_es(lrng_int_es_irq)) {
		eb->e_bits[lrng_int_es_irq] = 0;
		return;
	}

	/* Lock guarding replacement of per-NUMA hash */
	read_lock_irqsave(&drng->hash_lock, flags);

	hash_cb = drng->hash_cb;
	hash = drng->hash;

	/* The hash state of filled with all per-CPU pool hashes. */
	ret = hash_cb->hash_init(shash, hash);
	if (ret)
		goto err;

	/* Cap to maximum entropy that can ever be generated with given hash */
	lrng_cap_requested(hash_cb->hash_digestsize(hash) << 3, requested_bits);
	requested_irqs = lrng_entropy_to_data(requested_bits +
					      lrng_compress_osr(),
					      lrng_irq_entropy_bits);

	/*
	 * Harvest entropy from each per-CPU hash state - even though we may
	 * have collected sufficient entropy, we will hash all per-CPU pools.
	 */
	for_each_online_cpu(cpu) {
		struct lrng_drng *pcpu_drng = drng;
		u32 digestsize, pcpu_unused_irqs = 0;
		int node = cpu_to_node(cpu);

		/* If pool is not online, then no entropy is present. */
		if (!lrng_irq_pool_online(cpu))
			continue;

		if (lrng_drng && lrng_drng[node])
			pcpu_drng = lrng_drng[node];

		if (pcpu_drng == drng) {
			found_irqs = lrng_irq_pool_hash_one(hash_cb, hash,
							    cpu, digest,
							    &digestsize);
		} else {
			read_lock_irqsave(&pcpu_drng->hash_lock, flags2);
			found_irqs =
				lrng_irq_pool_hash_one(pcpu_drng->hash_cb,
						       pcpu_drng->hash, cpu,
						       digest, &digestsize);
			read_unlock_irqrestore(&pcpu_drng->hash_lock, flags2);
		}

		/* Inject the digest into the state of all per-CPU pools */
		ret = hash_cb->hash_update(shash, digest, digestsize);
		if (ret)
			goto err;

		collected_irqs += found_irqs;
		if (collected_irqs > requested_irqs) {
			pcpu_unused_irqs = collected_irqs - requested_irqs;
			atomic_add_return_relaxed(pcpu_unused_irqs,
				per_cpu_ptr(&lrng_irq_array_irqs, cpu));
			collected_irqs = requested_irqs;
		}
		pr_debug("%u interrupts used from entropy pool of CPU %d, %u interrupts remain unused\n",
			 found_irqs - pcpu_unused_irqs, cpu, pcpu_unused_irqs);
	}

	ret = hash_cb->hash_final(shash, digest);
	if (ret)
		goto err;

	collected_ent_bits = lrng_data_to_entropy(collected_irqs,
						  lrng_irq_entropy_bits);
	/* Apply oversampling: discount requested oversampling rate */
	returned_ent_bits = lrng_reduce_by_osr(collected_ent_bits);

	pr_debug("obtained %u bits by collecting %u bits of entropy from entropy pool noise source\n",
		 returned_ent_bits, collected_ent_bits);

	/*
	 * Truncate to available entropy as implicitly allowed by SP800-90B
	 * section 3.1.5.1.1 table 1 which awards truncated hashes full
	 * entropy.
	 *
	 * During boot time, we read requested_bits data with
	 * returned_ent_bits entropy. In case our conservative entropy
	 * estimate underestimates the available entropy we can transport as
	 * much available entropy as possible.
	 */
	memcpy(eb->e[lrng_int_es_irq], digest,
	       fully_seeded ? returned_ent_bits >> 3 : requested_bits >> 3);
	eb->e_bits[lrng_int_es_irq] = returned_ent_bits;

out:
	hash_cb->hash_desc_zero(shash);
	read_unlock_irqrestore(&drng->hash_lock, flags);
	memzero_explicit(digest, sizeof(digest));
	return;

err:
	eb->e_bits[lrng_int_es_irq] = 0;
	goto out;
}

/* Compress the lrng_irq_array array into lrng_irq_pool */
static void lrng_irq_array_compress(void)
{
	struct shash_desc *shash =
			(struct shash_desc *)this_cpu_ptr(lrng_irq_pool);
	struct lrng_drng *drng = lrng_drng_node_instance();
	const struct lrng_hash_cb *hash_cb;
	spinlock_t *lock = this_cpu_ptr(&lrng_irq_lock);
	unsigned long flags, flags2;
	void *hash;
	bool init = false;

	read_lock_irqsave(&drng->hash_lock, flags);
	hash_cb = drng->hash_cb;
	hash = drng->hash;

	if (unlikely(!this_cpu_read(lrng_irq_lock_init))) {
		init = true;
		spin_lock_init(lock);
		this_cpu_write(lrng_irq_lock_init, true);
		pr_debug("Initializing per-CPU entropy pool for CPU %d on NUMA node %d with hash %s\n",
			 raw_smp_processor_id(), numa_node_id(),
			 hash_cb->hash_name());
	}

	spin_lock_irqsave(lock, flags2);

	if (unlikely(init) && hash_cb->hash_init(shash, hash)) {
		this_cpu_write(lrng_irq_lock_init, false);
		pr_warn("Initialization of hash failed\n");
	} else if (lrng_irq_continuous_compression) {
		/* Add entire per-CPU data array content into entropy pool. */
		if (hash_cb->hash_update(shash,
					(u8 *)this_cpu_ptr(lrng_irq_array),
					LRNG_DATA_ARRAY_SIZE * sizeof(u32)))
			pr_warn_ratelimited("Hashing of entropy data failed\n");
	}

	spin_unlock_irqrestore(lock, flags2);
	read_unlock_irqrestore(&drng->hash_lock, flags);
}

/* Compress data array into hash */
static void lrng_irq_array_to_hash(u32 ptr)
{
	u32 *array = this_cpu_ptr(lrng_irq_array);

	/*
	 * During boot time the hash operation is triggered more often than
	 * during regular operation.
	 */
	if (unlikely(!lrng_state_fully_seeded())) {
		if ((ptr & 31) && (ptr < LRNG_DATA_WORD_MASK))
			return;
	} else if (ptr < LRNG_DATA_WORD_MASK) {
		return;
	}

	if (lrng_raw_array_entropy_store(*array)) {
		u32 i;

		/*
		 * If we fed even a part of the array to external analysis, we
		 * mark that the entire array and the per-CPU pool to have no
		 * entropy. This is due to the non-IID property of the data as
		 * we do not fully know whether the existing dependencies
		 * diminish the entropy beyond to what we expect it has.
		 */
		atomic_set(this_cpu_ptr(&lrng_irq_array_irqs), 0);

		for (i = 1; i < LRNG_DATA_ARRAY_SIZE; i++)
			lrng_raw_array_entropy_store(*(array + i));
	} else {
		lrng_irq_array_compress();
		/* Ping pool handler about received entropy */
		if (lrng_sp80090b_startup_complete_es(lrng_int_es_irq))
			lrng_es_add_entropy();
	}
}

/*
 * Concatenate full 32 bit word at the end of time array even when current
 * ptr is not aligned to sizeof(data).
 */
static void _lrng_irq_array_add_u32(u32 data)
{
	/* Increment pointer by number of slots taken for input value */
	u32 pre_ptr, mask, ptr = this_cpu_add_return(lrng_irq_array_ptr,
						     LRNG_DATA_SLOTS_PER_UINT);
	unsigned int pre_array;

	/*
	 * This function injects a unit into the array - guarantee that
	 * array unit size is equal to data type of input data.
	 */
	BUILD_BUG_ON(LRNG_DATA_ARRAY_MEMBER_BITS != (sizeof(data) << 3));

	/*
	 * The following logic requires at least two units holding
	 * the data as otherwise the pointer would immediately wrap when
	 * injection an u32 word.
	 */
	BUILD_BUG_ON(LRNG_DATA_NUM_VALUES <= LRNG_DATA_SLOTS_PER_UINT);

	lrng_data_split_u32(&ptr, &pre_ptr, &mask);

	/* MSB of data go into previous unit */
	pre_array = lrng_data_idx2array(pre_ptr);
	/* zeroization of slot to ensure the following OR adds the data */
	this_cpu_and(lrng_irq_array[pre_array], ~(0xffffffff & ~mask));
	this_cpu_or(lrng_irq_array[pre_array], data & ~mask);

	/* Invoke compression as we just filled data array completely */
	if (unlikely(pre_ptr > ptr))
		lrng_irq_array_to_hash(LRNG_DATA_WORD_MASK);

	/* LSB of data go into current unit */
	this_cpu_write(lrng_irq_array[lrng_data_idx2array(ptr)],
		       data & mask);

	if (likely(pre_ptr <= ptr))
		lrng_irq_array_to_hash(ptr);
}

/* Concatenate a 32-bit word at the end of the per-CPU array */
void lrng_irq_array_add_u32(u32 data)
{
	/*
	 * Disregard entropy-less data without continuous compression to
	 * avoid it overwriting data with entropy when array ptr wraps.
	 */
	if (lrng_irq_continuous_compression)
		_lrng_irq_array_add_u32(data);
}

/* Concatenate data of max LRNG_DATA_SLOTSIZE_MASK at the end of time array */
static void lrng_irq_array_add_slot(u32 data)
{
	/* Get slot */
	u32 ptr = this_cpu_inc_return(lrng_irq_array_ptr) &
							LRNG_DATA_WORD_MASK;
	unsigned int array = lrng_data_idx2array(ptr);
	unsigned int slot = lrng_data_idx2slot(ptr);

	BUILD_BUG_ON(LRNG_DATA_ARRAY_MEMBER_BITS % LRNG_DATA_SLOTSIZE_BITS);
	/* Ensure consistency of values */
	BUILD_BUG_ON(LRNG_DATA_ARRAY_MEMBER_BITS !=
		     sizeof(lrng_irq_array[0]) << 3);

	/* zeroization of slot to ensure the following OR adds the data */
	this_cpu_and(lrng_irq_array[array],
		     ~(lrng_data_slot_val(0xffffffff & LRNG_DATA_SLOTSIZE_MASK,
					  slot)));
	/* Store data into slot */
	this_cpu_or(lrng_irq_array[array], lrng_data_slot_val(data, slot));

	lrng_irq_array_to_hash(ptr);
}

static void
lrng_time_process_common(u32 time, void(*add_time)(u32 data))
{
	enum lrng_health_res health_test;

	if (lrng_raw_hires_entropy_store(time))
		return;

	health_test = lrng_health_test(time, lrng_int_es_irq);
	if (health_test > lrng_health_fail_use)
		return;

	if (health_test == lrng_health_pass)
		atomic_inc_return(this_cpu_ptr(&lrng_irq_array_irqs));

	add_time(time);
}

/*
 * Batching up of entropy in per-CPU array before injecting into entropy pool.
 */
static void lrng_time_process(void)
{
	u32 now_time = random_get_entropy();

	if (unlikely(!lrng_gcd_tested())) {
		/* When GCD is unknown, we process the full time stamp */
		lrng_time_process_common(now_time, _lrng_irq_array_add_u32);
		lrng_gcd_add_value(now_time);
	} else {
		/* GCD is known and applied */
		lrng_time_process_common((now_time / lrng_gcd_get()) &
					 LRNG_DATA_SLOTSIZE_MASK,
					 lrng_irq_array_add_slot);
	}

	lrng_perf_time(now_time);
}

/* Hot code path - Callback for interrupt handler */
void add_interrupt_randomness(int irq)
{
	if (lrng_highres_timer()) {
		lrng_time_process();
	} else {
		struct pt_regs *regs = get_irq_regs();
		static atomic_t reg_idx = ATOMIC_INIT(0);
		u64 ip;
		u32 tmp;

		if (regs) {
			u32 *ptr = (u32 *)regs;
			int reg_ptr = atomic_add_return_relaxed(1, &reg_idx);
			size_t n = (sizeof(struct pt_regs) / sizeof(u32));

			ip = instruction_pointer(regs);
			tmp = *(ptr + (reg_ptr % n));
			tmp = lrng_raw_regs_entropy_store(tmp) ? 0 : tmp;
			_lrng_irq_array_add_u32(tmp);
		} else {
			ip = _RET_IP_;
		}

		lrng_time_process();

		/*
		 * The XOR operation combining the different values is not
		 * considered to destroy entropy since the entirety of all
		 * processed values delivers the entropy (and not each
		 * value separately of the other values).
		 */
		tmp = lrng_raw_jiffies_entropy_store(jiffies) ? 0 : jiffies;
		tmp ^= lrng_raw_irq_entropy_store(irq) ? 0 : irq;
		tmp ^= lrng_raw_retip_entropy_store(ip) ? 0 : ip;
		tmp ^= ip >> 32;
		_lrng_irq_array_add_u32(tmp);
	}
}
EXPORT_SYMBOL(add_interrupt_randomness);

static void lrng_irq_es_state(unsigned char *buf, size_t buflen)
{
	const struct lrng_drng *lrng_drng_init = lrng_drng_init_instance();

	/* Assume the lrng_drng_init lock is taken by caller */
	snprintf(buf, buflen,
		 " Hash for operating entropy pool: %s\n"
		 " Available entropy: %u\n"
		 " per-CPU interrupt collection size: %u\n"
		 " Standards compliance: %s\n"
		 " High-resolution timer: %s\n"
		 " Continuous compression: %s\n",
		 lrng_drng_init->hash_cb->hash_name(),
		 lrng_irq_avail_entropy(0),
		 LRNG_DATA_NUM_VALUES,
		 lrng_sp80090b_compliant(lrng_int_es_irq) ? "SP800-90B " : "",
		 lrng_highres_timer() ? "true" : "false",
		 lrng_irq_continuous_compression ? "true" : "false");
}

struct lrng_es_cb lrng_es_irq = {
	.name 			= "IRQ",
	.get_ent		= lrng_irq_pool_hash,
	.curr_entropy		= lrng_irq_avail_entropy,
	.max_entropy		= lrng_irq_avail_pool_size,
	.state			= lrng_irq_es_state,
	.reset			= lrng_irq_reset,
	.switch_hash		= lrng_irq_switch_hash,
};
