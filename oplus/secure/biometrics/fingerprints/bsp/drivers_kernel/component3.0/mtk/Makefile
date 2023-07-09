obj-$(CONFIG_OPLUS_FINGERPRINT_COMMON) += oplus_fp_common/

### Capacitive fingerprint sensor
obj-$(CONFIG_OPLUS_FINGERPRINT_FPC) += fpc/

obj-$(CONFIG_OPLUS_FINGERPRINT_EGIS) += egis/

### Optical fingerprint sensor
ifneq ($(CONFIG_OPLUS_FINGERPRINT_GOODIX)_$(CONFIG_OPLUS_FINGERPRINT_GOODIX_OPTICAL), _)
obj-y += goodix_optical_fp/
endif
obj-$(CONFIG_OPLUS_FINGERPRINT_SILEAD_OPTICAL) += silead_optical_fp/
obj-$(CONFIG_OPLUS_FINGERPRINT_EGIS_OPTICAL)   += egis_optical_fp/
obj-$(CONFIG_OPLUS_FINGERPRINT_JIIOV_OPTICAL)  += anc_driver/
obj-$(CONFIG_OPLUS_FINGERPRINT_SILEAD_CAP)  += silead/

### FEATURE MACRO
ccflags-$(CONFIG_OPLUS_FINGERPRINT_NAVIGATION_ENABLE) += -DFP_NAVIGATION_ENABLE
