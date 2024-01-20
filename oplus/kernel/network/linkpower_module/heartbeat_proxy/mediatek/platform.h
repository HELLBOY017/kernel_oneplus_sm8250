/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  Copyright (c) [2020], MediaTek Inc. All rights reserved.
*  This software/firmware and related documentation ("MediaTek Software") are
*  protected under relevant copyright laws.
*
*  The information contained herein is confidential and proprietary to
*  MediaTek Inc. and/or its licensors. Except as otherwise provided in the
*  applicable licensing terms with MediaTek Inc. and/or its licensors, any
*  reproduction, modification, use or disclosure of MediaTek Software, and
*  information contained herein, in whole or in part, shall be strictly
*  prohibited.
*****************************************************************************/

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#ifdef USER_LIB

        #include <stdlib.h>
        #include <string.h>

        #if !defined(KAL_ON_OSCAR)
        #include <stdint.h>
        #include <limits.h>
        #else
        typedef signed char int8_t;
        typedef short int16_t;
        typedef int int32_t;
        typedef long long int64_t;

        typedef unsigned char uint8_t;
        typedef unsigned short uint16_t;
        typedef unsigned int uint32_t;
        typedef unsigned long long uint64_t;
        #endif

        #if (__WORDSIZE == 64)
        #define POINTER_TO_UINT(x) ((unsigned long long)(x))
        #else
        #define POINTER_TO_UINT(x) ((unsigned long)(x))
        #endif

        #define ALLOC(s) malloc(s)
        #define FREE free


#else /* non-USER_LIB */

        #include <linux/kernel.h>
        #include <linux/string.h>
        #include <linux/slab.h>

        /*
        typedef signed char int8_t;
        typedef short int16_t;
        typedef int int32_t;
        typedef long long int64_t;

        typedef unsigned char uint8_t;
        typedef unsigned short uint16_t;
        typedef unsigned int uint32_t;
        typedef unsigned long long uint64_t;
        */

        #ifdef CONFIG_X86_64
        #define POINTER_TO_UINT(x) ((unsigned long long)(x))
        #else
        #define POINTER_TO_UINT(x) ((unsigned long)(x))
        #endif

        #define ALLOC(s) kmalloc(s, GFP_KERNEL)
        #define FREE kfree


#endif /* USER_LIB */

#define HTOML(x) (x)
#define HTOMS(x) (x)
#define MTOHL(x) (x)
#define MTOHS(x) (x)

#define MEMCPY memcpy
#define MEMCMP memcmp
#define MEMSET memset


//we don't use long double, so the largest base data type size will be 8 bytes
//aligned to 2^3 = 8
#define MIPC_ALIGN_BITS (3)
#define MIPC_ALIGN_SIZE (1 << MIPC_ALIGN_BITS)
#define MIPC_ALIGN(x) ((((x) + (MIPC_ALIGN_SIZE - 1)) >> MIPC_ALIGN_BITS) << MIPC_ALIGN_BITS)

#ifndef offsetof
#define offsetof(t, m) ((size_t)&((t *)0)->m)
#endif

#define DLL_EXPORT


#endif
