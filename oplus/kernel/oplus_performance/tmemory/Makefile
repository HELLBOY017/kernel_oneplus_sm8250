KBUILD_OPTIONS += CONFIG_TMEMORY=m
KBUILD_OPTIONS += OPLUS_OUT_OF_TREE_KO=y

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
M ?= $(shell pwd)

modules modules_install clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) $(KBUILD_OPTIONS) $(@)
