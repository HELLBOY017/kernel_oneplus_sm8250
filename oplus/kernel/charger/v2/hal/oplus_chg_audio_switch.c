// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2021-2023 Oplus. All rights reserved.
 */
#if IS_ENABLED(CONFIG_TCPC_CLASS)
#include <tcpci.h>
#include <tcpm.h>
#endif
#define OPLUS_AUDIO_SET_MAX_TIME        (50) /* wait at most 250 ms*/
#define OPLUS_AUDIO_GET_MAX_TIME        (50)
#define OPLUS_AUDIO_WAIT_TIME           (5)

enum TYPEC_AUDIO_SWITCH_STATE {
	TYPEC_AUDIO_SWITCH_STATE_DPDM		= 0x0,
	TYPEC_AUDIO_SWITCH_STATE_FAST_CHG	= 0x1,
	TYPEC_AUDIO_SWITCH_STATE_AUDIO		= 0x1 << 1,
	TYPEC_AUDIO_SWITCH_STATE_UNKNOW		= 0x1 << 2,
	TYPEC_AUDIO_SWITCH_STATE_MASK		= 0x7,
	TYPEC_AUDIO_SWITCH_STATE_SUPPORT	= 0x1 << 4,
	TYPEC_AUDIO_SWITCH_STATE_NO_RAM		= 0x1 << 4,
	TYPEC_AUDIO_SWITCH_STATE_I2C_ERR	= 0x1 << 8,
	TYPEC_AUDIO_SWITCH_STATE_INVALID_PARAM  = 0x1 << 9,
};

struct audio_switch {
	int audio_switch_state;
	unsigned long audio_switch_cnt;
	unsigned long audio_switch_ack_cnt;
	struct mutex audio_switch_lock;
};

static struct audio_switch g_audio_switch;

#if IS_ENABLED(CONFIG_TCPC_CLASS)
#define AUDIO_SWITCH_BY_TYPEC_NOTIFY 1
#endif

#if IS_ENABLED(CONFIG_OPLUS_AUDIO_SWITCH_GLINK)
static RAW_NOTIFIER_HEAD(chg_glink_notifier);
#endif

#if IS_ENABLED(CONFIG_SND_SOC_OPLUS_TYPEC_SWITCH)
extern int typec_switch_status0(void);
extern int typec_switch_to_fast_charger(int to_fast_charger);
#endif

__maybe_unused static void lock_audio_switch(void)
{
	mutex_lock(&g_audio_switch.audio_switch_lock);
}

__maybe_unused static void unlock_audio_switch(void)
{
	mutex_unlock(&g_audio_switch.audio_switch_lock);
}
__maybe_unused static bool oplus_set_audio_switch_callback(int state)
{
	lock_audio_switch();
	g_audio_switch.audio_switch_state = state;
	g_audio_switch.audio_switch_cnt++;
	unlock_audio_switch();

	chg_info("audio_switch_state is %d, audio_switch_cnt = %ld\n",
			state, g_audio_switch.audio_switch_cnt);

	return true;
}

__maybe_unused static bool oplus_get_audio_switch_callback(int state)
{
	lock_audio_switch();
	g_audio_switch.audio_switch_state = state;
	g_audio_switch.audio_switch_ack_cnt++;
	unlock_audio_switch();

	chg_info("audio_switch_state is %d, audio_switch_ack_cnt = %ld\n",
			state, g_audio_switch.audio_switch_ack_cnt);

	return true;
}

static void audio_switch_init(void)
{
	static bool inited = false;

	if (!inited) {
		mutex_init(&g_audio_switch.audio_switch_lock);
		g_audio_switch.audio_switch_state = 0;
		g_audio_switch.audio_switch_cnt = 0;
		g_audio_switch.audio_switch_ack_cnt = 0;
		inited = true;
	}
}

static int oplus_set_audio_switch_status(int state)
{
#ifdef AUDIO_SWITCH_BY_TYPEC_NOTIFY
	int ret = 0;
	int wait = 0;
	unsigned long switch_cnt = 0;

#if IS_ENABLED(CONFIG_OPLUS_AUDIO_SWITCH_GLINK)
	switch_cnt = g_audio_switch.audio_switch_cnt;
	ret = raw_notifier_call_chain(&chg_glink_notifier, state, NULL);
	if (ret)
		oplus_set_audio_switch_callback(state);
#else
	struct tcpc_device *tcpc = tcpc_dev_get_by_name("type_c_port0");
	if (tcpc == NULL) {
		chg_err("get type_c_port0 fail\n");
		return -1;
	}
	switch_cnt = g_audio_switch.audio_switch_cnt;
	ret = tcpci_notify_switch_set_state(tcpc, state, &oplus_set_audio_switch_callback);
#endif
	while (wait < OPLUS_AUDIO_SET_MAX_TIME) {
		if (switch_cnt < g_audio_switch.audio_switch_cnt) {
			chg_info("wait = %d ms complete!\n", wait);
			break;
		}
		wait++;
		mdelay(OPLUS_AUDIO_WAIT_TIME);
	}

	if (wait >= OPLUS_AUDIO_SET_MAX_TIME) {
		ret = -1;
		chg_info("wait = %d > MAXT_TIME, time out, set failed!\n", wait);
	}

	return ret;
#else
#if IS_ENABLED(CONFIG_SND_SOC_OPLUS_TYPEC_SWITCH)
	return typec_switch_to_fast_charger(state);
#else
	return 0; /*TODO: not realized now.*/
#endif
#endif
}

static int oplus_get_audio_switch_status(void)
{
#ifdef AUDIO_SWITCH_BY_TYPEC_NOTIFY
	int ret = 0;
	int wait = 0;
	unsigned long switch_cnt = 0;

#if IS_ENABLED(CONFIG_OPLUS_AUDIO_SWITCH_GLINK)
	switch_cnt = g_audio_switch.audio_switch_ack_cnt;
	ret = raw_notifier_call_chain(&chg_glink_notifier, TYPEC_AUDIO_SWITCH_STATE_AUDIO, NULL);
	if (ret != TYPEC_AUDIO_SWITCH_STATE_INVALID_PARAM)
		oplus_get_audio_switch_callback(ret);
#else
	struct tcpc_device *tcpc = tcpc_dev_get_by_name("type_c_port0");
	if (tcpc == NULL) {
		chg_err("get type_c_port0 fail\n");
		return -1;
	}
	switch_cnt = g_audio_switch.audio_switch_ack_cnt;
	ret = tcpci_notify_switch_get_state(tcpc, &oplus_get_audio_switch_callback);
#endif
	while (wait < OPLUS_AUDIO_GET_MAX_TIME) {
		if (switch_cnt < g_audio_switch.audio_switch_ack_cnt) {
			chg_debug("get the audio_state = %d, wait = %d ms, audio_switch_ack_cnt = %ld!\n",
					wait, g_audio_switch.audio_switch_state, g_audio_switch.audio_switch_ack_cnt);
			ret = g_audio_switch.audio_switch_state;
			break;
		}
		wait++;
		mdelay(OPLUS_AUDIO_WAIT_TIME);
	}

	if (wait >= OPLUS_AUDIO_GET_MAX_TIME) {
		ret = -1;
		chg_err("wait = %d > MAXT_TIME, time out, get failed!\n", wait);
	}
	return ret;
#else
#if IS_ENABLED(CONFIG_SND_SOC_OPLUS_TYPEC_SWITCH)
	return typec_switch_status0();
#else
	return 0; /*TODO: not realized now.*/
#endif
#endif
}

