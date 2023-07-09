obj-y += sched_assist_common.o
obj-y += sched_assist_mutex.o
obj-y += sched_assist_rwsem.o
obj-y += sched_assist_futex.o
obj-y += sched_assist_workqueue.o
obj-$(CONFIG_LOCKING_PROTECT) += sched_assist_locking.o

obj-$(CONFIG_OPLUS_LOCKING_STRATEGY) += sync/locking_main.o
obj-$(CONFIG_OPLUS_LOCKING_STRATEGY) += sync/futex.o
obj-$(CONFIG_OPLUS_LOCKING_STRATEGY) += sync/sysfs.o

ifeq ($(VERSION).$(PATCHLEVEL), 5.4)
obj-y += sched_assist_slide.o
endif

obj-$(CONFIG_OPLUS_CPU_AUDIO_PERF) += sched_assist_audio.o
