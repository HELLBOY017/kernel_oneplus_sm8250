/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#ifndef __FP_FAULT_INJECT_H
#define __FP_FAULT_INJECT_H

void fault_inject_set_block_msg(unsigned int cmd);
void fault_inject_fp_msg_hook(struct fingerprint_message_t *fingerprint_msg, int *need_report);

#endif /* __FP_FAULT_INJECT_H */
