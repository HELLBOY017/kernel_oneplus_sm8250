obj-$(CONFIG_OPLUS_FEATURE_DATA_MODULE) += oplus_data_module.o

ccflags-y += -Wall -Werror

oplus_data_module-objs := data_main.o \
			comm_netlink/comm_netlink.o \
			comm_netlink/protobuf-c.o \
			proto-src/netlink_msg.pb-c.o \
			dpi/dpi_core.o \
			dpi/log_stream.o\
			dpi/tmgp_sgame.o \
			dpi/heytap_market.o \
			cls_dpi/cls_dpi.o \
			tmgp_sgame/wzry_stats.o
