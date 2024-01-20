ufcs_class-y := ufcs_core.o
ufcs_class-y += ufcs_event.o
ufcs_class-y += ufcs_event_ctrl.o
ufcs_class-y += ufcs_event_data.o
ufcs_class-y += ufcs_event_vendor.o
ufcs_class-y += ufcs_intf.o
ufcs_class-y += ufcs_notify.o
ufcs_class-y += ufcs_policy_engine.o
ufcs_class-y += ufcs_pe_cable.o
ufcs_class-y += ufcs_pe_vendor.o
ufcs_class-y += ufcs_pe_test.o
ufcs_class-y += ufcs_sha256.o
ufcs_class-y += ufcs_timer.o

ccflags-y += -I$(srctree)/$(src)/include
ccflags-y += -I$(srctree)/$(src)/include/internal

obj-$(CONFIG_OPLUS_UFCS_CLASS) += ufcs_class.o
