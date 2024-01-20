# project: oplus/kernel/cpu
obj-y += sched_assist_common.o
obj-y += sched_assist_workqueue.o
#ifdef CONFIG_OPLUS_FEATURE_CPU_JANKINFO
obj-y += sa_jankinfo.o
#endif
obj-$(CONFIG_LOCKING_PROTECT) += sched_assist_locking.o

ifeq ($(VERSION).$(PATCHLEVEL), 5.4)
obj-y += sched_assist_slide.o
endif

obj-$(CONFIG_OPLUS_CPU_AUDIO_PERF) += sched_assist_audio.o

# project: oplus/kernel/synchronize
obj-$(CONFIG_OPLUS_LOCKING_STRATEGY) += oplus_locking_strategy.o
oplus_locking_strategy-y += sync/locking_main.o
oplus_locking_strategy-y += sync/futex.o
oplus_locking_strategy-y += sync/sysfs.o
oplus_locking_strategy-y += sync/mutex.o
oplus_locking_strategy-y += sync/rwsem.o
oplus_locking_strategy-$(CONFIG_OPLUS_LOCKING_MONITOR) += sync/kern_lock_stat.o
obj-$(CONFIG_OPLUS_FEATURE_EAS_OPT) += eas_opt/eas_opt.o
obj-$(CONFIG_OPLUS_FEATURE_VT_CAP) += eas_opt/oplus_cap.o
obj-$(CONFIG_OPLUS_CPUFREQ_IOWAIT_PROTECT) += eas_opt/oplus_iowait.o
