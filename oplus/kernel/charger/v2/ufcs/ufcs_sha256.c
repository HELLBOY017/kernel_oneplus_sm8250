// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2023 Oplus. All rights reserved.
 */

#include <linux/types.h>
#include <linux/kernel.h>

#define UFCS_ENCRYPTION_BLOCK_SIZE 32
#define UFCS_ENCRYPTION_HASH_A 0x6a09e667
#define UFCS_ENCRYPTION_HASH_B 0xbb67ae85
#define UFCS_ENCRYPTION_HASH_C 0x3c6ef372
#define UFCS_ENCRYPTION_HASH_D 0xa54ff53a
#define UFCS_ENCRYPTION_HASH_E 0x510e527f
#define UFCS_ENCRYPTION_HASH_F 0x9b05688c
#define UFCS_ENCRYPTION_HASH_G 0x1f83d9ab
#define UFCS_ENCRYPTION_HASH_H 0x5be0cd19

static unsigned char ufcs_source_data[64] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06,
	0x38, 0xb8, 0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80,
};

static unsigned int ufcs_verify[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
	0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
	0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
	0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
	0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
	0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define ROTLEFT(a, b) (((a) << (b)) | ((a) >> (32 - (b))))
#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32 - (b))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x, 2) ^ ROTRIGHT(x, 13) ^ ROTRIGHT(x, 22))
#define EP1(x) (ROTRIGHT(x, 6) ^ ROTRIGHT(x, 11) ^ ROTRIGHT(x, 25))
#define SIG0(x) (ROTRIGHT(x, 7) ^ ROTRIGHT(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x, 17) ^ ROTRIGHT(x, 19) ^ ((x) >> 10))

int ufcs_get_encrypt_data(unsigned char random_a[16],
			  unsigned char random_b[16],
			  unsigned char auth_msg[16],
			  unsigned char encrypt_data[32])
{
	unsigned char i, j;
	unsigned int a, b, c, d, e, f, g, h, t1, t2, m[64];

	for (i = 0; i < 16; i++) {
		ufcs_source_data[i] = random_a[i];
	}

	for (i = 0; i < 16; i++) {
		ufcs_source_data[32 + i] = random_b[i];
	}

	for (i = 0; i < 16; i++) {
		ufcs_source_data[16 + i] = auth_msg[i];
	}

	for (i = 0, j = 0; i < 16; ++i, j += 4) {
		m[i] = (ufcs_source_data[j] << 24) |
		       (ufcs_source_data[j + 1] << 16) |
		       (ufcs_source_data[j + 2] << 8) |
		       (ufcs_source_data[j + 3]);
	}

	for (i = 16; i < 64; ++i) {
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
	}

	a = UFCS_ENCRYPTION_HASH_A;
	b = UFCS_ENCRYPTION_HASH_B;
	c = UFCS_ENCRYPTION_HASH_C;
	d = UFCS_ENCRYPTION_HASH_D;
	e = UFCS_ENCRYPTION_HASH_E;
	f = UFCS_ENCRYPTION_HASH_F;
	g = UFCS_ENCRYPTION_HASH_G;
	h = UFCS_ENCRYPTION_HASH_H;

	for (i = 0; i < 64; ++i) {
		t1 = h + EP1(e) + CH(e, f, g) + ufcs_verify[i] + m[i];
		t2 = EP0(a) + MAJ(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	a += UFCS_ENCRYPTION_HASH_A;
	b += UFCS_ENCRYPTION_HASH_B;
	c += UFCS_ENCRYPTION_HASH_C;
	d += UFCS_ENCRYPTION_HASH_D;
	e += UFCS_ENCRYPTION_HASH_E;
	f += UFCS_ENCRYPTION_HASH_F;
	g += UFCS_ENCRYPTION_HASH_G;
	h += UFCS_ENCRYPTION_HASH_H;

	for (i = 0; i < 4; ++i) {
		encrypt_data[i] = (a >> (24 - i * 8)) & 0x000000ff;
		encrypt_data[i + 4] = (b >> (24 - i * 8)) & 0x000000ff;
		encrypt_data[i + 8] = (c >> (24 - i * 8)) & 0x000000ff;
		encrypt_data[i + 12] = (d >> (24 - i * 8)) & 0x000000ff;
		encrypt_data[i + 16] = (e >> (24 - i * 8)) & 0x000000ff;
		encrypt_data[i + 20] = (f >> (24 - i * 8)) & 0x000000ff;
		encrypt_data[i + 24] = (g >> (24 - i * 8)) & 0x000000ff;
		encrypt_data[i + 28] = (h >> (24 - i * 8)) & 0x000000ff;
	}

	return 0;
}
