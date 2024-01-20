ifeq ($(CONFIG_OPLUS_CHARGER_OPTIGA),y)
subdir-ccflags-y += -I$(srctree)/drivers/gpio
subdir-ccflags-y += -I$(srctree)/drivers/pinctrl
subdir-ccflags-y += -I$(srctree)/drivers/pinctrl/qcom
endif

obj-y += oplus_ufcs_protocol.o
obj-y += ufcs_ic/oplus_sc8547a.o
obj-y += ufcs_ic/oplus_sc2201.o
obj-y += ufcs_ic/oplus_nu2112a.o
obj-y += ufcs_ic/oplus_nu2112a_slave.o
