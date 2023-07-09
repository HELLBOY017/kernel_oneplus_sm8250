/***********************************************************
** Copyright (C), 2008-2022, oplus Mobile Comm Corp., Ltd.
** File: ring_data.h
** Description: ring data lib
**
** Version: 1.0
** Date : 2022/7/8
** Author: ShiQianhua
**
** ------------------ Revision History:------------------------
** <author> <data> <version > <desc>
** shiqianhua 2022/7/8 1.0 build this module
****************************************************************/
#ifndef __RING_DATA_H__
#define __RING_DATA_H__

typedef struct {
	void *data;
	int item_len;
	int size;
	int cur_idx;
	int total_size;
	int used;
} ring_data_t;

#define RING_DATA_STRUCT(name, pdata, pitem_len, psize) \
	ring_data_t name = {.data = pdata, .item_len = pitem_len, .size = psize, .cur_idx = 0, .total_size = 0, .used = 0}

/*
static void ring_data_init(ring_data_t *ring, void *data, int item_len, int size)
{
	ring->data = data;
	ring->item_len = item_len;
	ring->size = size;
	ring->cur_idx = 0;
	ring->total_size = 0;
}
*/

static inline void *ring_get_current_item(ring_data_t *ring_data)
{
	if (!ring_data->used) {
		return NULL;
	}
	return ((char *)ring_data->data + (ring_data->item_len * ring_data->cur_idx));
}

static inline void *ring_get_next_item(ring_data_t *ring_data)
{
	if (!ring_data->used) {
		ring_data->used = 1;
		ring_data->cur_idx = 0;
		ring_data->total_size = 1;
		return (void *)(ring_data->data);
	}

	if (ring_data->cur_idx >= (ring_data->size - 1)) {
		ring_data->cur_idx = 0;
	} else {
		ring_data->cur_idx += 1;
	}
	if (ring_data->total_size < ring_data->size) {
		ring_data->total_size += 1;
	}
	return (void *)((char *)(ring_data->data) + (ring_data->item_len *ring_data->cur_idx));
}

static inline int ring_copy_by_order(void *out, ring_data_t *ring_data)
{
	int i = 0;
	int idx = ring_data->cur_idx;

	if (!ring_data->used) {
		return 0;
	}

	for(i = 0; i < ring_data->total_size; i++) {
		char *p = (char *)ring_data->data + (idx * ring_data->item_len);
		memcpy((char *)out + (i * ring_data->item_len), p, ring_data->item_len);
		if (idx == 0) {
			idx = ring_data->size - 1;
		} else {
			idx--;
		}
	}

	return ring_data->total_size;
}


#endif  /* __RING_DATA_H__ */
