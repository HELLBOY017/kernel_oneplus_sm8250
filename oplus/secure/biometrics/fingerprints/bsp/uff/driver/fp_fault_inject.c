#include <linux/module.h>
#include "fp_driver.h"
#include "include/fingerprint_event.h"

static unsigned int g_fingerprint_msg_module_mask = 0;
static unsigned int g_fingerprint_msg_event_mask = 0;

void fault_inject_set_block_msg(unsigned int cmd)
{
    switch (cmd) {
    case FP_IOC_FAULT_INJECT_BLOCK_MSG_CLEAN:
        pr_info("%s FP_IOC_FAULT_INJECT_BLOCK_MSG_CLEAN\n", __func__);
        g_fingerprint_msg_module_mask = 0;
        g_fingerprint_msg_event_mask  = 0;
        break;
    case FP_IOC_FAULT_INJECT_BLOCK_MSG_UP:
        pr_info("%s FP_IOC_FAULT_INJECT_BLOCK_MSG_UP\n", __func__);
        g_fingerprint_msg_module_mask |= (0x1 << E_FP_TP);
        g_fingerprint_msg_event_mask  |= (0x1 << E_FP_EVENT_TP_TOUCHUP);
        break;
    case FP_IOC_FAULT_INJECT_BLOCK_MSG_DOWN:
        pr_info("%s FP_IOC_FAULT_INJECT_BLOCK_MSG_DOWN\n", __func__);
        g_fingerprint_msg_module_mask |= (0x1 << E_FP_TP);
        g_fingerprint_msg_event_mask  |= (0x1 << E_FP_EVENT_TP_TOUCHDOWN);
        break;
    case FP_IOC_FAULT_INJECT_BLOCK_MSG_UIREADY:
        pr_info("%s FP_IOC_FAULT_INJECT_BLOCK_MSG_UIREADY\n", __func__);
        g_fingerprint_msg_module_mask |= (0x1 << E_FP_LCD);
        g_fingerprint_msg_event_mask  |= (0x1 << E_FP_EVENT_UI_READY);
        break;
    default:
        break;
    }
}

void fault_inject_fp_msg_hook(struct fingerprint_message_t *fingerprint_msg, int *need_report)
{
    if (((0x1 << fingerprint_msg->module) & g_fingerprint_msg_module_mask) &&
        ((0x1 << fingerprint_msg->event) & g_fingerprint_msg_event_mask)) {
        *need_report = 0;
        pr_info("fault inject: message module:%d, event:%d, not allowed to be reported",
                fingerprint_msg->module, fingerprint_msg->event);
    }
}

