# tmemory
#ccflags-y := -std=gnu99 -Wno-incompatible-pointer-types -Wno-format -Wno-unused-variable -Wno-implicit-function-declaration -Wno-int-conversion -Wno-declaration-after-statement
obj-$(CONFIG_TMEMORY) += tmemory.o

tmemory-y := main.o io.o shrinker.o util.o sysfs.o
tmemory-$(CONFIG_TMEMORY_CRYPTO)		+= crypt.o
tmemory-$(CONFIG_TMEMORY_FAULT_INJECTION)	+= fault_inject.o

ifeq ($(OPLUS_OUT_OF_TREE_KO),y)
ccflags-y += -DCONFIG_TMEMORY=1
ccflags-y += -DCONFIG_TMEMORY_CRYPTO=1
ccflags-y += -DCONFIG_TMEMORY_MIGRATION=1
ccflags-y += -DCONFIG_TMEMORY_SWITCH_TIME_DEFAULT=5000
tmemory-y += crypt.o
endif
