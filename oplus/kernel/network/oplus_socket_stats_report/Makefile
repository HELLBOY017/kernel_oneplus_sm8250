#
# Add for kernel socket info send to user space.
#
KBUILD_OPTIONS += CONFIG_OPLUS_FEATURE_ESTABLISH_REPORT=m

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
M ?= $(shell pwd)
modules modules_install clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) $(KBUILD_OPTIONS) $(@)
