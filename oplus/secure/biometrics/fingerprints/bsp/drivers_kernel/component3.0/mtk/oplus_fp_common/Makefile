obj-$(CONFIG_OPLUS_FINGERPRINT_COMMON) += oplus_fp_common.o
ccflags-$(CONFIG_OPLUS_FINGERPRINT_GKI_ENABLE) += -DCONFIG_OPLUS_FINGERPRINT_GKI_ENABLE

$(info [fp_bsp]LINUX_KERNEL_VERSION:$(LINUX_KERNEL_VERSION))

ifeq ($(CONFIG_OPLUS_FINGERPRINT_USE_PROC_OPS),y)
ccflags-y += -DUSE_PROC_OPS
endif

### Platform marco
ifeq ($(CONFIG_OPLUS_FINGERPRINT_QCOM),y)
ccflags-y += -DQCOM_PLATFORM
else ifeq ($(CONFIG_OPLUS_FINGERPRINT_MTK),y)
ccflags-y += -DMTK_PLATFORM
ccflags-y += -DFP_TEE_BINDE_CORE_ENABLE
ifeq ($(CONFIG_OPLUS_FINGERPRINT_GKI_ENABLE),y)
#tp for gki
ccflags-y += -I$(srctree)/../vendor/oplus/kernel/touchpanel/oplus_touchscreen_v2/touchpanel_notify
endif
endif
