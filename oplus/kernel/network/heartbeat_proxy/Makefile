# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2018-2020 Oplus. All rights reserved.

ifeq ($(OPLUS_VND_BUILD_PLATFORM), MT6985)
KBUILD_OPTIONS += CONFIG_OPLUS_FEATURE_HEARTBEAT_MTK=m
endif

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
M ?= $(shell pwd)
modules modules_install clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) $(KBUILD_OPTIONS) $(@)
