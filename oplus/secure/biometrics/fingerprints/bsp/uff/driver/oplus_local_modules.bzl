load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_local_modules():


    define_oplus_ddk_module(
        name = "oplus_bsp_uff_fp_driver",
        srcs = native.glob([
            "**/*.h",
            "fp_driver.c",
            "fp_platform.c",
            "fingerprint_event.c",
            "fp_health.c",
            "fp_netlink.c",
        ]),
        ko_deps = [
#            "//vendor/oplus/kernel/touchpanel/oplus_touchscreen_v2:oplus_bsp_tp_notify",	#built in-tree
        ],
        includes = ["."],
        conditional_defines = {
            "qcom":  ["QCOM_PLATFORM"],
            "mtk":   ["MTK_PLATFORM"],
        },
        local_defines = ["CONFIG_OPLUS_FINGERPRINT_GKI_ENABLE","CONFIG_TOUCHPANEL_NOTIFY"],
        header_deps = [
            "//vendor/oplus/kernel/touchpanel/oplus_touchscreen_v2:config_headers",
        ],
    )
    ddk_copy_to_dist_dir(
        name = "oplus_bsp_fp",
        module_list = [
            "oplus_bsp_uff_fp_driver",
        ],
    )
