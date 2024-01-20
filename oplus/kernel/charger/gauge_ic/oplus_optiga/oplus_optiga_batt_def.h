// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2023 Oplus. All rights reserved.
 */

#ifndef _OPTIGA_BATT_DEF_H_
#define _OPTIGA_BATT_DEF_H_

/* battery core limited volatge in mV */
#define BAT_CORE_LIMITED_VOL_4480 4480
#define BAT_CORE_LIMITED_VOL_4450 4450
#define BAT_CORE_LIMITED_VOL_4400 4400

/* 1st or 2nd vendor */
#define FIRST_VENDOR 0x0
#define SECOND_VENDOR 0x1
#define UNKNOWN_VENDOR 0xFF

/* battery module vendor */
#define MINGMEI 0x0
#define XINGWANGDA 0x1
#define DESAI 0x2
#define FEIMAOTUI 0x3
#define NVT 0x4
#define GUANYU_M 0x5 /* GUANYU module */
#define ATL_INDIA 0x6
#define UNKNOWN_MODULE_VENDOR 0xFF

/* battery core vendor */
#define ATL 0x0
#define SDI 0x1
#define LIWEI 0x2
#define GUANYU_C 0x3 /* GUANYU core */
#define UNKNOWN_CORE_VENDOR 0xFF

#endif /*_OPTIGA_BATT_DEF_H_*/

