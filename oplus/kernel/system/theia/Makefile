# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2018-2020 Oplus. All rights reserved.

obj-$(CONFIG_OPLUS_FEATURE_THEIA) += oplus_theia.o
# for ko build, avoid cyclic dependence by build into single ko
oplus_theia-y := black_screen_check.o bright_screen_check.o theia_kevent_kernel.o powerkey_monitor.o theia_send_event.o
