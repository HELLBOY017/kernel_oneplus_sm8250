obj-$(CONFIG_OPLUS_CPU_FREQ_GOV_UAG) += cpufreq_uag.o
cpufreq_uag-y += cpufreq_uag_main.o
cpufreq_uag-$(CONFIG_UAG_NONLINEAR_FREQ_CTL) += nonlinear_opp_cap.o
cpufreq_uag-$(CONFIG_OPLUS_UAG_AMU_AWARE) += stall_util_cal.o
cpufreq_uag-$(CONFIG_OPLUS_FEATURE_TOUCH_BOOST) += touch_boost.o

obj-$(CONFIG_UA_KERNEL_CPU_IOCTL) += ua_cpu_ioctl.o
ua_cpu_ioctl-y += ua_ioctl/ua_ioctl_common.o
ua_cpu_ioctl-$(CONFIG_OPLUS_FEATURE_FRAME_BOOST) += ua_ioctl/frame_ioctl.o
ua_cpu_ioctl-y += ua_ioctl/touch_ioctl.o

