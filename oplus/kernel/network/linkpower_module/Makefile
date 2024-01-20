# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2018-2020 Oplus. All rights reserved.

ifeq ($(CHIPSET_COMPANY), QCOM)
KBUILD_OPTIONS += CONFIG_OPLUS_FEATURE_QCOM=m
else ifeq ($(CHIPSET_COMPANY), MTK)
KBUILD_OPTIONS += CONFIG_OPLUS_FEATURE_MTK=m
endif

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
M ?= $(shell pwd)
modules modules_install clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) $(KBUILD_OPTIONS) $(@)
