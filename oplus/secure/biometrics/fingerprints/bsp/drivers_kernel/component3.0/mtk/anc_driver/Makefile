ccflags-$(CONFIG_OPLUS_FINGERPRINT_GKI_ENABLE) += -DCONFIG_OPLUS_FINGERPRINT_GKI_ENABLE

ifeq ($(CONFIG_OPLUS_FINGERPRINT_MTK),y)
#add for gki mtk lcd uiready
ccflags-$(CONFIG_DRM_MEDIATEK_V2) += -I$(srctree)/drivers/gpu/drm/mediatek/mediatek_v2/
ccflags-$(CONFIG_DRM_MEDIATEK_V2) += -DCONFIG_DRM_MEDIATEK_V2

jiiov_mtk-y := jiiov_platform_mtk.o jiiov_netlink.o
obj-m += jiiov_mtk.o
else
obj-$(CONFIG_OPPO_FINGERPRINT_JIIOV_OPTICAL) += jiiov_qcom.o
jiiov_qcom-y := jiiov_platform.o jiiov_netlink.o
endif
