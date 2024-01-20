load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_local_modules():

    define_oplus_ddk_module(
        name = "oplus_network_data_module",
        srcs = native.glob([
            "**/*.h",
            "data_module/data_main.c",
            "data_module/comm_netlink/comm_netlink.c",
            "data_module/comm_netlink/protobuf-c.c",
            "data_module/proto-src/netlink_msg.pb-c.c",
            "data_module/dpi/dpi_core.c",
            "data_module/dpi/log_stream.c",
            "data_module/dpi/tmgp_sgame.c",
            "data_module/dpi/heytap_market.c",
            "data_module/cls_dpi/cls_dpi.c",
            "data_module/dpi/zoom.c",
            "data_module/dpi/tencent_meeting.c",
            "data_module/dpi/wechat.c",
            "data_module/oplus_game_main_stream_monitor/oplus_game_main_stream_monitor.c",
            "tmgp_sgame/wzry_stats.c",
        ]),
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_network_linkpower_module",
        conditional_srcs = {
            "CONFIG_OPLUS_DDK_MTK" : {
                True: [
                    "linkpower_module/heartbeat_proxy/mediatek/heartbeat_proxy_mtk.c",
                    "linkpower_module/heartbeat_proxy/mediatek/mipc_hashmap.c",
                    "linkpower_module/heartbeat_proxy/mediatek/mipc_list.c",
                    "linkpower_module/heartbeat_proxy/mediatek/mipc_msg.c",
                    "linkpower_module/ccci_wakeup_hook/ccci_wakeup_hook.c"
                ],
                False: [
                    "linkpower_module/heartbeat_proxy/qualcomm/heartbeat_proxy_qcom.c",
                    "linkpower_module/qrtr_hook/qrtr_hook.c"
                ],
            }
        },
        srcs = native.glob([
            "**/*.h",
            "linkpower_module/linkpower_main.c",
            "linkpower_module/linkpower_netlink/linkpower_netlink.c",
            "linkpower_module/sk_pid_hook/sk_pid_hook.c",
        ]),
        conditional_defines = {
            "mtk": ["MTK_PLATFORM"],
            "qcom": ["QCOM_PLATFORM"],
        },
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_network_app_monitor",
        srcs = native.glob([
            "**/*.h",
            "oplus_apps_monitor/oplus_apps_monitor.c",
            "oplus_apps_monitor/oplus_apps_power_monitor.c",
        ]),
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_network_dns_hook",
        srcs = native.glob([
            "**/*.h",
            "oplus_dns_hook/oplus_dns_hook.c",
        ]),
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_network_game_first",
        srcs = native.glob([
            "**/*.h",
            "oplus_game_first/oplus_game_first.c",
        ]),
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_network_qr_scan",
        srcs = native.glob([
            "**/*.h",
            "oplus_qr_scan/oplus_qr_scan.c",
        ]),
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_network_score",
        srcs = native.glob([
            "**/*.h",
            "oplus_score/oplus_score.c",
        ]),
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_network_stats_calc",
        srcs = native.glob([
            "**/*.h",
            "oplus_stats_calc/oplus_stats_calc.c",
        ]),
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_network_vnet",
        srcs = native.glob([
            "**/*.h",
            "oplus_vnet/oplus_vnet.c",
        ]),
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_network_rf_cable_monitor",
        srcs = native.glob([
            "**/*.h",
            "oplus_rf_cable_monitor/oplus_rf_cable_monitor.c",
        ]),
        conditional_defines = {
            "qcom":  ["QCOM_PLATFORM"],
        },
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_network_oem_qmi",
        srcs = native.glob([
            "**/*.h",
            "oplus_network_oem_qmi/oem_qmi_client.c",
        ]),
        conditional_defines = {
            "qcom":  ["QCOM_PLATFORM"],
        },
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_network_esim",
        srcs = native.glob([
            "**/*.h",
            "oplus_network_esim/oplus_network_esim.c",
        ]),
        ko_deps = [
            "//vendor/oplus/kernel/network:oplus_network_oem_qmi",
        ],
        conditional_defines = {
            "qcom":  ["QCOM_PLATFORM"],
        },
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_network_sim_detect",
        srcs = native.glob([
            "**/*.h",
            "oplus_network_sim_detect/sim_detect.c",
        ]),
        ko_deps = [
            "//vendor/oplus/kernel/network:oplus_network_oem_qmi",
        ],
        conditional_defines = {
            "qcom":  ["QCOM_PLATFORM"],
        },
        includes = ["."],
    )

    ddk_headers(
        name = "config_headers",
        hdrs  = native.glob([
            "**/*.h",
        ]),
        includes = ["."],
    )

    ddk_copy_to_dist_dir(
        name = "oplus_network",
        module_list = [
            "oplus_network_data_module",
            "oplus_network_linkpower_module",
            "oplus_network_app_monitor",
            "oplus_network_dns_hook",
            "oplus_network_vnet",
            "oplus_network_game_first",
            "oplus_network_qr_scan",
            "oplus_network_score",
            "oplus_network_stats_calc",
            "oplus_network_rf_cable_monitor",
            "oplus_network_oem_qmi",
            "oplus_network_esim",
            "oplus_network_sim_detect",
        ],
    )
