// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

/* This file is used internally */

#ifndef __OPLUS_UFCS_SHA256_H__
#define __OPLUS_UFCS_SHA256_H__

int ufcs_get_encrypt_data(unsigned char random_a[16],
			  unsigned char random_b[16],
			  unsigned char auth_msg[16],
			  unsigned char encrypt_data[32]);

#endif /* __OPLUS_UFCS_SHA256_H__ */
