// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
/*
 * Backend for the LRNG providing the cryptographic primitives using the
 * kernel crypto API and its DRBG.
 *
 * Copyright (C) 2016 - 2021, Stephan Mueller <smueller@chronox.de>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <crypto/drbg.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/lrng.h>

#include "lrng_kcapi_hash.h"

/*
 * Define a DRBG plus a hash / MAC used to extract data from the entropy pool.
 * For LRNG_HASH_NAME you can use a hash or a MAC (HMAC or CMAC) of your choice
 * (Note, you should use the suggested selections below -- using SHA-1 or MD5
 * is not wise). The idea is that the used cipher primitive can be selected to
 * be the same as used for the DRBG. I.e. the LRNG only uses one cipher
 * primitive using the same cipher implementation with the options offered in
 * the following. This means, if the CTR DRBG is selected and AES-NI is present,
 * both the CTR DRBG and the selected cmac(aes) use AES-NI.
 *
 * The security strengths of the DRBGs are all 256 bits according to
 * SP800-57 section 5.6.1.
 *
 * This definition is allowed to be changed.
 */
#ifdef CONFIG_CRYPTO_DRBG_CTR
static unsigned int lrng_drbg_type = 0;
#elif defined CONFIG_CRYPTO_DRBG_HMAC
static unsigned int lrng_drbg_type = 1;
#elif defined CONFIG_CRYPTO_DRBG_HASH
static unsigned int lrng_drbg_type = 2;
#else
#error "Unknown DRBG in use"
#endif

/* The parameter must be r/o in sysfs as otherwise races appear. */
module_param(lrng_drbg_type, uint, 0444);
MODULE_PARM_DESC(lrng_drbg_type, "DRBG type used for LRNG (0->CTR_DRBG, 1->HMAC_DRBG, 2->Hash_DRBG)");

struct lrng_drbg {
	const char *hash_name;
	const char *drbg_core;
};

static const struct lrng_drbg lrng_drbg_types[] = {
	{	/* CTR_DRBG with AES-256 using derivation function */
		.hash_name = "sha512",
		.drbg_core = "drbg_nopr_ctr_aes256",
	}, {	/* HMAC_DRBG with SHA-512 */
		.hash_name = "sha512",
		.drbg_core = "drbg_nopr_hmac_sha512",
	}, {	/* Hash_DRBG with SHA-512 using derivation function */
		.hash_name = "sha512",
		.drbg_core = "drbg_nopr_sha512"
	}
};

static int lrng_drbg_drng_seed_helper(void *drng, const u8 *inbuf, u32 inbuflen)
{
	struct drbg_state *drbg = (struct drbg_state *)drng;
	LIST_HEAD(seedlist);
	struct drbg_string data;
	int ret;

	drbg_string_fill(&data, inbuf, inbuflen);
	list_add_tail(&data.list, &seedlist);
	ret = drbg->d_ops->update(drbg, &seedlist, drbg->seeded);

	if (ret >= 0)
		drbg->seeded = true;

	return ret;
}

static int lrng_drbg_drng_generate_helper(void *drng, u8 *outbuf, u32 outbuflen)
{
	struct drbg_state *drbg = (struct drbg_state *)drng;

	return drbg->d_ops->generate(drbg, outbuf, outbuflen, NULL);
}

static void *lrng_drbg_drng_alloc(u32 sec_strength)
{
	struct drbg_state *drbg;
	int coreref = -1;
	bool pr = false;
	int ret;

	drbg_convert_tfm_core(lrng_drbg_types[lrng_drbg_type].drbg_core,
			      &coreref, &pr);
	if (coreref < 0)
		return ERR_PTR(-EFAULT);

	drbg = kzalloc(sizeof(struct drbg_state), GFP_KERNEL);
	if (!drbg)
		return ERR_PTR(-ENOMEM);

	drbg->core = &drbg_cores[coreref];
	drbg->seeded = false;
	ret = drbg_alloc_state(drbg);
	if (ret)
		goto err;

	if (sec_strength > drbg_sec_strength(drbg->core->flags)) {
		pr_err("Security strength of DRBG (%u bits) lower than requested by LRNG (%u bits)\n",
			drbg_sec_strength(drbg->core->flags) * 8,
			sec_strength * 8);
		goto dealloc;
	}

	if (sec_strength < drbg_sec_strength(drbg->core->flags))
		pr_warn("Security strength of DRBG (%u bits) higher than requested by LRNG (%u bits)\n",
			drbg_sec_strength(drbg->core->flags) * 8,
			sec_strength * 8);

	pr_info("DRBG with %s core allocated\n", drbg->core->backend_cra_name);

	return drbg;

dealloc:
	if (drbg->d_ops)
		drbg->d_ops->crypto_fini(drbg);
	drbg_dealloc_state(drbg);
err:
	kfree(drbg);
	return ERR_PTR(-EINVAL);
}

static void lrng_drbg_drng_dealloc(void *drng)
{
	struct drbg_state *drbg = (struct drbg_state *)drng;

	if (drbg && drbg->d_ops)
		drbg->d_ops->crypto_fini(drbg);
	drbg_dealloc_state(drbg);
	kfree_sensitive(drbg);
	pr_info("DRBG deallocated\n");
}

static void *lrng_drbg_hash_alloc(void)
{
	return lrng_kcapi_hash_alloc(lrng_drbg_types[lrng_drbg_type].hash_name);
}

static const char *lrng_drbg_name(void)
{
	return lrng_drbg_types[lrng_drbg_type].drbg_core;
}

static const char *lrng_hash_name(void)
{
	return lrng_drbg_types[lrng_drbg_type].hash_name;
}

static const struct lrng_crypto_cb lrng_drbg_crypto_cb = {
	.lrng_drng_name			= lrng_drbg_name,
	.lrng_hash_name			= lrng_hash_name,
	.lrng_drng_alloc		= lrng_drbg_drng_alloc,
	.lrng_drng_dealloc		= lrng_drbg_drng_dealloc,
	.lrng_drng_seed_helper		= lrng_drbg_drng_seed_helper,
	.lrng_drng_generate_helper	= lrng_drbg_drng_generate_helper,
	.lrng_hash_alloc		= lrng_drbg_hash_alloc,
	.lrng_hash_dealloc		= lrng_kcapi_hash_dealloc,
	.lrng_hash_digestsize		= lrng_kcapi_hash_digestsize,
	.lrng_hash_init			= lrng_kcapi_hash_init,
	.lrng_hash_update		= lrng_kcapi_hash_update,
	.lrng_hash_final		= lrng_kcapi_hash_final,
	.lrng_hash_desc_zero		= lrng_kcapi_hash_zero,
};

static int __init lrng_drbg_init(void)
{
	if (lrng_drbg_type >= ARRAY_SIZE(lrng_drbg_types)) {
		pr_err("lrng_drbg_type parameter too large (given %u - max: %lu)",
		       lrng_drbg_type,
		       (unsigned long)ARRAY_SIZE(lrng_drbg_types) - 1);
		return -EAGAIN;
	}
	return lrng_set_drng_cb(&lrng_drbg_crypto_cb);
}

static void __exit lrng_drbg_exit(void)
{
	lrng_set_drng_cb(NULL);
}

late_initcall(lrng_drbg_init);
module_exit(lrng_drbg_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Stephan Mueller <smueller@chronox.de>");
MODULE_DESCRIPTION("Linux Random Number Generator - SP800-90A DRBG backend");
