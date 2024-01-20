#ifndef __OPLUS_CHG_DUAL_CHAN_H__
#define __OPLUS_CHG_DUAL_CHAN_H__

#define OPLUS_DUAL_CHAN_CHECK_MAX 10
#define DUAL_CHAN_INFO_COUNT 4
#define DUAL_CHAN_PAGE_SIZE 64

enum dual_chan_topic_item {
	SWITCH_ITEM_DUAL_CHAN_STATUS,
};

enum dual_chan_err_type {
	DUAL_CHAN_EXIT_ERR = 1,
	DUAL_CHAN_ENTER_ERR,
};


void oplus_dual_chan_buck_reset(void);
int oplus_get_dual_chan_info(char *buf);
bool oplus_get_dual_chan_support(void);
void oplus_dual_chan_curr_vote_reset(void);

#endif /* __OPLUS_CHG_DUAL_CHAN_H__ */
