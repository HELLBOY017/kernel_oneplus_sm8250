# SPDX-License-Identifier: GPL-2.0-only
zram-y	:=	zcomp.o zram_drv.o
zram-$(CONFIG_ZRAM_DEDUP)	+=	zram_dedup.o

obj-$(CONFIG_ZRAM)	+=	zram.o

#ifdef OPLUS_FEATURE_ZRAM_WRITEBACK
obj-$(CONFIG_ZWB_HANDLE) += zwb_handle/
#endif /* OPLUS_FEATURE_ZRAM_WRITEBACK */
