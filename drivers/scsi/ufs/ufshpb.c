/*
 * Universal Flash Storage Host Performance Booster
 *
 * Copyright (C) 2017-2018 Samsung Electronics Co., Ltd.
 * Copyright (C) 2020 Oplus. All rights reserved.
 *
 * Authors:
 *	Yongmyung Lee <ymhungry.lee@samsung.com>
 *	Jinyoung Choi <j-young.choi@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * See the COPYING file in the top-level directory or visit
 * <http://www.gnu.org/licenses/gpl-2.0.html>
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * This program is provided "AS IS" and "WITH ALL FAULTS" and
 * without warranty of any kind. You are solely responsible for
 * determining the appropriateness of using and distributing
 * the program and assume all risks associated with your exercise
 * of rights with respect to the program, including but not limited
 * to infringement of third party rights, the risks and costs of
 * program errors, damage to or loss of data, programs or equipment,
 * and unavailability or interruption of operations. Under no
 * circumstances will the contributor of this Program be liable for
 * any damages of any kind arising from your use or distribution of
 * this program.
 *
 * The Linux Foundation chooses to take subject only to the GPLv2
 * license terms, and distributes only under these terms.
 */

#include "ufshcd.h"
#include "ufshpb.h"

#define UFSHCD_REQ_SENSE_SIZE	18

static int ufshpb_create_sysfs(struct ufsf_feature *ufsf,
			       struct ufshpb_lu *hpb);
static void ufshpb_remove_sysfs(struct ufshpb_lu *hpb);

static int create_hpbfn_enable_proc(void);
static void remove_hpbfn_enable_proc(void);

static inline void
ufshpb_get_pos_from_lpn(struct ufshpb_lu *hpb, unsigned long lpn, int *rgn_idx,
			int *srgn_idx, int *offset)
{
	int rgn_offset;

	*rgn_idx = lpn >> hpb->entries_per_rgn_shift;
	rgn_offset = lpn & hpb->entries_per_rgn_mask;
	*srgn_idx = rgn_offset >> hpb->entries_per_srgn_shift;
	*offset = rgn_offset & hpb->entries_per_srgn_mask;
}

inline int ufshpb_valid_srgn(struct ufshpb_region *rgn,
			     struct ufshpb_subregion *srgn)
{
	return rgn->rgn_state != HPB_RGN_INACTIVE &&
		srgn->srgn_state == HPB_SRGN_CLEAN;
}

/* Must be held hpb_lock  */
static bool ufshpb_ppn_dirty_check(struct ufshpb_lu *hpb, unsigned long lpn,
				   int transfer_len)
{
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	unsigned long cur_lpn = lpn;
	int rgn_idx, srgn_idx, srgn_offset, find_size;
	int scan_cnt = transfer_len;

	do {
		ufshpb_get_pos_from_lpn(hpb, cur_lpn, &rgn_idx, &srgn_idx,
					&srgn_offset);
		rgn = hpb->rgn_tbl + rgn_idx;
		srgn = rgn->srgn_tbl + srgn_idx;

		if (!ufshpb_valid_srgn(rgn, srgn))
			return true;

		if (unlikely(!srgn->mctx || !srgn->mctx->ppn_dirty))
			return true;

		if (hpb->entries_per_srgn < srgn_offset + scan_cnt) {
			int cnt = hpb->entries_per_srgn - srgn_offset;

			find_size = hpb->entries_per_srgn;
			scan_cnt -= cnt;
			cur_lpn += cnt;
		} else {
			find_size = srgn_offset + scan_cnt;
			scan_cnt = 0;
		}

		srgn_offset =
			find_next_bit((unsigned long *)srgn->mctx->ppn_dirty,
				      hpb->entries_per_srgn, srgn_offset);

		if (srgn_offset < hpb->entries_per_srgn)
			return srgn_offset < find_size;

	} while (scan_cnt);

	return false;
}

static void ufshpb_set_read16_cmd(struct ufshpb_lu *hpb,
				  struct ufshcd_lrb *lrbp,
				  u64 ppn,
				  unsigned int transfer_len)
{
	unsigned char *cdb = lrbp->cmd->cmnd;

	cdb[0] = READ_16;
	cdb[2] = lrbp->cmd->cmnd[2];
	cdb[3] = lrbp->cmd->cmnd[3];
	cdb[4] = lrbp->cmd->cmnd[4];
	cdb[5] = lrbp->cmd->cmnd[5];
	cdb[6] = GET_BYTE_7(ppn);
	cdb[7] = GET_BYTE_6(ppn);
	cdb[8] = GET_BYTE_5(ppn);
	cdb[9] = GET_BYTE_4(ppn);
	cdb[10] = GET_BYTE_3(ppn);
	cdb[11] = GET_BYTE_2(ppn);
	cdb[12] = GET_BYTE_1(ppn);
	cdb[13] = GET_BYTE_0(ppn);

	if (lrbp->hpb_ctx_id < MAX_HPB_CONTEXT_ID)
		cdb[14] = (1 << 7) | lrbp->hpb_ctx_id;
	else
		cdb[14] = UFSHPB_GROUP_NUMBER;

	cdb[15] = transfer_len;

	lrbp->cmd->cmd_len = MAX_CDB_SIZE;
}

/* called with hpb_lock (irq) */
static inline void
ufshpb_set_dirty_bits(struct ufshpb_lu *hpb, struct ufshpb_region *rgn,
		      struct ufshpb_subregion *srgn, int dword, int offset,
		      unsigned int cnt)
{
	const unsigned long mask = ((1UL << cnt) - 1) & 0xffffffff;

	if (rgn->rgn_state == HPB_RGN_INACTIVE)
		return;

	BUG_ON(!srgn->mctx);
	srgn->mctx->ppn_dirty[dword] |= (mask << offset);
}

static inline void ufshpb_get_bit_offset(struct ufshpb_lu *hpb, int srgn_offset,
					 int *dword, int *offset)
{
	*dword = srgn_offset >> bits_per_dword_shift;
	*offset = srgn_offset & bits_per_dword_mask;
}

static void ufshpb_set_dirty(struct ufshpb_lu *hpb, struct ufshcd_lrb *lrbp,
			     int rgn_idx, int srgn_idx, int srgn_offset)
{
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	int cnt, bit_cnt, bit_dword, bit_offset;

	cnt = blk_rq_sectors(lrbp->cmd->request) >> sects_per_blk_shift;
	ufshpb_get_bit_offset(hpb, srgn_offset, &bit_dword, &bit_offset);

	do {
		bit_cnt = min(cnt, BITS_PER_DWORD - bit_offset);

		rgn = hpb->rgn_tbl + rgn_idx;
		srgn = rgn->srgn_tbl + srgn_idx;

		ufshpb_set_dirty_bits(hpb, rgn, srgn, bit_dword, bit_offset,
				      bit_cnt);

		bit_offset = 0;
		bit_dword++;

		if (bit_dword == hpb->dwords_per_srgn) {
			bit_dword = 0;
			srgn_idx++;

			if (srgn_idx == hpb->srgns_per_rgn) {
				srgn_idx = 0;
				rgn_idx++;
			}
		}
		cnt -= bit_cnt;
	} while (cnt);

	BUG_ON(cnt < 0);
}

static inline bool ufshpb_is_read_cmd(struct scsi_cmnd *cmd)
{
	if (cmd->cmnd[0] == READ_10 || cmd->cmnd[0] == READ_16)
		return true;

	return false;
}

static inline bool ufshpb_is_write_discard_lrbp(struct ufshcd_lrb *lrbp)
{
	if (lrbp->cmd->cmnd[0] == WRITE_10 || lrbp->cmd->cmnd[0] == WRITE_16 ||
	    lrbp->cmd->cmnd[0] == UNMAP)
		return true;

	return false;
}

static u64 ufshpb_get_ppn(struct ufshpb_map_ctx *mctx, int pos, int *error)
{
	u64 *ppn_table;
	struct page *page = NULL;
	int index, offset;

	index = pos / HPB_ENTREIS_PER_OS_PAGE;
	offset = pos % HPB_ENTREIS_PER_OS_PAGE;

	page = mctx->m_page[index];
	if (unlikely(!page)) {
		*error = -ENOMEM;
		ERR_MSG("mctx %p cannot get m_page", mctx);
		return 0;
	}

	ppn_table = page_address(page);
	if (unlikely(!ppn_table)) {
		*error = -ENOMEM;
		ERR_MSG("mctx %p cannot get ppn_table vm", mctx);
		return 0;
	}

	return ppn_table[offset];
}

#if defined(CONFIG_HPB_ERR_INJECTION)
static u64 ufshpb_get_bitflip_ppn(struct ufshpb_lu *hpb, u64 ppn)
{
	return (ppn ^ hpb->err_injection_bitflip);
}

static u64 ufshpb_get_offset_ppn(struct ufshpb_lu *hpb, u64 ppn)
{
	return (ppn + hpb->err_injection_offset);
}

static u64 ufshpb_get_random_ppn(struct ufshpb_lu *hpb, u64 ppn)
{
	u64 random_ppn = 0;

	if (!hpb->err_injection_random)
		return ppn;

	get_random_bytes(&random_ppn, sizeof(random_ppn));

	return random_ppn;
}

static u64 ufshpb_get_err_injection_ppn(struct ufshpb_lu *hpb, u64 ppn)
{
	u64 new_ppn = 0;

	if (hpb->err_injection_select == HPB_ERR_INJECTION_BITFLIP)
		new_ppn = ufshpb_get_bitflip_ppn(hpb, ppn);
	else if (hpb->err_injection_select == HPB_ERR_INJECTION_OFFSET)
		new_ppn = ufshpb_get_offset_ppn(hpb, ppn);
	else if (hpb->err_injection_select == HPB_ERR_INJECTION_RANDOM)
		new_ppn = ufshpb_get_random_ppn(hpb, ppn);

	return new_ppn;
}
#endif

inline int ufshpb_get_state(struct ufsf_feature *ufsf)
{
	return atomic_read(&ufsf->hpb_state);
}

inline void ufshpb_set_state(struct ufsf_feature *ufsf, int state)
{
	atomic_set(&ufsf->hpb_state, state);
}

static inline int ufshpb_lu_get(struct ufshpb_lu *hpb)
{
	if (!hpb || ufshpb_get_state(hpb->ufsf) != HPB_PRESENT)
		return -ENODEV;

	kref_get(&hpb->ufsf->hpb_kref);
	return 0;
}

static inline void ufshpb_schedule_error_handler(struct kref *kref)
{
	struct ufsf_feature *ufsf;

	ufsf = container_of(kref, struct ufsf_feature, hpb_kref);
	schedule_work(&ufsf->hpb_eh_work);
}

static inline void ufshpb_lu_put(struct ufshpb_lu *hpb)
{
	kref_put(&hpb->ufsf->hpb_kref, ufshpb_schedule_error_handler);
}

static void ufshpb_failed(struct ufshpb_lu *hpb, const char *f)
{
	ERR_MSG("ufshpb_driver failed. function (%s)", f);
	ufshpb_set_state(hpb->ufsf, HPB_FAILED);
	ufshpb_lu_put(hpb);
}

static inline void ufshpb_put_pre_req(struct ufshpb_lu *hpb,
				      struct ufshpb_req *pre_req)
{
	list_add_tail(&pre_req->list_req, &hpb->lh_pre_req_free);
	hpb->num_inflight_pre_req--;
}

static struct ufshpb_req *ufshpb_get_pre_req(struct ufshpb_lu *hpb)
{
	struct ufshpb_req *pre_req;

	if (hpb->num_inflight_pre_req >= hpb->throttle_pre_req) {
		HPB_DEBUG(hpb, "pre_req throttle. inflight %d throttle %d",
			  hpb->num_inflight_pre_req, hpb->throttle_pre_req);
		return NULL;
	}

	pre_req = list_first_entry_or_null(&hpb->lh_pre_req_free,
					   struct ufshpb_req,
					   list_req);
	if (!pre_req) {
		HPB_DEBUG(hpb, "There is no pre_req");
		return NULL;
	}

	list_del_init(&pre_req->list_req);
	hpb->num_inflight_pre_req++;

	return pre_req;
}

static void ufshpb_pre_req_compl_fn(struct request *req, blk_status_t error)
{
	struct ufshpb_req *pre_req = (struct ufshpb_req *)req->end_io_data;
	struct ufshpb_lu *hpb = pre_req->hpb;
	unsigned long flags;
	struct scsi_sense_hdr sshdr;

	if (error) {
		ERR_MSG("block status %d", error);
		scsi_normalize_sense(pre_req->sense, SCSI_SENSE_BUFFERSIZE,
				     &sshdr);
		ERR_MSG("code %x sense_key %x asc %x ascq %x",
			sshdr.response_code,
			sshdr.sense_key, sshdr.asc, sshdr.ascq);
		ERR_MSG("byte4 %x byte5 %x byte6 %x additional_len %x",
			sshdr.byte4, sshdr.byte5,
			sshdr.byte6, sshdr.additional_length);
	}

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	ufshpb_put_pre_req(pre_req->hpb, pre_req);
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);

	ufshpb_lu_put(pre_req->hpb);
}

static int ufshpb_prep_entry(struct ufshpb_req *pre_req, struct page *page)
{
	struct ufshpb_lu *hpb = pre_req->hpb;
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	u64 *addr;
	u64 entry_ppn = 0;
	unsigned long lpn = pre_req->wb.lpn;
	int rgn_idx, srgn_idx, srgn_offset;
	int i, error = 0;
	unsigned long flags;

	addr = page_address(page);

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	for (i = 0; i < pre_req->wb.len; i++, lpn++) {
		ufshpb_get_pos_from_lpn(hpb, lpn, &rgn_idx, &srgn_idx,
					&srgn_offset);

		rgn = hpb->rgn_tbl + rgn_idx;
		srgn = rgn->srgn_tbl + srgn_idx;

		if (!ufshpb_valid_srgn(rgn, srgn))
			goto mctx_error;

		BUG_ON(!srgn->mctx);

		entry_ppn = ufshpb_get_ppn(srgn->mctx, srgn_offset, &error);
		if (error)
			goto mctx_error;

#if defined(CONFIG_HPB_ERR_INJECTION)
		if (hpb->err_injection_select != HPB_ERR_INJECTION_DISABLE)
			entry_ppn =
				ufshpb_get_err_injection_ppn(hpb, entry_ppn);
#endif

		addr[i] = entry_ppn;
	}
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	return 0;
mctx_error:
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	return -ENOMEM;
}

static int ufshpb_pre_req_add_bio_page(struct request_queue *q,
				       struct ufshpb_req *pre_req)
{
	struct page *page = pre_req->wb.m_page;
	struct bio *bio = pre_req->bio;
	int ret;

	BUG_ON(!page);

	bio_reset(bio);

	ret = ufshpb_prep_entry(pre_req, page);
	if (ret)
		return ret;

	ret = bio_add_pc_page(q, bio, page, OS_PAGE_SIZE, 0);
	if (ret != OS_PAGE_SIZE) {
		ERR_MSG("bio_add_pc_page fail: %d", ret);
		return -ENOMEM;
	}

	return 0;
}

static void ufshpb_init_cmd_errh(struct scsi_cmnd *cmd)
{
	cmd->serial_number = 0;
	scsi_set_resid(cmd, 0);
	memset(cmd->sense_buffer, 0, SCSI_SENSE_BUFFERSIZE);
	if (cmd->cmd_len == 0)
		cmd->cmd_len = scsi_command_size(cmd->cmnd);
}

static void ufshpb_pre_req_done(struct scsi_cmnd *cmd)
{
	blk_complete_request(cmd->request);
}

static inline unsigned long ufshpb_get_lpn(struct request *rq)
{
	return blk_rq_pos(rq) / SECTORS_PER_BLOCK;
}

static inline unsigned int ufshpb_get_len(struct request *rq)
{
	return blk_rq_sectors(rq) / SECTORS_PER_BLOCK;
}

static inline unsigned int ufshpb_is_unaligned(struct request *rq)
{
	return blk_rq_sectors(rq) % SECTORS_PER_BLOCK;
}

static inline int ufshpb_issue_ctx_id_ticket(struct ufshpb_lu *hpb)
{
	int hpb_ctx_id;

	hpb->ctx_id_ticket++;
	if (hpb->ctx_id_ticket >= MAX_HPB_CONTEXT_ID)
		hpb->ctx_id_ticket = 0;
	hpb_ctx_id = hpb->ctx_id_ticket;

	return hpb_ctx_id;
}

static inline void ufshpb_set_write_buf_cmd(unsigned char *cdb,
					    unsigned long lpn, unsigned int len,
					    int hpb_ctx_id)
{
	int len_byte = len * HPB_ENTRY_SIZE;

	cdb[0] = UFSHPB_WRITE_BUFFER;
	cdb[1] = UFSHPB_WRITE_BUFFER_ID;
	cdb[2] = GET_BYTE_3(lpn);
	cdb[3] = GET_BYTE_2(lpn);
	cdb[4] = GET_BYTE_1(lpn);
	cdb[5] = GET_BYTE_0(lpn);
	cdb[6] = (1 << 7) | hpb_ctx_id;
	cdb[7] = GET_BYTE_1(len_byte);
	cdb[8] = GET_BYTE_0(len_byte);
	cdb[9] = 0x00;	/* Control = 0x00 */
}

static void ufshpb_mimic_scsi_release_buffers(struct scsi_cmnd *cmd)
{
	if (cmd->sdb.table.nents)
		sg_free_table_chained(&cmd->sdb.table, false);

	memset(&cmd->sdb, 0, sizeof(cmd->sdb));

	if (scsi_prot_sg_count(cmd))
		sg_free_table_chained(&cmd->prot_sdb->table, false);
}

static inline void ufshpb_mimic_scsi_dispatch_cmd(struct scsi_cmnd *cmd)
{
	atomic_inc(&cmd->device->iorequest_cnt);

	scsi_log_send(cmd);

	cmd->scsi_done = ufshpb_pre_req_done;
}

static int ufshpb_mimic_scsi_request_fn(struct ufshpb_lu *hpb,
					struct request *req)
{
	struct request_queue *q = req->q;
	struct scsi_device *sdev = q->queuedata;
	struct Scsi_Host *shost = sdev->host;
	struct scsi_cmnd *cmd;
	unsigned long flags;
	unsigned int busy;
	int ret = 0;

	spin_lock_irqsave(q->queue_lock, flags);
	req->rq_flags |= RQF_STARTED;

	ret = q->prep_rq_fn(q, req);
	if (unlikely(ret != BLKPREP_OK)) {
		HPB_DEBUG(hpb, "scsi_prep_fn is fail");
		ret = -EIO;
		goto prep_err;
	}
	cmd = req->special;
	if (unlikely(!cmd))
		BUG();

	busy = atomic_inc_return(&sdev->device_busy) - 1;
	if (busy >= sdev->queue_depth) {
		ret = -EAGAIN;
		goto finish_cmd;
	}

	/* lh_pre_req_free list is dummy head for blk_dequeue_request() */
	list_add_tail(&req->queuelist, &hpb->lh_pre_req_dummy);
	ret = blk_queue_start_tag(q, req);
	if (ret) {
		list_del_init(&req->queuelist);
		ret = -EAGAIN;
		goto finish_cmd;
	}
	spin_unlock_irqrestore(q->queue_lock, flags);

	/*
	 * UFS device has multi luns, so starget is not used.
	 * In case of UFS, starget->can_queue <= 0.
	 */
	if (unlikely(scsi_target(sdev)->can_queue > 0))
		atomic_inc(&scsi_target(sdev)->target_busy);
	atomic_inc(&shost->host_busy);

	ufshpb_init_cmd_errh(cmd);

	ufshpb_mimic_scsi_dispatch_cmd(cmd);

	return ret;
finish_cmd:
	ufshpb_mimic_scsi_release_buffers(cmd);
	scsi_put_command(cmd);
	put_device(&sdev->sdev_gendev);
	req->special = NULL;
	atomic_dec(&sdev->device_busy);
prep_err:
	spin_unlock_irqrestore(q->queue_lock, flags);
	return ret;
}

static int ufshpb_set_pre_req(struct ufshpb_lu *hpb, struct scsi_cmnd *cmd,
			      struct ufshpb_req *pre_req, int hpb_ctx_id)
{
	struct scsi_device *sdev = cmd->device;
	struct request_queue *q = sdev->request_queue;
	struct request *req;
	struct scsi_request *rq;
	struct scsi_cmnd *scmd;
	struct bio *bio = pre_req->bio;
	int ret = 0;

	pre_req->hpb = hpb;
	pre_req->wb.lpn = ufshpb_get_lpn(cmd->request);
	pre_req->wb.len = ufshpb_get_len(cmd->request);
	ret = ufshpb_pre_req_add_bio_page(q, pre_req);
	if (ret)
		return ret;

	req = pre_req->req;

	/*
	 * blk_init_rl() -> alloc_request_size().
	 * q->init_rq_fn = scsi_old_init_rq behavior.
	 */
	scmd = (struct scsi_cmnd *)(req + 1);
	memset(scmd, 0, sizeof(*scmd));
	scmd->sense_buffer = pre_req->sense;
	scmd->req.sense = scmd->sense_buffer;

	/* blk_get_request behavior */
	blk_rq_init(q, req);
	q->initialize_rq_fn(req);

	/* 1. request setup */
	blk_rq_append_bio(req, &bio);
	req->cmd_flags = REQ_OP_WRITE | REQ_SYNC | REQ_OP_SCSI_OUT;
	req->rq_flags = RQF_QUIET | RQF_PREEMPT;
	req->timeout = msecs_to_jiffies(30000);
	req->end_io_data = (void *)pre_req;
	req->end_io = ufshpb_pre_req_compl_fn;

	/* 2. scsi_request setup */
	rq = scsi_req(req);
	ufshpb_set_write_buf_cmd(rq->cmd, pre_req->wb.lpn, pre_req->wb.len,
				 hpb_ctx_id);
	rq->cmd_len = scsi_command_size(rq->cmd);

	ret = ufshpb_mimic_scsi_request_fn(hpb, req);

	return ret;
}

static inline bool ufshpb_is_support_chunk(int transfer_len)
{
	return transfer_len <= HPB_MULTI_CHUNK_HIGH;
}

static int ufshpb_check_pre_req_cond(struct ufshpb_lu *hpb,
				     struct scsi_cmnd *cmd)
{
	struct request *rq = cmd->request;
	unsigned long flags;
	unsigned int transfer_len;
	unsigned int lpn;

	if (!ufshpb_is_read_cmd(cmd))
		return -EINVAL;

	if (ufshpb_is_unaligned(rq))
		return -EINVAL;

	transfer_len = ufshpb_get_len(rq);
	if (!transfer_len)
		return -EINVAL;

	if (!ufshpb_is_support_chunk(transfer_len))
		return -EINVAL;

	/*
	 * WRITE_BUFFER CMD support 36K (len=9) ~ 512K (len=128) default.
	 * it is possible to change range of transfer_len through sysfs.
	 */
	if (transfer_len < hpb->pre_req_min_tr_len ||
	    transfer_len > hpb->pre_req_max_tr_len)
		return -EINVAL;

	lpn = ufshpb_get_lpn(cmd->request);

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	if (ufshpb_ppn_dirty_check(hpb, lpn, transfer_len)) {
		spin_unlock_irqrestore(&hpb->hpb_lock, flags);
		return -EINVAL;
	}
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);

	return 0;
}

void ufshpb_end_pre_req(struct ufsf_feature *ufsf, struct request *req)
{
	struct scsi_cmnd *scmd = (struct scsi_cmnd *)(req + 1);

	set_host_byte(scmd, DID_OK);

	scmd->scsi_done(scmd);
}

int ufshpb_prepare_pre_req(struct ufsf_feature *ufsf, struct scsi_cmnd *cmd,
			   int lun)
{
	struct ufs_hba *hba = ufsf->hba;
	struct ufshpb_lu *hpb;
	struct ufshpb_req *pre_req;
	struct ufshcd_lrb *add_lrbp;
	struct ufshcd_lrb *orig_lrbp = &hba->lrb[cmd->request->tag];
	struct scsi_cmnd *pre_cmd;
	unsigned long flags;
	int add_tag, hpb_ctx_id;
	int ret = 0;

	/* WKLU could not be HPB-LU */
	if (!ufsf_is_valid_lun(lun))
		return -ENODEV;

	hpb = ufsf->hpb_lup[lun];
	ret = ufshpb_lu_get(hpb);
	if (unlikely(ret))
		return ret;

	if (hpb->force_disable) {
		ret = -ENODEV;
		goto put_hpb;
	}

	ret = ufshpb_check_pre_req_cond(hpb, cmd);
	if (ret)
		goto put_hpb;

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	pre_req = ufshpb_get_pre_req(hpb);
	if (!pre_req) {
		spin_unlock_irqrestore(&hpb->hpb_lock, flags);
		ret = -ENOMEM;
		goto put_hpb;
	}

	hpb_ctx_id = ufshpb_issue_ctx_id_ticket(hpb);
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);

	ret = ufshpb_set_pre_req(hpb, cmd, pre_req, hpb_ctx_id);
	if (ret)
		goto put_pre_req;

	add_tag = pre_req->req->tag;
	if (test_and_set_bit_lock(add_tag, &hba->lrb_in_use)) {
		ufshpb_end_pre_req(ufsf, pre_req->req);
		return -EIO;
	}

	add_lrbp = &hba->lrb[add_tag];
	WARN_ON(add_lrbp->cmd);

	pre_cmd = pre_req->req->special;
	add_lrbp->cmd = pre_cmd;
	add_lrbp->sense_bufflen = UFSHCD_REQ_SENSE_SIZE;
	add_lrbp->sense_buffer = pre_cmd->sense_buffer;
	add_lrbp->task_tag = add_tag;
	add_lrbp->lun = lun;
	add_lrbp->intr_cmd = !ufshcd_is_intr_aggr_allowed(hba) ? true : false;
	add_lrbp->req_abort_skip = false;

	orig_lrbp->hpb_ctx_id = hpb_ctx_id;

	return add_tag;
put_pre_req:
	spin_lock_irqsave(&hpb->hpb_lock, flags);
	ufshpb_put_pre_req(hpb, pre_req);
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
put_hpb:
	ufshpb_lu_put(hpb);
	return ret;
}

int ufshpb_prepare_add_lrbp(struct ufsf_feature *ufsf, int add_tag)
{
	struct ufs_hba *hba = ufsf->hba;
	struct ufshcd_lrb *add_lrbp;
	struct scsi_cmnd *pre_cmd;
	int err = 0;

	add_lrbp = &hba->lrb[add_tag];

	pre_cmd = add_lrbp->cmd;

	err = ufshcd_hold(hba, true);
	if (err)
		goto hold_err;

	err = ufshcd_hibern8_hold(hba, true);
	if (err) {
		ufshcd_release(hba, true);
		goto hold_err;
	}

	/* Vote PM QoS for the pre_req */
	ufshcd_vops_pm_qos_req_start(hba, pre_cmd->request);

	err = ufshcd_comp_scsi_upiu(hba, add_lrbp);
	if (err)
		goto map_err;

	err = ufshcd_map_sg(hba, add_lrbp);
	if (err)
		goto map_err;

	return 0;

	scsi_dma_unmap(pre_cmd);
map_err:
	ufshcd_vops_pm_qos_req_end(hba, pre_cmd->request, true);
	ufshcd_release_all(hba);
hold_err:
	add_lrbp->cmd = NULL;
	clear_bit_unlock(add_tag, &hba->lrb_in_use);
	ufsf_hpb_end_pre_req(&hba->ufsf, pre_cmd->request);
	return -EIO;
}

/* routine : READ10 -> HPB_READ  */
void ufshpb_prep_fn(struct ufsf_feature *ufsf, struct ufshcd_lrb *lrbp)
{
	struct ufshpb_lu *hpb;
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	struct request *rq;
	u64 ppn = 0;
	unsigned long lpn, flags;
	int transfer_len = TRANSFER_LEN;
	int rgn_idx, srgn_idx, srgn_offset, ret, error = 0;
	bool span_flag = false;

	/* WKLU could not be HPB-LU */
	if (!lrbp || !ufsf_is_valid_lun(lrbp->lun))
		return;

	if (!ufshpb_is_write_discard_lrbp(lrbp) &&
	    !ufshpb_is_read_cmd(lrbp->cmd))
		return;

	rq = lrbp->cmd->request;
	hpb = ufsf->hpb_lup[lrbp->lun];
	ret = ufshpb_lu_get(hpb);
	if (unlikely(ret))
		return;

	if (hpb->force_disable) {
		if (ufshpb_is_read_cmd(lrbp->cmd))
			TMSG(ufsf, hpb->lun, "%llu + %u READ_10",
			     (unsigned long long) blk_rq_pos(rq),
			     (unsigned int) blk_rq_sectors(rq));
		goto put_hpb;
	}

	lpn = ufshpb_get_lpn(rq);
	if (abs(lpn-hpb->lpn_last) > 1024) {
		ufsf_para.span++;
		span_flag = true;
	}
	hpb->lpn_last = lpn;
	ufshpb_get_pos_from_lpn(hpb, lpn, &rgn_idx, &srgn_idx, &srgn_offset);
	rgn = hpb->rgn_tbl + rgn_idx;
	srgn = rgn->srgn_tbl + srgn_idx;

	/*
	 * If cmd type is WRITE, bitmap set to dirty.
	 */
	if (ufshpb_is_write_discard_lrbp(lrbp)) {
		spin_lock_irqsave(&hpb->hpb_lock, flags);
		if (rgn->rgn_state == HPB_RGN_INACTIVE) {
			spin_unlock_irqrestore(&hpb->hpb_lock, flags);
			goto put_hpb;
		}
		ufshpb_set_dirty(hpb, lrbp, rgn_idx, srgn_idx, srgn_offset);
		spin_unlock_irqrestore(&hpb->hpb_lock, flags);
		goto put_hpb;
	}

	if (!ufshpb_is_read_cmd(lrbp->cmd))
		goto put_hpb;

	if (unlikely(ufshpb_is_unaligned(rq))) {
		TMSG_CMD(hpb, "READ_10 not aligned 4KB", rq, rgn_idx, srgn_idx);
		goto put_hpb;
	}

	transfer_len = ufshpb_get_len(rq);
	if (unlikely(!transfer_len))
		goto put_hpb;

	if (!ufshpb_is_support_chunk(transfer_len)) {
		TMSG_CMD(hpb, "READ_10 doesn't support chunk size",
			 rq, rgn_idx, srgn_idx);
		goto put_hpb;
	}

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	if (ufshpb_ppn_dirty_check(hpb, lpn, transfer_len)) {
		atomic64_inc(&hpb->miss);
		ufsf_para.miss++;
		TMSG_CMD(hpb, "READ_10 E_D", rq, rgn_idx, srgn_idx);
		spin_unlock_irqrestore(&hpb->hpb_lock, flags);
		goto put_hpb;
	}

	ppn = ufshpb_get_ppn(srgn->mctx, srgn_offset, &error);
#if defined(CONFIG_HPB_ERR_INJECTION)
	if (hpb->err_injection_select != HPB_ERR_INJECTION_DISABLE)
		ppn = ufshpb_get_err_injection_ppn(hpb, ppn);
#endif
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	if (unlikely(error)) {
		ERR_MSG("get_ppn failed.. err %d region %d subregion %d",
			error, rgn_idx, srgn_idx);
		ufshpb_lu_put(hpb);
		goto wakeup_ee_worker;
	}

	ufshpb_set_read16_cmd(hpb, lrbp, ppn, transfer_len);
	TMSG(ufsf, hpb->lun, "%llu + %u HPB_READ %d - %d context_id %d",
	     (unsigned long long) blk_rq_pos(lrbp->cmd->request),
	     (unsigned int) blk_rq_sectors(lrbp->cmd->request), rgn_idx,
	     srgn_idx, lrbp->hpb_ctx_id);

	atomic64_inc(&hpb->hit);
	ufsf_para.hit++;
	if(span_flag)
		ufsf_para.span_hit++;
	if (transfer_len == HPB_4_CHUNK_LEN)
		ufsf_para.hit_4k++;
	else if (transfer_len <= HPB_32_CHUNK_LEN)
		ufsf_para.hit_8_32k++;

put_hpb:
	ufshpb_lu_put(hpb);
	return;
wakeup_ee_worker:
	ufshpb_failed(hpb, __func__);
}

static inline void ufshpb_put_map_req(struct ufshpb_lu *hpb,
				      struct ufshpb_req *map_req)
{
	list_add_tail(&map_req->list_req, &hpb->lh_map_req_free);
	hpb->num_inflight_map_req--;
}

static struct ufshpb_req *ufshpb_get_map_req(struct ufshpb_lu *hpb)
{
	struct ufshpb_req *map_req;

	if (hpb->num_inflight_map_req >= hpb->throttle_map_req) {
		HPB_DEBUG(hpb, "map_req throttle. inflight %d throttle %d",
			  hpb->num_inflight_map_req, hpb->throttle_map_req);
		return NULL;
	}

	map_req = list_first_entry_or_null(&hpb->lh_map_req_free,
					   struct ufshpb_req, list_req);
	if (!map_req) {
		HPB_DEBUG(hpb, "There is no map_req");
		return NULL;
	}

	list_del_init(&map_req->list_req);
	hpb->num_inflight_map_req++;

	return map_req;
}

static int ufshpb_clean_dirty_bitmap(struct ufshpb_lu *hpb,
				     struct ufshpb_subregion *srgn)
{
	struct ufshpb_region *rgn;

	BUG_ON(!srgn->mctx);

	rgn = hpb->rgn_tbl + srgn->rgn_idx;

	if (rgn->rgn_state == HPB_RGN_INACTIVE) {
		HPB_DEBUG(hpb, "%d - %d evicted", srgn->rgn_idx,
			  srgn->srgn_idx);
		return -EINVAL;
	}

	memset(srgn->mctx->ppn_dirty, 0x00,
	       hpb->entries_per_srgn >> bits_per_byte_shift);

	return 0;
}

static void ufshpb_clean_active_subregion(struct ufshpb_lu *hpb,
					  struct ufshpb_subregion *srgn)
{
	struct ufshpb_region *rgn;

	BUG_ON(!srgn->mctx);

	rgn = hpb->rgn_tbl + srgn->rgn_idx;

	if (rgn->rgn_state == HPB_RGN_INACTIVE) {
		HPB_DEBUG(hpb, "%d - %d evicted", srgn->rgn_idx,
			  srgn->srgn_idx);
		return;
	}
	srgn->srgn_state = HPB_SRGN_CLEAN;
}

static void ufshpb_error_active_subregion(struct ufshpb_lu *hpb,
					  struct ufshpb_subregion *srgn)
{
	struct ufshpb_region *rgn;

	BUG_ON(!srgn->mctx);

	rgn = hpb->rgn_tbl + srgn->rgn_idx;

	if (rgn->rgn_state == HPB_RGN_INACTIVE) {
		ERR_MSG("%d - %d evicted", srgn->rgn_idx, srgn->srgn_idx);
		return;
	}
	srgn->srgn_state = HPB_SRGN_DIRTY;
}

static void ufshpb_check_ppn(struct ufshpb_lu *hpb, int rgn_idx, int srgn_idx,
			     struct ufshpb_map_ctx *mctx, const char *str)
{
	int error = 0;
	u64 val[2];

	BUG_ON(!mctx);

	val[0] = ufshpb_get_ppn(mctx, 0, &error);
	if (!error)
		val[1] = ufshpb_get_ppn(mctx, hpb->entries_per_srgn - 1,
					&error);
	if (error)
		val[0] = val[1] = 0;

	HPB_DEBUG(hpb, "%s READ BUFFER %d - %d ( %llx ~ %llx )", str, rgn_idx,
		  srgn_idx, val[0], val[1]);
}

static void ufshpb_map_compl_process(struct ufshpb_req *map_req)
{
	struct ufshpb_lu *hpb = map_req->hpb;
	struct ufshpb_subregion *srgn;
	unsigned long flags;

	srgn = hpb->rgn_tbl[map_req->rb.rgn_idx].srgn_tbl +
		map_req->rb.srgn_idx;

	if (hpb->debug)
		ufshpb_check_ppn(hpb, srgn->rgn_idx, srgn->srgn_idx, srgn->mctx,
				 "COMPL");

	TMSG(hpb->ufsf, hpb->lun, "Noti: C RB %d - %d", map_req->rb.rgn_idx,
	     map_req->rb.srgn_idx);

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	ufshpb_clean_active_subregion(hpb, srgn);
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
}

static void ufshpb_update_active_info(struct ufshpb_lu *hpb, int rgn_idx,
				      int srgn_idx)
{
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;

	rgn = hpb->rgn_tbl + rgn_idx;
	srgn = rgn->srgn_tbl + srgn_idx;

	list_del_init(&rgn->list_inact_rgn);

	if (list_empty(&srgn->list_act_srgn))
		list_add_tail(&srgn->list_act_srgn, &hpb->lh_act_srgn);
}

static void ufshpb_update_inactive_info(struct ufshpb_lu *hpb, int rgn_idx)
{
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	int srgn_idx;

	rgn = hpb->rgn_tbl + rgn_idx;

	for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
		srgn = rgn->srgn_tbl + srgn_idx;

		list_del_init(&srgn->list_act_srgn);
	}

	if (list_empty(&rgn->list_inact_rgn))
		list_add_tail(&rgn->list_inact_rgn, &hpb->lh_inact_rgn);
}

static int ufshpb_map_req_error(struct ufshpb_req *map_req)
{
	struct ufshpb_lu *hpb = map_req->hpb;
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	struct scsi_sense_hdr sshdr;
	unsigned long flags;

	rgn = hpb->rgn_tbl + map_req->rb.rgn_idx;
	srgn = rgn->srgn_tbl + map_req->rb.srgn_idx;

	scsi_normalize_sense(map_req->sense, SCSI_SENSE_BUFFERSIZE, &sshdr);

	ERR_MSG("code %x sense_key %x asc %x ascq %x", sshdr.response_code,
		sshdr.sense_key, sshdr.asc, sshdr.ascq);
	ERR_MSG("byte4 %x byte5 %x byte6 %x additional_len %x", sshdr.byte4,
		sshdr.byte5, sshdr.byte6, sshdr.additional_length);

	if (sshdr.sense_key != ILLEGAL_REQUEST)
		return 0;

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	if (rgn->rgn_state == HPB_RGN_PINNED) {
		if (sshdr.asc == 0x06 && sshdr.ascq == 0x01) {
			HPB_DEBUG(hpb, "retry pinned rb %d - %d",
				  map_req->rb.rgn_idx, map_req->rb.srgn_idx);

			spin_unlock_irqrestore(&hpb->hpb_lock, flags);
			spin_lock(&hpb->retry_list_lock);
			list_add_tail(&map_req->list_req,
				      &hpb->lh_map_req_retry);
			spin_unlock(&hpb->retry_list_lock);

			schedule_delayed_work(&hpb->retry_work,
					      msecs_to_jiffies(RETRY_DELAY_MS));
			return -EAGAIN;
		}
		HPB_DEBUG(hpb, "pinned rb %d - %d(dirty)",
			  map_req->rb.rgn_idx, map_req->rb.srgn_idx);

		ufshpb_error_active_subregion(hpb, srgn);
		spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	} else {
		ufshpb_error_active_subregion(hpb, srgn);

		spin_unlock_irqrestore(&hpb->hpb_lock, flags);

		spin_lock_irqsave(&hpb->rsp_list_lock, flags);
		ufshpb_update_inactive_info(hpb, map_req->rb.rgn_idx);
		spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);

		HPB_DEBUG(hpb, "Non-pinned rb %d will be inactive",
			  map_req->rb.rgn_idx);

		schedule_work(&hpb->task_work);
	}

	return 0;
}

#ifdef CONFIG_PM
static inline void ufshpb_mimic_blk_pm_put_request(struct request *rq)
{
	if (rq->q->dev && !(rq->rq_flags & RQF_PM) && !--rq->q->nr_pending)
		pm_runtime_mark_last_busy(rq->q->dev);
}
#endif

/* routine : map_req compl */
static void ufshpb_map_req_compl_fn(struct request *req, blk_status_t error)
{
	struct ufshpb_req *map_req = (struct ufshpb_req *) req->end_io_data;
	struct ufshpb_lu *hpb = map_req->hpb;
	unsigned long flags;
	int ret;

#ifdef CONFIG_PM
	ufshpb_mimic_blk_pm_put_request(req);
#endif
	if (ufshpb_get_state(hpb->ufsf) != HPB_PRESENT)
		goto free_map_req;

	if (error) {
		ERR_MSG("COMP_NOTI: RB number %d ( %d - %d )", error,
			map_req->rb.rgn_idx, map_req->rb.srgn_idx);

		ret = ufshpb_map_req_error(map_req);
		if (ret)
			goto retry_map_req;
	} else
		ufshpb_map_compl_process(map_req);

free_map_req:
	spin_lock_irqsave(&hpb->hpb_lock, flags);
	ufshpb_put_map_req(map_req->hpb, map_req);
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
retry_map_req:
	scsi_device_put(hpb->ufsf->sdev_ufs_lu[hpb->lun]);
	ufshpb_lu_put(hpb);
}

static inline void ufshpb_set_read_buf_cmd(unsigned char *cdb, int rgn_idx,
					   int srgn_idx, int srgn_mem_size)
{
	cdb[0] = UFSHPB_READ_BUFFER;
	cdb[1] = UFSHPB_READ_BUFFER_ID;
	cdb[2] = GET_BYTE_1(rgn_idx);
	cdb[3] = GET_BYTE_0(rgn_idx);
	cdb[4] = GET_BYTE_1(srgn_idx);
	cdb[5] = GET_BYTE_0(srgn_idx);
	cdb[6] = GET_BYTE_2(srgn_mem_size);
	cdb[7] = GET_BYTE_1(srgn_mem_size);
	cdb[8] = GET_BYTE_0(srgn_mem_size);
	cdb[9] = 0x00;
}

static int ufshpb_map_req_add_bio_page(struct ufshpb_lu *hpb,
				       struct request_queue *q, struct bio *bio,
				       struct ufshpb_map_ctx *mctx)
{
	struct page *page = NULL;
	int i, ret = 0;

	bio_reset(bio);

	for (i = 0; i < hpb->mpages_per_srgn; i++) {
		/* virt_to_page(p + (OS_PAGE_SIZE * i)); */
		page = mctx->m_page[i];
		if (!page)
			return -ENOMEM;

		ret = bio_add_pc_page(q, bio, page, hpb->mpage_bytes, 0);

		if (ret != hpb->mpage_bytes) {
			ERR_MSG("bio_add_pc_page fail: %d", ret);
			return -ENOMEM;
		}
	}

	return 0;
}

static int ufshpb_execute_map_req(struct ufshpb_lu *hpb,
				  struct ufshpb_req *map_req)
{
	struct scsi_device *sdev;
	struct request_queue *q;
	struct request *req;
	struct scsi_cmnd *scmd;
	struct bio *bio = map_req->bio;
	struct scsi_request *rq;
	int ret = 0;

	sdev = hpb->ufsf->sdev_ufs_lu[hpb->lun];
	if (!sdev) {
		ERR_MSG("cannot find scsi_device");
		return -ENODEV;
	}

	ret = ufsf_get_scsi_device(hpb->ufsf->hba, sdev);
	if (ret)
		return ret;

	q = sdev->request_queue;

	ret = ufshpb_map_req_add_bio_page(hpb, q, bio, map_req->rb.mctx);
	if (ret) {
		scsi_device_put(sdev);
		return ret;
	}

	req = map_req->req;

	/*
	 * blk_init_rl() -> alloc_request_size().
	 * q->init_rq_fn = scsi_old_init_rq behavior.
	 */
	scmd = (struct scsi_cmnd *)(req + 1);
	memset(scmd, 0, sizeof(*scmd));
	scmd->sense_buffer = map_req->sense;
	scmd->req.sense = scmd->sense_buffer;

	/* blk_get_request behavior */
	blk_rq_init(q, req);
	q->initialize_rq_fn(req);

	/* 1. request setup */
	blk_rq_append_bio(req, &bio); /* req->__data_len is setted */
	req->cmd_flags = REQ_OP_READ | REQ_OP_SCSI_IN;
	req->rq_flags = RQF_QUIET | RQF_PREEMPT;
	req->timeout = msecs_to_jiffies(30000);
	req->end_io_data = (void *)map_req;

	/* 2. scsi_request setup */
	rq = scsi_req(req);
	ufshpb_set_read_buf_cmd(rq->cmd, map_req->rb.rgn_idx,
				map_req->rb.srgn_idx, hpb->srgn_mem_size);
	rq->cmd_len = scsi_command_size(rq->cmd);

	if (hpb->debug)
		ufshpb_check_ppn(hpb, map_req->rb.rgn_idx, map_req->rb.srgn_idx,
				 map_req->rb.mctx, "ISSUE");

	TMSG(hpb->ufsf, hpb->lun, "Noti: I RB %d - %d", map_req->rb.rgn_idx,
	     map_req->rb.srgn_idx);

	blk_execute_rq_nowait(q, NULL, req, 1, ufshpb_map_req_compl_fn);

	atomic64_inc(&hpb->map_req_cnt);
	ufsf_para.map_req++;

	return 0;
}

static inline void ufshpb_set_map_req(struct ufshpb_lu *hpb, int rgn_idx,
				      int srgn_idx, struct ufshpb_map_ctx *mctx,
				      struct ufshpb_req *map_req)
{
	map_req->hpb = hpb;
	map_req->rb.rgn_idx = rgn_idx;
	map_req->rb.srgn_idx = srgn_idx;
	map_req->rb.mctx = mctx;
	map_req->rb.lun = hpb->lun;
}

static struct ufshpb_map_ctx *ufshpb_get_map_ctx(struct ufshpb_lu *hpb,
						 int *err)
{
	struct ufshpb_map_ctx *mctx;

	mctx = list_first_entry_or_null(&hpb->lh_map_ctx_free,
					struct ufshpb_map_ctx, list_table);
	if (mctx) {
		list_del_init(&mctx->list_table);
		hpb->debug_free_table--;
		return mctx;
	}
	*err = -ENOMEM;
	return NULL;
}

static inline void ufshpb_add_lru_info(struct victim_select_info *lru_info,
				       struct ufshpb_region *rgn)
{
	rgn->rgn_state = HPB_RGN_ACTIVE;
	list_add_tail(&rgn->list_lru_rgn, &lru_info->lh_lru_rgn);
	atomic64_inc(&lru_info->active_cnt);
	ufsf_para.rgn_act++;
}

static inline int ufshpb_add_region(struct ufshpb_lu *hpb,
				    struct ufshpb_region *rgn)
{
	struct victim_select_info *lru_info;
	int srgn_idx;
	int err = 0;

	lru_info = &hpb->lru_info;

	for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
		struct ufshpb_subregion *srgn;

		srgn = rgn->srgn_tbl + srgn_idx;

		srgn->mctx = ufshpb_get_map_ctx(hpb, &err);
		if (!srgn->mctx) {
			HPB_DEBUG(hpb, "get mctx err %d srgn %d free_table %d",
				  err, srgn_idx, hpb->debug_free_table);
			goto out;
		}

		srgn->srgn_state = HPB_SRGN_DIRTY;
	}
	HPB_DEBUG(hpb, "\x1b[44m\x1b[32m E->active region: %d \x1b[0m",
		  rgn->rgn_idx);
	TMSG(hpb->ufsf, hpb->lun, "Noti: ACT RG: %d", rgn->rgn_idx);

	ufshpb_add_lru_info(lru_info, rgn);
out:
	return err;
}

static inline void ufshpb_put_map_ctx(struct ufshpb_lu *hpb,
				      struct ufshpb_map_ctx *mctx)
{
	list_add(&mctx->list_table, &hpb->lh_map_ctx_free);
	hpb->debug_free_table++;
}

static inline void ufshpb_purge_active_subregion(struct ufshpb_lu *hpb,
						 struct ufshpb_subregion *srgn,
						 int state)
{
	if (state == HPB_SRGN_UNUSED) {
		ufshpb_put_map_ctx(hpb, srgn->mctx);
		srgn->mctx = NULL;
	}

	srgn->srgn_state = state;
}

static inline void ufshpb_cleanup_lru_info(struct victim_select_info *lru_info,
					   struct ufshpb_region *rgn)
{
	list_del_init(&rgn->list_lru_rgn);
	rgn->rgn_state = HPB_RGN_INACTIVE;
	atomic64_dec(&lru_info->active_cnt);
	ufsf_para.rgn_act--;
}

static void __ufshpb_evict_region(struct ufshpb_lu *hpb,
				  struct ufshpb_region *rgn)
{
	struct victim_select_info *lru_info;
	struct ufshpb_subregion *srgn;
	int srgn_idx;

	lru_info = &hpb->lru_info;

	HPB_DEBUG(hpb, "\x1b[41m\x1b[33m C->EVICT region: %d \x1b[0m",
		  rgn->rgn_idx);
	TMSG(hpb->ufsf, hpb->lun, "Noti: EVIC RG: %d", rgn->rgn_idx);

	ufshpb_cleanup_lru_info(lru_info, rgn);

	for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
		srgn = rgn->srgn_tbl + srgn_idx;

		ufshpb_purge_active_subregion(hpb, srgn, HPB_SRGN_UNUSED);
	}
}

static void ufshpb_hit_lru_info(struct victim_select_info *lru_info,
				struct ufshpb_region *rgn)
{
	switch (lru_info->selection_type) {
	case LRU:
		list_move_tail(&rgn->list_lru_rgn, &lru_info->lh_lru_rgn);
		break;
	default:
		break;
	}
}

/*
 *  Must be held hpb_lock before call this func.
 */
static int ufshpb_check_issue_state_srgns(struct ufshpb_lu *hpb,
					  struct ufshpb_region *rgn)
{
	struct ufshpb_subregion *srgn;
	int srgn_idx;

	for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
		srgn  = rgn->srgn_tbl + srgn_idx;

		if (srgn->srgn_state == HPB_SRGN_ISSUED)
			return -EPERM;
	}
	return 0;
}

static struct ufshpb_region *ufshpb_victim_lru_info(struct ufshpb_lu *hpb)
{
	struct victim_select_info *lru_info = &hpb->lru_info;
	struct ufshpb_region *rgn;
	struct ufshpb_region *victim_rgn = NULL;

	switch (lru_info->selection_type) {
	case LRU:
		list_for_each_entry(rgn, &lru_info->lh_lru_rgn, list_lru_rgn) {
			if (!rgn)
				break;

			if (ufshpb_check_issue_state_srgns(hpb, rgn))
				continue;

			victim_rgn = rgn;
			break;
		}
		break;
	default:
		break;
	}

	return victim_rgn;
}

static int ufshpb_evict_region(struct ufshpb_lu *hpb, struct ufshpb_region *rgn)
{
	unsigned long flags;

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	if (rgn->rgn_state == HPB_RGN_PINNED) {
		/*
		 * Pinned active-block should not drop-out.
		 * But if so, it would treat error as critical,
		 * and it will run hpb_eh_work
		 */
		ERR_MSG("pinned active-block drop-out error");
		goto out;
	}

	if (!list_empty(&rgn->list_lru_rgn)) {
		if (ufshpb_check_issue_state_srgns(hpb, rgn))
			goto evict_fail;

		__ufshpb_evict_region(hpb, rgn);
	}
out:
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	return 0;
evict_fail:
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	return -EPERM;
}

static inline struct
ufshpb_rsp_field *ufshpb_get_hpb_rsp(struct ufshcd_lrb *lrbp)
{
	return (struct ufshpb_rsp_field *)&lrbp->ucd_rsp_ptr->sr.sense_data_len;
}

static int ufshpb_issue_map_req(struct ufshpb_lu *hpb,
				struct ufshpb_subregion *srgn)
{
	struct ufshpb_req *map_req;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&hpb->hpb_lock, flags);

	if (srgn->srgn_state == HPB_SRGN_ISSUED) {
		ret = -EAGAIN;
		goto unlock_out;
	}

	map_req = ufshpb_get_map_req(hpb);
	if (!map_req) {
		ret = -ENOMEM;
		goto unlock_out;
	}

	srgn->srgn_state = HPB_SRGN_ISSUED;

	ret = ufshpb_clean_dirty_bitmap(hpb, srgn);
	if (ret) {
		ufshpb_put_map_req(hpb, map_req);
		goto unlock_out;
	}
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);

	ufshpb_set_map_req(hpb, srgn->rgn_idx, srgn->srgn_idx,
			   srgn->mctx, map_req);

	ret = ufshpb_lu_get(hpb);
	if (unlikely(ret)) {
		ERR_MSG("ufshpb_lu_get failed. (%d)", ret);
		spin_lock_irqsave(&hpb->hpb_lock, flags);
		ufshpb_put_map_req(hpb, map_req);
		goto unlock_out;
	}

	ret = ufshpb_execute_map_req(hpb, map_req);
	if (ret) {
		ERR_MSG("issue map_req failed. [%d-%d] err %d",
			srgn->rgn_idx, srgn->srgn_idx, ret);
		ufshpb_lu_put(hpb);
		goto wakeup_ee_worker;
	}
	return ret;
unlock_out:
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	return ret;
wakeup_ee_worker:
	ufshpb_failed(hpb, __func__);
	return ret;
}

static int ufshpb_load_region(struct ufshpb_lu *hpb, struct ufshpb_region *rgn)
{
	struct ufshpb_region *victim_rgn;
	struct victim_select_info *lru_info = &hpb->lru_info;
	unsigned long flags;
	int ret = 0;

	/*
	 * if already region is added to lru_list,
	 * just initiate the information of lru.
	 * because the region already has the map ctx.
	 * (!list_empty(&rgn->list_region) == region->state=active...)
	 */
	spin_lock_irqsave(&hpb->hpb_lock, flags);
	if (!list_empty(&rgn->list_lru_rgn)) {
		ufshpb_hit_lru_info(lru_info, rgn);
		goto out;
	}

	if (rgn->rgn_state == HPB_RGN_INACTIVE) {
		if (atomic64_read(&lru_info->active_cnt)
		    == lru_info->max_lru_active_cnt) {
			victim_rgn = ufshpb_victim_lru_info(hpb);
			if (!victim_rgn) {
				HPB_DEBUG(hpb, "UFSHPB victim_rgn is NULL");
				ret = -ENOMEM;
				goto out;
			}
			TMSG(hpb->ufsf, hpb->lun, "Noti: VT RG %d",
			     victim_rgn->rgn_idx);
			HPB_DEBUG(hpb, "LRU MAX(=%lld). victim choose %d",
				  (long long)atomic64_read(&lru_info->active_cnt),
				  victim_rgn->rgn_idx);

			__ufshpb_evict_region(hpb, victim_rgn);
		}

		ret = ufshpb_add_region(hpb, rgn);
		if (ret) {
			ERR_MSG("UFSHPB memory allocation failed");
			spin_unlock_irqrestore(&hpb->hpb_lock, flags);
			goto wakeup_ee_worker;
		}
	}
out:
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	return ret;
wakeup_ee_worker:
	ufshpb_failed(hpb, __func__);
	return ret;
}

static void ufshpb_rsp_req_region_update(struct ufshpb_lu *hpb,
					 struct ufshpb_rsp_field *rsp_field)
{
	int num, rgn_idx, srgn_idx;

	/*
	 * If active rgn = inactive rgn, choose inactive rgn.
	 * So, active process -> inactive process
	 */
	spin_lock(&hpb->rsp_list_lock);
	for (num = 0; num < rsp_field->active_rgn_cnt; num++) {
		rgn_idx = be16_to_cpu(rsp_field->hpb_active_field[num].active_rgn);
		srgn_idx = be16_to_cpu(rsp_field->hpb_active_field[num].active_srgn);

		HPB_DEBUG(hpb, "act num: %d, region: %d, subregion: %d",
			  num + 1, rgn_idx, srgn_idx);
		ufshpb_update_active_info(hpb, rgn_idx, srgn_idx);
		atomic64_inc(&hpb->rb_active_cnt);
		ufsf_para.noti_act++;
	}

	for (num = 0; num < rsp_field->inactive_rgn_cnt; num++) {
		rgn_idx = be16_to_cpu(rsp_field->hpb_inactive_field[num]);
		HPB_DEBUG(hpb, "inact num: %d, region: %d", num + 1, rgn_idx);
		ufshpb_update_inactive_info(hpb, rgn_idx);
		atomic64_inc(&hpb->rb_inactive_cnt);
		ufsf_para.noti_inact++;
	}
	spin_unlock(&hpb->rsp_list_lock);

	TMSG(hpb->ufsf, hpb->lun, "Noti: #ACT %u, #INACT %u",
	     rsp_field->active_rgn_cnt, rsp_field->inactive_rgn_cnt);

	schedule_work(&hpb->task_work);
}

static inline int ufshpb_may_field_valid(struct ufshcd_lrb *lrbp,
					 struct ufshpb_rsp_field *rsp_field)
{
	if (be16_to_cpu(rsp_field->sense_data_len) != DEV_SENSE_SEG_LEN ||
	    rsp_field->desc_type != DEV_DES_TYPE ||
	    rsp_field->additional_len != DEV_ADDITIONAL_LEN ||
	    rsp_field->hpb_type == HPB_RSP_NONE ||
	    rsp_field->active_rgn_cnt > MAX_ACTIVE_NUM ||
	    rsp_field->inactive_rgn_cnt > MAX_INACTIVE_NUM ||
	    (!rsp_field->active_rgn_cnt && !rsp_field->inactive_rgn_cnt))
		return -EINVAL;

	if (!ufsf_is_valid_lun(lrbp->lun)) {
		ERR_MSG("LU(%d) is not supported", lrbp->lun);
		return -EINVAL;
	}

	return 0;
}

static bool ufshpb_is_empty_rsp_lists(struct ufshpb_lu *hpb)
{
	bool ret = true;
	unsigned long flags;

	spin_lock_irqsave(&hpb->rsp_list_lock, flags);
	if (!list_empty(&hpb->lh_inact_rgn) || !list_empty(&hpb->lh_act_srgn))
		ret = false;
	spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);

	return ret;
}

/* routine : isr (ufs) */
void ufshpb_rsp_upiu(struct ufsf_feature *ufsf, struct ufshcd_lrb *lrbp)
{
	struct ufshpb_lu *hpb;
	struct ufshpb_rsp_field *rsp_field;
	int data_seg_len, ret;

	data_seg_len = be32_to_cpu(lrbp->ucd_rsp_ptr->header.dword_2)
		& MASK_RSP_UPIU_DATA_SEG_LEN;

	if (!data_seg_len) {
		bool do_task_work = false;

		if (!ufsf_is_valid_lun(lrbp->lun))
			return;

		hpb = ufsf->hpb_lup[lrbp->lun];
		ret = ufshpb_lu_get(hpb);
		if (unlikely(ret))
			return;

		do_task_work = !ufshpb_is_empty_rsp_lists(hpb);
		if (do_task_work)
			schedule_work(&hpb->task_work);

		goto put_hpb;
	}

	rsp_field = ufshpb_get_hpb_rsp(lrbp);

	if (ufshpb_may_field_valid(lrbp, rsp_field)) {
		WARN_ON(rsp_field->additional_len != DEV_ADDITIONAL_LEN);
		return;
	}

	hpb = ufsf->hpb_lup[lrbp->lun];
	ret = ufshpb_lu_get(hpb);
	if (unlikely(ret)) {
		ERR_MSG("ufshpb_lu_get failed. (%d)", ret);
		return;
	}

	if (hpb->force_map_req_disable)
		goto put_hpb;

	HPB_DEBUG(hpb, "**** HPB Noti %u LUN %u Seg-Len %u, #ACT %u, #INACT %u",
		  rsp_field->hpb_type, lrbp->lun,
		  be32_to_cpu(lrbp->ucd_rsp_ptr->header.dword_2) &
		  MASK_RSP_UPIU_DATA_SEG_LEN, rsp_field->active_rgn_cnt,
		  rsp_field->inactive_rgn_cnt);
	atomic64_inc(&hpb->rb_noti_cnt);
	ufsf_para.noti++;

	switch (rsp_field->hpb_type) {
	case HPB_RSP_REQ_REGION_UPDATE:
		WARN_ON(data_seg_len != DEV_DATA_SEG_LEN);
		ufshpb_rsp_req_region_update(hpb, rsp_field);
		goto put_hpb;
	default:
		HPB_DEBUG(hpb, "hpb_type is not available : %d",
			  rsp_field->hpb_type);
		goto put_hpb;
	}

put_hpb:
	ufshpb_lu_put(hpb);
}

static int ufshpb_execute_map_req_wait(struct ufshpb_lu *hpb,
				       unsigned char *cmd,
				       struct ufshpb_subregion *srgn)
{
	struct ufsf_feature *ufsf = hpb->ufsf;
	struct scsi_device *sdev;
	struct request_queue *q;
	struct request *req;
	struct scsi_request *rq;
	struct bio *bio;
	struct scsi_sense_hdr sshdr;
	unsigned long flags;
	int ret = 0;

	sdev = ufsf->sdev_ufs_lu[hpb->lun];
	if (!sdev) {
		ERR_MSG("cannot find scsi_device");
		return -ENODEV;
	}

	q = sdev->request_queue;

	ret = ufsf_get_scsi_device(ufsf->hba, sdev);
	if (ret)
		return ret;

	req = blk_get_request(q, REQ_OP_SCSI_IN, GFP_KERNEL);
	if (IS_ERR(req)) {
		ERR_MSG("cannot get request");
		ret = -EIO;
		goto sdev_put_out;
	}

	bio = bio_kmalloc(GFP_KERNEL, hpb->mpages_per_srgn);
	if (!bio) {
		ret = -ENOMEM;
		goto req_put_out;
	}

	ret = ufshpb_map_req_add_bio_page(hpb, q, bio, srgn->mctx);
	if (ret)
		goto mem_free_out;

	/* 1. request setup*/
	blk_rq_append_bio(req, &bio); /* req->__data_len */
	req->timeout = msecs_to_jiffies(30000);
	req->cmd_flags |= REQ_OP_READ;
	req->rq_flags |= RQF_QUIET | RQF_PREEMPT;

	/* 2. scsi_request setup */
	rq = scsi_req(req);
	rq->cmd_len = scsi_command_size(cmd);
	memcpy(rq->cmd, cmd, rq->cmd_len);

	blk_execute_rq(q, NULL, req, 1);
	if (rq->result) {
		ret = -EIO;
		scsi_normalize_sense(rq->sense, SCSI_SENSE_BUFFERSIZE, &sshdr);
		ERR_MSG("code %x sense_key %x asc %x ascq %x",
			sshdr.response_code, sshdr.sense_key, sshdr.asc,
			sshdr.ascq);
		ERR_MSG("byte4 %x byte5 %x byte6 %x additional_len %x",
			sshdr.byte4, sshdr.byte5, sshdr.byte6,
			sshdr.additional_length);
		spin_lock_irqsave(&hpb->hpb_lock, flags);
		ufshpb_error_active_subregion(hpb, srgn);
		spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	}
mem_free_out:
	bio_put(bio);
req_put_out:
	blk_put_request(req);
sdev_put_out:
	scsi_device_put(sdev);
	return ret;
}

static int ufshpb_issue_map_req_from_list(struct ufshpb_lu *hpb)
{
	struct ufshpb_subregion *srgn;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&hpb->rsp_list_lock, flags);

	while ((srgn = list_first_entry_or_null(&hpb->lh_pinned_srgn,
						struct ufshpb_subregion,
						list_act_srgn))) {
		unsigned char cmd[10] = { 0 };

		list_del_init(&srgn->list_act_srgn);
		spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);

		ufshpb_set_read_buf_cmd(cmd, srgn->rgn_idx, srgn->srgn_idx,
					hpb->srgn_mem_size);

		if (hpb->debug)
			ufshpb_check_ppn(hpb, srgn->rgn_idx, srgn->srgn_idx,
					 srgn->mctx, "ISSUE");

		TMSG(hpb->ufsf, hpb->lun, "Noti: I RB %d - %d",
		     srgn->rgn_idx, srgn->srgn_idx);

		ret = ufshpb_execute_map_req_wait(hpb, cmd, srgn);
		if (ret < 0) {
			ERR_MSG("region %d sub %d failed with err %d",
				srgn->rgn_idx, srgn->srgn_idx, ret);
			spin_lock_irqsave(&hpb->rsp_list_lock, flags);
			if (list_empty(&srgn->list_act_srgn))
				list_add(&srgn->list_act_srgn,
					 &hpb->lh_pinned_srgn);
			continue;
		}

		TMSG(hpb->ufsf, hpb->lun, "Noti: C RB %d - %d",
		     srgn->rgn_idx, srgn->srgn_idx);

		if (hpb->debug)
			ufshpb_check_ppn(hpb, srgn->rgn_idx, srgn->srgn_idx,
					 srgn->mctx, "COMPL");

		spin_lock_irqsave(&hpb->hpb_lock, flags);
		ufshpb_clean_active_subregion(hpb, srgn);
		spin_unlock_irqrestore(&hpb->hpb_lock, flags);

		spin_lock_irqsave(&hpb->rsp_list_lock, flags);
	}

	spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);

	return 0;
}

static void ufshpb_pinned_work_handler(struct work_struct *work)
{
	struct ufshpb_lu *hpb;
	int ret;

	hpb = container_of(work, struct ufshpb_lu, pinned_work);
	HPB_DEBUG(hpb, "worker start for pinned region");

	pm_runtime_get_sync(hpb->ufsf->hba->dev);
	ufshpb_lu_get(hpb);

	if (!list_empty(&hpb->lh_pinned_srgn)) {
		ret = ufshpb_issue_map_req_from_list(hpb);
		/*
		 * if its function failed at init time,
		 * ufshpb-device will request map-req,
		 * so it is not critical-error, and just finish work-handler
		 */
		if (ret)
			HPB_DEBUG(hpb, "failed map-issue. ret %d", ret);
	}
	ufshpb_lu_put(hpb);
	pm_runtime_put_sync(hpb->ufsf->hba->dev);
	HPB_DEBUG(hpb, "worker end");
}

static int ufshpb_check_pm(struct ufshpb_lu *hpb)
{
	struct ufs_hba *hba = hpb->ufsf->hba;

	if (hba->pm_op_in_progress ||
	    hba->curr_dev_pwr_mode != UFS_ACTIVE_PWR_MODE) {
		INFO_MSG("hba current power state %d pm_progress %d",
			 hba->curr_dev_pwr_mode,
			 hba->pm_op_in_progress);
		return -ENODEV;
	}
	return 0;
}

static void ufshpb_retry_work_handler(struct work_struct *work)
{
	struct ufshpb_lu *hpb;
	struct delayed_work *dwork = to_delayed_work(work);
	struct ufshpb_req *map_req, *next;
	unsigned long flags;
	bool do_retry = false;
	int ret = 0;

	LIST_HEAD(retry_list);

	hpb = container_of(dwork, struct ufshpb_lu, retry_work);

	if (ufshpb_check_pm(hpb))
		return;

	ret = ufshpb_lu_get(hpb);
	if (unlikely(ret)) {
		ERR_MSG("ufshpb_lu_get failed. (%d)", ret);
		return;
	}

	HPB_DEBUG(hpb, "retry worker start");

	spin_lock_bh(&hpb->retry_list_lock);
	list_splice_init(&hpb->lh_map_req_retry, &retry_list);
	spin_unlock_bh(&hpb->retry_list_lock);

	list_for_each_entry_safe(map_req, next, &retry_list, list_req) {
		if (ufshpb_get_state(hpb->ufsf) == HPB_SUSPEND) {
			INFO_MSG("suspend state. Issue READ_BUFFER in the next turn");
			spin_lock_bh(&hpb->retry_list_lock);
			list_splice_init(&retry_list, &hpb->lh_map_req_retry);
			spin_unlock_bh(&hpb->retry_list_lock);
			do_retry = true;
			break;
		}

		list_del_init(&map_req->list_req);

		ret = ufshpb_lu_get(hpb);
		if (unlikely(ret)) {
			WARN_MSG("ufshpb_lu_get failed (%d)", ret);
			spin_lock_irqsave(&hpb->hpb_lock, flags);
			ufshpb_put_map_req(hpb, map_req);
			spin_unlock_irqrestore(&hpb->hpb_lock, flags);
			continue;
		}

		ret = ufshpb_execute_map_req(hpb, map_req);
		if (ret) {
			ERR_MSG("issue map_req failed. [%d-%d] err %d",
				map_req->rb.rgn_idx, map_req->rb.srgn_idx, ret);
			ufshpb_lu_put(hpb);
			goto wakeup_ee_worker;
		}
	}
	HPB_DEBUG(hpb, "worker end");
	ufshpb_lu_put(hpb);

	if (do_retry)
		schedule_delayed_work(&hpb->retry_work,
				      msecs_to_jiffies(RETRY_DELAY_MS));
	return;
wakeup_ee_worker:
	ufshpb_lu_put(hpb);
	ufshpb_failed(hpb, __func__);
}

static void ufshpb_add_starved_list(struct ufshpb_lu *hpb,
				    struct ufshpb_region *rgn,
				    struct list_head *starved_list)
{
	struct ufshpb_subregion *srgn;
	int srgn_idx;

	if (!list_empty(&rgn->list_inact_rgn))
		return;

	for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
		srgn = rgn->srgn_tbl + srgn_idx;

		if (!list_empty(&srgn->list_act_srgn))
			return;
	}

	list_add_tail(&rgn->list_inact_rgn, starved_list);
}

static void ufshpb_run_inactive_region_list(struct ufshpb_lu *hpb)
{
	struct ufshpb_region *rgn;
	unsigned long flags;
	int ret;
	LIST_HEAD(starved_list);

	spin_lock_irqsave(&hpb->rsp_list_lock, flags);
	while ((rgn = list_first_entry_or_null(&hpb->lh_inact_rgn,
					       struct ufshpb_region,
					       list_inact_rgn))) {
		if (ufshpb_get_state(hpb->ufsf) == HPB_SUSPEND) {
			INFO_MSG("suspend state. Inactivate rgn the next turn");
			break;
		}

		list_del_init(&rgn->list_inact_rgn);
		spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);

		ret = ufshpb_evict_region(hpb, rgn);
		if (ret) {
			spin_lock_irqsave(&hpb->rsp_list_lock, flags);
			ufshpb_add_starved_list(hpb, rgn, &starved_list);
			spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);
		}

		spin_lock_irqsave(&hpb->rsp_list_lock, flags);
	}

	list_splice(&starved_list, &hpb->lh_inact_rgn);
	spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);
}

static void ufshpb_add_active_list(struct ufshpb_lu *hpb,
				   struct ufshpb_region *rgn,
				   struct ufshpb_subregion *srgn)
{
	if (!list_empty(&rgn->list_inact_rgn))
		return;

	if (!list_empty(&srgn->list_act_srgn)) {
		list_move(&srgn->list_act_srgn, &hpb->lh_act_srgn);
		return;
	}

	list_add(&srgn->list_act_srgn, &hpb->lh_act_srgn);
}

static void ufshpb_run_active_subregion_list(struct ufshpb_lu *hpb)
{
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&hpb->rsp_list_lock, flags);
	while ((srgn = list_first_entry_or_null(&hpb->lh_act_srgn,
						struct ufshpb_subregion,
						list_act_srgn))) {
		if (ufshpb_get_state(hpb->ufsf) == HPB_SUSPEND) {
			INFO_MSG("suspend state. Issuing READ_BUFFER the next turn");
			break;
		}

		list_del_init(&srgn->list_act_srgn);

		if (hpb->force_map_req_disable) {
			HPB_DEBUG(hpb, "map_req disabled");
			continue;
		}

		spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);

		rgn = hpb->rgn_tbl + srgn->rgn_idx;

		ret = ufshpb_load_region(hpb, rgn);
		if (ret)
			break;

		ret = ufshpb_issue_map_req(hpb, srgn);
		if (ret)
			break;

		spin_lock_irqsave(&hpb->rsp_list_lock, flags);
	}

	if (ret) {
		spin_lock_irqsave(&hpb->rsp_list_lock, flags);
		ufshpb_add_active_list(hpb, rgn, srgn);
	}
	spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);
}

static void ufshpb_task_work_handler(struct work_struct *work)
{
	struct ufshpb_lu *hpb;
	int ret;

	hpb = container_of(work, struct ufshpb_lu, task_work);
	ret = ufshpb_lu_get(hpb);
	if (unlikely(ret)) {
		if (hpb)
			WARN_MSG("lu_get failed.. hpb_state %d",
				 ufshpb_get_state(hpb->ufsf));
		WARN_MSG("warning: ufshpb_lu_get failed %d..", ret);
		return;
	}

	ufshpb_run_inactive_region_list(hpb);
	ufshpb_run_active_subregion_list(hpb);

	ufshpb_lu_put(hpb);
}

static inline void ufshpb_map_req_mempool_remove(struct ufshpb_lu *hpb)
{
	int i;

	for (i = 0; i < hpb->qd; i++) {
		kfree(hpb->map_req[i].req);
		bio_put(hpb->map_req[i].bio);
	}

	kfree(hpb->map_req);
}

static inline void ufshpb_pre_req_mempool_remove(struct ufshpb_lu *hpb)
{
	int i;

	for (i = 0; i < hpb->qd; i++) {
		kfree(hpb->pre_req[i].req);
		bio_put(hpb->pre_req[i].bio);
		__free_page(hpb->pre_req[i].wb.m_page);
	}

	kfree(hpb->pre_req);
}

static void ufshpb_table_mempool_remove(struct ufshpb_lu *hpb)
{
	struct ufshpb_map_ctx *mctx, *next;
	int i;

	/*
	 * the mctx in the lh_map_ctx_free has been allocated completely.
	 */
	list_for_each_entry_safe(mctx, next, &hpb->lh_map_ctx_free,
				 list_table) {
		for (i = 0; i < hpb->mpages_per_srgn; i++)
			__free_page(mctx->m_page[i]);

		vfree(mctx->ppn_dirty);
		kfree(mctx->m_page);
		kfree(mctx);
		hpb->alloc_mctx--;
	}
}

/*
 * this function doesn't need to hold lock due to be called in init.
 * (hpb_lock, rsp_list_lock, etc..)
 */
static int ufshpb_init_pinned_active_region(struct ufshpb_lu *hpb,
					    struct ufshpb_region *rgn)
{
	struct ufshpb_subregion *srgn;
	int srgn_idx, j;
	int err = 0;

	for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
		srgn = rgn->srgn_tbl + srgn_idx;

		srgn->mctx = ufshpb_get_map_ctx(hpb, &err);
		if (err) {
			ERR_MSG("get mctx err %d srgn %d free_table %d",
				err, srgn_idx, hpb->debug_free_table);
			goto release;
		}

		srgn->srgn_state = HPB_SRGN_ISSUED;
		/*
		 * no need to clean ppn_dirty bitmap
		 * because it is vzalloc-ed @ufshpb_table_mempool_init()
		 */
		list_add_tail(&srgn->list_act_srgn, &hpb->lh_pinned_srgn);
	}

	rgn->rgn_state = HPB_RGN_PINNED;

	return 0;

release:
	for (j = 0; j < srgn_idx; j++) {
		srgn = rgn->srgn_tbl + j;
		ufshpb_put_map_ctx(hpb, srgn->mctx);
	}

	return err;
}

static inline bool ufshpb_is_pinned_region(struct ufshpb_lu *hpb, int rgn_idx)
{
	if (hpb->lu_pinned_end_offset != -1 &&
	    rgn_idx >= hpb->lu_pinned_rgn_startidx &&
	    rgn_idx <= hpb->lu_pinned_end_offset)
		return true;

	return false;
}

static inline void ufshpb_init_jobs(struct ufshpb_lu *hpb)
{
	INIT_WORK(&hpb->pinned_work, ufshpb_pinned_work_handler);
	INIT_DELAYED_WORK(&hpb->retry_work, ufshpb_retry_work_handler);
	INIT_WORK(&hpb->task_work, ufshpb_task_work_handler);
}

static inline void ufshpb_cancel_jobs(struct ufshpb_lu *hpb)
{
	cancel_work_sync(&hpb->pinned_work);
	cancel_delayed_work_sync(&hpb->retry_work);
	cancel_work_sync(&hpb->task_work);
}

static void ufshpb_init_subregion_tbl(struct ufshpb_lu *hpb,
				      struct ufshpb_region *rgn)
{
	int srgn_idx;

	for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
		struct ufshpb_subregion *srgn = rgn->srgn_tbl + srgn_idx;

		INIT_LIST_HEAD(&srgn->list_act_srgn);

		srgn->rgn_idx = rgn->rgn_idx;
		srgn->srgn_idx = srgn_idx;
		srgn->srgn_state = HPB_SRGN_UNUSED;
	}
}

static inline int ufshpb_alloc_subregion_tbl(struct ufshpb_lu *hpb,
					     struct ufshpb_region *rgn,
					     int srgn_cnt)
{
	rgn->srgn_tbl =
		kzalloc(sizeof(struct ufshpb_subregion) * srgn_cnt, GFP_KERNEL);
	if (!rgn->srgn_tbl)
		return -ENOMEM;

	rgn->srgn_cnt = srgn_cnt;
	return 0;
}

static int ufshpb_table_mempool_init(struct ufshpb_lu *hpb)
{
	struct ufshpb_map_ctx *mctx = NULL;
	int i, j, k;

	INIT_LIST_HEAD(&hpb->lh_map_ctx_free);

	hpb->alloc_mctx = hpb->lu_max_active_rgns * hpb->srgns_per_rgn;

	for (i = 0; i < hpb->alloc_mctx; i++) {
		mctx = kmalloc(sizeof(struct ufshpb_map_ctx), GFP_KERNEL);
		if (!mctx)
			goto release_mem;

		mctx->m_page =
			kzalloc(sizeof(struct page *) * hpb->mpages_per_srgn,
				GFP_KERNEL);
		if (!mctx->m_page)
			goto release_mem;

		mctx->ppn_dirty = vzalloc(hpb->entries_per_srgn >>
					  bits_per_byte_shift);
		if (!mctx->ppn_dirty)
			goto release_mem;

		for (j = 0; j < hpb->mpages_per_srgn; j++) {
			mctx->m_page[j] = alloc_page(GFP_KERNEL | __GFP_ZERO);
			if (!mctx->m_page[j]) {
				for (k = 0; k < j; k++)
					__free_page(mctx->m_page[k]);
				goto release_mem;
			}
		}

		INIT_LIST_HEAD(&mctx->list_table);
		list_add(&mctx->list_table, &hpb->lh_map_ctx_free);

		hpb->debug_free_table++;
	}

	INFO_MSG("The number of mctx = (%d). debug_free_table (%d)",
		 hpb->alloc_mctx, hpb->debug_free_table);
	return 0;
release_mem:
	/*
	 * mctxs already added in lh_map_ctx_free will be removed
	 * in the caller function.
	 */
	if (mctx) {
		kfree(mctx->m_page);
		vfree(mctx->ppn_dirty);
		kfree(mctx);
	}
	return -ENOMEM;
}

static int
ufshpb_map_req_mempool_init(struct ufshpb_lu *hpb)
{
	struct scsi_device *sdev;
	struct request_queue *q;
	struct ufshpb_req *map_req = NULL;
	int qd = hpb->qd;
	int i, j;

	sdev = hpb->ufsf->sdev_ufs_lu[hpb->lun];
	q = sdev->request_queue;

	INIT_LIST_HEAD(&hpb->lh_map_req_free);
	INIT_LIST_HEAD(&hpb->lh_map_req_retry);

	hpb->map_req = kzalloc(sizeof(struct ufshpb_req) * qd, GFP_KERNEL);
	if (!hpb->map_req)
		goto release_mem;

	/*
	 * q->cmd_size: sizeof(struct scsi_cmd) + shost->hostt->cmd_size
	 */
	for (i = 0; i < qd; i++) {
		map_req = hpb->map_req + i;
		INIT_LIST_HEAD(&map_req->list_req);
		map_req->req = kzalloc(sizeof(struct request) + q->cmd_size,
				       GFP_KERNEL);
		if (!map_req->req) {
			for (j = 0; j < i; j++)
				kfree(hpb->map_req[j].req);
			goto release_mem;
		}

		map_req->bio = bio_kmalloc(GFP_KERNEL, hpb->mpages_per_srgn);
		if (!map_req->bio) {
			kfree(hpb->map_req[i].req);
			for (j = 0; j < i; j++) {
				kfree(hpb->map_req[j].req);
				bio_put(hpb->map_req[j].bio);
			}
			goto release_mem;
		}
		list_add_tail(&map_req->list_req, &hpb->lh_map_req_free);
	}

	return 0;
release_mem:
	kfree(hpb->map_req);
	return -ENOMEM;
}

static int
ufshpb_pre_req_mempool_init(struct ufshpb_lu *hpb)
{
	struct scsi_device *sdev;
	struct request_queue *q;
	struct ufshpb_req *pre_req = NULL;
	int qd = hpb->qd;
	int i, j;

	INIT_LIST_HEAD(&hpb->lh_pre_req_free);
	INIT_LIST_HEAD(&hpb->lh_pre_req_dummy);

	sdev = hpb->ufsf->sdev_ufs_lu[hpb->lun];
	q = sdev->request_queue;

	hpb->pre_req = kzalloc(sizeof(struct ufshpb_req) * qd, GFP_KERNEL);
	if (!hpb->pre_req)
		goto release_mem;

	/*
	 * q->cmd_size: sizeof(struct scsi_cmd) + shost->hostt->cmd_size
	 */
	for (i = 0; i < qd; i++) {
		pre_req = hpb->pre_req + i;
		INIT_LIST_HEAD(&pre_req->list_req);
		pre_req->req = kzalloc(sizeof(struct request) + q->cmd_size,
				       GFP_KERNEL);
		if (!pre_req->req) {
			for (j = 0; j < i; j++)
				kfree(hpb->pre_req[j].req);
			goto release_mem;
		}

		pre_req->bio = bio_kmalloc(GFP_KERNEL, 1);
		if (!pre_req->bio) {
			kfree(hpb->pre_req[i].req);
			for (j = 0; j < i; j++) {
				kfree(hpb->pre_req[j].req);
				bio_put(hpb->pre_req[j].bio);
			}
			goto release_mem;
		}

		pre_req->wb.m_page = alloc_page(GFP_KERNEL | __GFP_ZERO);
		if (!pre_req->wb.m_page) {
			kfree(hpb->pre_req[i].req);
			bio_put(hpb->pre_req[i].bio);
			for (j = 0; j < i; j++) {
				kfree(hpb->pre_req[j].req);
				bio_put(hpb->pre_req[j].bio);
				__free_page(hpb->pre_req[j].wb.m_page);
			}
			goto release_mem;
		}
		list_add_tail(&pre_req->list_req, &hpb->lh_pre_req_free);
	}

	return 0;
release_mem:
	kfree(hpb->pre_req);
	return -ENOMEM;
}

static void ufshpb_find_lu_qd(struct ufshpb_lu *hpb)
{
	struct scsi_device *sdev;
	struct ufs_hba *hba;

	sdev = hpb->ufsf->sdev_ufs_lu[hpb->lun];
	hba = hpb->ufsf->hba;

	/*
	 * ufshcd_slave_alloc(sdev) -> ufshcd_set_queue_depth(sdev)
	 * a lu-queue-depth compared with lu_info and hba->nutrs
	 * is selected in ufshcd_set_queue_depth()
	 */
	hpb->qd = sdev->queue_depth;
	INFO_MSG("lu (%d) queue_depth (%d)", hpb->lun, hpb->qd);
	if (!hpb->qd) {
		hpb->qd = hba->nutrs;
		INFO_MSG("lu_queue_depth is 0. we use device's queue info.");
		INFO_MSG("hba->nutrs = (%d)", hba->nutrs);
	}

	hpb->throttle_map_req = hpb->qd;
	hpb->throttle_pre_req = hpb->qd;
	hpb->num_inflight_map_req = 0;
	hpb->num_inflight_pre_req = 0;
}

static void ufshpb_init_lu_constant(struct ufshpb_dev_info *hpb_dev_info,
				    struct ufshpb_lu *hpb)
{
	unsigned long long rgn_unit_size, rgn_mem_size;
	int entries_per_rgn;

	hpb->debug = false;

	ufshpb_find_lu_qd(hpb);

	/* for pre_req */
	hpb->pre_req_min_tr_len = HPB_MULTI_CHUNK_LOW;
	hpb->pre_req_max_tr_len = HPB_MULTI_CHUNK_HIGH;
	hpb->ctx_id_ticket = 0;

	/*	From descriptors	*/
	rgn_unit_size = (unsigned long long)
		SECTOR * (0x01 << hpb_dev_info->rgn_size);
	rgn_mem_size = rgn_unit_size / BLOCK * HPB_ENTRY_SIZE;

	hpb->srgn_unit_size = (unsigned long long)
		SECTOR * (0x01 << hpb_dev_info->srgn_size);
	hpb->srgn_mem_size = hpb->srgn_unit_size / BLOCK * HPB_ENTRY_SIZE;

	/* relation : lu <-> region <-> sub region <-> entry */
	entries_per_rgn = rgn_mem_size / HPB_ENTRY_SIZE;
	hpb->entries_per_srgn = hpb->srgn_mem_size / HPB_ENTRY_SIZE;
	hpb->srgns_per_rgn = rgn_mem_size / hpb->srgn_mem_size;

	/*
	 * regions_per_lu = (lu_num_blocks * 4096) / region_unit_size
	 *	          = (lu_num_blocks * HPB_ENTRY_SIZE) / region_mem_size
	 */
	hpb->rgns_per_lu =
		((unsigned long long)hpb->lu_num_blocks
		 + (rgn_mem_size / HPB_ENTRY_SIZE) - 1)
		/ (rgn_mem_size / HPB_ENTRY_SIZE);
	hpb->srgns_per_lu =
		((unsigned long long)hpb->lu_num_blocks
		 + (hpb->srgn_mem_size / HPB_ENTRY_SIZE) - 1)
		/ (hpb->srgn_mem_size / HPB_ENTRY_SIZE);

	/* mempool info */
	hpb->mpage_bytes = OS_PAGE_SIZE;
	hpb->mpages_per_srgn = hpb->srgn_mem_size / hpb->mpage_bytes;

	/* Bitmask Info. */
	hpb->dwords_per_srgn = hpb->entries_per_srgn / BITS_PER_DWORD;
	hpb->entries_per_rgn_shift = ffs(entries_per_rgn) - 1;
	hpb->entries_per_rgn_mask = entries_per_rgn - 1;
	hpb->entries_per_srgn_shift = ffs(hpb->entries_per_srgn) - 1;
	hpb->entries_per_srgn_mask = hpb->entries_per_srgn - 1;

	INFO_MSG("===== From Device Descriptor! =====");
	INFO_MSG("hpb_region_size = (%d), hpb_subregion_size = (%d)",
		 hpb_dev_info->rgn_size, hpb_dev_info->srgn_size);
	INFO_MSG("=====   Constant Values(LU)   =====");
	INFO_MSG("region_unit_size = (%lld), region_mem_size (%lld)",
		 rgn_unit_size, rgn_mem_size);
	INFO_MSG("subregion_unit_size = (%lld), subregion_mem_size (%d)",
		 hpb->srgn_unit_size, hpb->srgn_mem_size);

	INFO_MSG("lu_num_blocks = (%d)", hpb->lu_num_blocks);
	INFO_MSG("regions_per_lu = (%d), subregions_per_lu = (%d)",
		 hpb->rgns_per_lu, hpb->srgns_per_lu);

	INFO_MSG("subregions_per_region = %d", hpb->srgns_per_rgn);
	INFO_MSG("entries_per_region (%u) shift (%u) mask (0x%X)",
		 entries_per_rgn, hpb->entries_per_rgn_shift,
		 hpb->entries_per_rgn_mask);
	INFO_MSG("entries_per_subregion (%u) shift (%u) mask (0x%X)",
		 hpb->entries_per_srgn, hpb->entries_per_srgn_shift,
		 hpb->entries_per_srgn_mask);
	INFO_MSG("mpages_per_subregion : (%d)", hpb->mpages_per_srgn);
	INFO_MSG("===================================");
}

static int ufshpb_lu_hpb_init(struct ufsf_feature *ufsf, int lun)
{
	struct ufshpb_lu *hpb = ufsf->hpb_lup[lun];
	struct ufshpb_region *rgn_table, *rgn;
	struct ufshpb_subregion *srgn;
	int rgn_idx, srgn_idx, total_srgn_cnt, srgn_cnt, i, ret = 0;
	bool do_work_handler = false;

	ufshpb_init_lu_constant(&ufsf->hpb_dev_info, hpb);

	rgn_table = kzalloc(sizeof(struct ufshpb_region) * hpb->rgns_per_lu,
			    GFP_KERNEL);
	if (!rgn_table) {
		ret = -ENOMEM;
		goto out;
	}

	INFO_MSG("active_region_table bytes: (%lu)",
		 (sizeof(struct ufshpb_region) * hpb->rgns_per_lu));

	hpb->rgn_tbl = rgn_table;

	spin_lock_init(&hpb->hpb_lock);
	spin_lock_init(&hpb->retry_list_lock);
	spin_lock_init(&hpb->rsp_list_lock);

	/* init lru information */
	INIT_LIST_HEAD(&hpb->lru_info.lh_lru_rgn);
	hpb->lru_info.selection_type = LRU;

	INIT_LIST_HEAD(&hpb->lh_pinned_srgn);
	INIT_LIST_HEAD(&hpb->lh_act_srgn);
	INIT_LIST_HEAD(&hpb->lh_inact_rgn);

	INIT_LIST_HEAD(&hpb->lh_map_ctx_free);

	ufshpb_init_jobs(hpb);

	ret = ufshpb_map_req_mempool_init(hpb);
	if (ret) {
		ERR_MSG("map_req_mempool init fail!");
		goto release_rgn_table;
	}

	ret = ufshpb_pre_req_mempool_init(hpb);
	if (ret) {
		ERR_MSG("pre_req_mempool init fail!");
		goto release_map_req_mempool;
	}

	ret = ufshpb_table_mempool_init(hpb);
	if (ret) {
		ERR_MSG("ppn table mempool init fail!");
		ufshpb_table_mempool_remove(hpb);
		goto release_pre_req_mempool;
	}

	total_srgn_cnt = hpb->srgns_per_lu;
	INFO_MSG("total_subregion_count: (%d)", total_srgn_cnt);
	for (rgn_idx = 0, srgn_cnt = 0; rgn_idx < hpb->rgns_per_lu;
	     rgn_idx++, total_srgn_cnt -= srgn_cnt) {
		rgn = rgn_table + rgn_idx;
		rgn->rgn_idx = rgn_idx;

		INIT_LIST_HEAD(&rgn->list_inact_rgn);

		/* init lru region information*/
		INIT_LIST_HEAD(&rgn->list_lru_rgn);

		srgn_cnt = min(total_srgn_cnt, hpb->srgns_per_rgn);

		ret = ufshpb_alloc_subregion_tbl(hpb, rgn, srgn_cnt);
		if (ret)
			goto release_srgns;
		ufshpb_init_subregion_tbl(hpb, rgn);

		if (ufshpb_is_pinned_region(hpb, rgn_idx)) {
			ret = ufshpb_init_pinned_active_region(hpb, rgn);
			if (ret)
				goto release_srgns;

			do_work_handler = true;
		} else {
			rgn->rgn_state = HPB_RGN_INACTIVE;
		}
	}

	if (total_srgn_cnt != 0) {
		ERR_MSG("error total_subregion_count: %d",
			total_srgn_cnt);
		goto release_srgns;
	}

	/*
	 * even if creating sysfs failed, ufshpb could run normally.
	 * so we don't deal with error handling
	 */
	ufshpb_create_sysfs(ufsf, hpb);

	return 0;
release_srgns:
	for (i = 0; i < rgn_idx; i++) {
		rgn = rgn_table + i;
		if (rgn->srgn_tbl) {
			for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt;
			     srgn_idx++) {
				srgn = rgn->srgn_tbl + srgn_idx;
				if (srgn->mctx)
					ufshpb_put_map_ctx(hpb, srgn->mctx);
			}
			kfree(rgn->srgn_tbl);
		}
	}

	ufshpb_table_mempool_remove(hpb);
release_pre_req_mempool:
	ufshpb_pre_req_mempool_remove(hpb);
release_map_req_mempool:
	ufshpb_map_req_mempool_remove(hpb);
release_rgn_table:
	kfree(rgn_table);
out:
	return ret;
}

static inline int ufshpb_version_check(struct ufshpb_dev_info *hpb_dev_info)
{
	INFO_MSG("Support HPB Spec : UFSHPB_VER_2_0_0 = (%.4X)   UFSHPB_VER_2_0_1 = (%.4X)   Device = (%.4X)",
		 UFSHPB_VER_2_0_0, UFSHPB_VER_2_0_1,hpb_dev_info->version);

	INFO_MSG("HPB Driver Version : (%.6X%s)", UFSHPB_DD_VER, UFSHPB_DD_VER_POST);

	if (hpb_dev_info->version != UFSHPB_VER_2_0_0 &&
		hpb_dev_info->version != UFSHPB_VER_2_0_1 && hpb_dev_info->version != UFSHPB_VER_2_2_1) {
		ERR_MSG("ERROR: HPB Spec Version mismatch. So HPB disabled.");
		return -ENODEV;
	}
	return 0;
}

void ufshpb_get_dev_info(struct ufsf_feature *ufsf, u8 *desc_buf)
{
	struct ufshpb_dev_info *hpb_dev_info = &ufsf->hpb_dev_info;
	int ret;

	if (desc_buf[DEVICE_DESC_PARAM_UFS_FEAT] & UFS_FEATURE_SUPPORT_HPB_BIT)
		INFO_MSG("bUFSFeaturesSupport: HPB is set");
	else {
		INFO_MSG("bUFSFeaturesSupport: HPB not support");
		ufshpb_set_state(ufsf, HPB_FAILED);
		return;
	}

	hpb_dev_info->version = LI_EN_16(desc_buf + DEVICE_DESC_PARAM_HPB_VER);

	ret = ufshpb_version_check(hpb_dev_info);
	if (ret)
		ufshpb_set_state(ufsf, HPB_FAILED);
}

void ufshpb_get_geo_info(struct ufsf_feature *ufsf, u8 *geo_buf)
{
	struct ufshpb_dev_info *hpb_dev_info = &ufsf->hpb_dev_info;
	int hpb_device_max_active_rgns = 0;

	hpb_dev_info->num_lu = geo_buf[GEOMETRY_DESC_HPB_NUMBER_LU];
	if (hpb_dev_info->num_lu == 0) {
		ERR_MSG("Don't have a lu for hpb.");
		ufshpb_set_state(ufsf, HPB_FAILED);
		return;
	}

	hpb_dev_info->rgn_size = geo_buf[GEOMETRY_DESC_HPB_REGION_SIZE];
	hpb_dev_info->srgn_size = geo_buf[GEOMETRY_DESC_HPB_SUBREGION_SIZE];
	hpb_device_max_active_rgns =
		LI_EN_16(geo_buf + GEOMETRY_DESC_HPB_DEVICE_MAX_ACTIVE_REGIONS);

	INFO_MSG("[48] bHPBRegionSiz (%u)", hpb_dev_info->rgn_size);
	INFO_MSG("[49] bHPBNumberLU (%u)", hpb_dev_info->num_lu);
	INFO_MSG("[4A] bHPBSubRegionSize (%u)", hpb_dev_info->srgn_size);
	INFO_MSG("[4B:4C] wDeviceMaxActiveHPBRegions (%u)",
		 hpb_device_max_active_rgns);

	if (hpb_dev_info->rgn_size == 0 || hpb_dev_info->srgn_size == 0 ||
	    hpb_device_max_active_rgns == 0) {
		ERR_MSG("HPB NOT normally supported by device");
		ufshpb_set_state(ufsf, HPB_FAILED);
	}
}

void ufshpb_get_lu_info(struct ufsf_feature *ufsf, int lun, u8 *unit_buf)
{
	struct ufsf_lu_desc lu_desc;
	struct ufshpb_lu *hpb;

	lu_desc.lu_enable = unit_buf[UNIT_DESC_PARAM_LU_ENABLE];
	lu_desc.lu_queue_depth = unit_buf[UNIT_DESC_PARAM_LU_Q_DEPTH];
	lu_desc.lu_logblk_size = unit_buf[UNIT_DESC_PARAM_LOGICAL_BLK_SIZE];
	lu_desc.lu_logblk_cnt =
		LI_EN_64(unit_buf + UNIT_DESC_PARAM_LOGICAL_BLK_COUNT);
	lu_desc.lu_max_active_hpb_rgns =
		LI_EN_16(unit_buf + UNIT_DESC_HPB_LU_MAX_ACTIVE_REGIONS);
	lu_desc.lu_hpb_pinned_rgn_startidx =
		LI_EN_16(unit_buf + UNIT_DESC_HPB_LU_PIN_REGION_START_OFFSET);
	lu_desc.lu_num_hpb_pinned_rgns =
		LI_EN_16(unit_buf + UNIT_DESC_HPB_LU_NUM_PIN_REGIONS);

	if (lu_desc.lu_num_hpb_pinned_rgns > 0) {
		lu_desc.lu_hpb_pinned_end_offset =
			lu_desc.lu_hpb_pinned_rgn_startidx +
			lu_desc.lu_num_hpb_pinned_rgns - 1;
	} else
		lu_desc.lu_hpb_pinned_end_offset = PINNED_NOT_SET;

	INFO_MSG("LUN(%d) [0A] bLogicalBlockSize (%d)",
		 lun, lu_desc.lu_logblk_size);
	INFO_MSG("LUN(%d) [0B] qLogicalBlockCount (%llu)",
		 lun, lu_desc.lu_logblk_cnt);
	INFO_MSG("LUN(%d) [03] bLuEnable (%d)", lun, lu_desc.lu_enable);
	INFO_MSG("LUN(%d) [06] bLuQueueDepth (%d)", lun, lu_desc.lu_queue_depth);
	INFO_MSG("LUN(%d) [23:24] wLUMaxActiveHPBRegions (%d)",
		 lun, lu_desc.lu_max_active_hpb_rgns);
	INFO_MSG("LUN(%d) [25:26] wHPBPinnedRegionStartIdx (%d)",
		 lun, lu_desc.lu_hpb_pinned_rgn_startidx);
	INFO_MSG("LUN(%d) [27:28] wNumHPBPinnedRegions (%d)",
		 lun, lu_desc.lu_num_hpb_pinned_rgns);
	INFO_MSG("LUN(%d) PINNED Start (%d) End (%d)",
		 lun, lu_desc.lu_hpb_pinned_rgn_startidx,
		 lu_desc.lu_hpb_pinned_end_offset);

	ufsf->hpb_lup[lun] = NULL;

	if (lu_desc.lu_enable == 0x02 && lu_desc.lu_max_active_hpb_rgns) {
		ufsf->hpb_lup[lun] = kzalloc(sizeof(struct ufshpb_lu),
					     GFP_KERNEL);
		if (!ufsf->hpb_lup[lun]) {
			ERR_MSG("hpb_lup[%d] alloc failed", lun);
			return;
		}

		hpb = ufsf->hpb_lup[lun];
		hpb->ufsf = ufsf;
		hpb->lun = lun;
		hpb->lu_num_blocks = lu_desc.lu_logblk_cnt;
		hpb->lu_max_active_rgns = lu_desc.lu_max_active_hpb_rgns;
		hpb->lru_info.max_lru_active_cnt =
			lu_desc.lu_max_active_hpb_rgns -
			lu_desc.lu_num_hpb_pinned_rgns;
		hpb->lu_pinned_rgn_startidx =
			lu_desc.lu_hpb_pinned_rgn_startidx;
		hpb->lu_pinned_end_offset = lu_desc.lu_hpb_pinned_end_offset;
	} else {
		INFO_MSG("===== LU (%d) is hpb-disabled.", lun);
	}
}

static void ufshpb_error_handler(struct work_struct *work)
{
	struct ufsf_feature *ufsf;

	ufsf = container_of(work, struct ufsf_feature, hpb_eh_work);

	WARN_MSG("driver has failed. but UFSHCD can run without UFSHPB");
	WARN_MSG("UFSHPB will be removed from the kernel");

	ufshpb_remove(ufsf, HPB_FAILED);
}

extern int ufsplus_hpb_status;
static int ufshpb_init(struct ufsf_feature *ufsf)
{
	int lun, ret;
	struct ufshpb_lu *hpb;
	int hpb_enabled_lun = 0;

	seq_scan_lu(lun) {
		if (!ufsf->hpb_lup[lun])
			continue;

		/*
		 * HPB need info about request queue in order to issue
		 * RB-CMD for pinned region.
		 */
		if (!ufsf->sdev_ufs_lu[lun]) {
			WARN_MSG("lun (%d) don't have scsi_device", lun);
			continue;
		}

		ret = ufshpb_lu_hpb_init(ufsf, lun);
		if (ret) {
			kfree(ufsf->hpb_lup[lun]);
			continue;
		}
		hpb_enabled_lun++;
	}

	if(hpb_enabled_lun)
		ufsplus_hpb_status = 1;

	if (hpb_enabled_lun == 0) {
		ERR_MSG("No UFSHPB LU to init");
		ret = -ENODEV;
		goto hpb_failed;
	}

	INIT_WORK(&ufsf->hpb_eh_work, ufshpb_error_handler);

	kref_init(&ufsf->hpb_kref);
	ufshpb_set_state(ufsf, HPB_PRESENT);

	seq_scan_lu(lun)
		if (ufsf->hpb_lup[lun]) {
			hpb = ufsf->hpb_lup[lun];

			INFO_MSG("UFSHPB LU %d working", lun);
			if (hpb->lu_pinned_end_offset != PINNED_NOT_SET)
				schedule_work(&hpb->pinned_work);
		}
	create_hpbfn_enable_proc();

	return 0;
hpb_failed:
	ufshpb_set_state(ufsf, HPB_FAILED);
	return ret;
}

static void ufshpb_drop_retry_list(struct ufshpb_lu *hpb)
{
	struct ufshpb_req *map_req, *next;
	unsigned long flags;

	if (list_empty(&hpb->lh_map_req_retry))
		return;

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	list_for_each_entry_safe(map_req, next, &hpb->lh_map_req_retry,
				 list_req) {
		INFO_MSG("drop map_req %p ( %d - %d )", map_req,
			 map_req->rb.rgn_idx, map_req->rb.srgn_idx);

		list_del_init(&map_req->list_req);

		ufshpb_put_map_req(hpb, map_req);
	}
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
}

static void ufshpb_drop_rsp_lists(struct ufshpb_lu *hpb)
{
	struct ufshpb_region *rgn, *next_rgn;
	struct ufshpb_subregion *srgn, *next_srgn;
	unsigned long flags;

	spin_lock_irqsave(&hpb->rsp_list_lock, flags);
	list_for_each_entry_safe(rgn, next_rgn, &hpb->lh_inact_rgn,
				 list_inact_rgn) {
		list_del_init(&rgn->list_inact_rgn);
	}

	list_for_each_entry_safe(srgn, next_srgn, &hpb->lh_act_srgn,
				 list_act_srgn) {
		list_del_init(&srgn->list_act_srgn);
	}
	spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);
}

static void ufshpb_destroy_subregion_tbl(struct ufshpb_lu *hpb,
					 struct ufshpb_region *rgn)
{
	int srgn_idx;

	for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
		struct ufshpb_subregion *srgn;

		srgn = rgn->srgn_tbl + srgn_idx;
		srgn->srgn_state = HPB_SRGN_UNUSED;

		ufshpb_put_map_ctx(hpb, srgn->mctx);
	}

	kfree(rgn->srgn_tbl);
}

static void ufshpb_destroy_region_tbl(struct ufshpb_lu *hpb)
{
	int rgn_idx;

	INFO_MSG("Start");

	for (rgn_idx = 0; rgn_idx < hpb->rgns_per_lu; rgn_idx++) {
		struct ufshpb_region *rgn;

		rgn = hpb->rgn_tbl + rgn_idx;
		if (rgn->rgn_state == HPB_RGN_PINNED ||
		    rgn->rgn_state == HPB_RGN_ACTIVE) {
			rgn->rgn_state = HPB_RGN_INACTIVE;

			ufshpb_destroy_subregion_tbl(hpb, rgn);
		}
	}

	ufshpb_table_mempool_remove(hpb);
	kfree(hpb->rgn_tbl);

	INFO_MSG("End");
}

void ufshpb_remove(struct ufsf_feature *ufsf, int state)
{
	struct ufshpb_lu *hpb;
	int lun;

	INFO_MSG("start release");
	ufshpb_set_state(ufsf, HPB_FAILED);

	INFO_MSG("kref count (%d)",
		 atomic_read(&ufsf->hpb_kref.refcount.refs));

	remove_hpbfn_enable_proc();
	seq_scan_lu(lun) {
		hpb = ufsf->hpb_lup[lun];

		INFO_MSG("lun (%d) (%p)", lun, hpb);

		ufsf->hpb_lup[lun] = NULL;

		if (!hpb)
			continue;

		ufshpb_cancel_jobs(hpb);

		ufshpb_destroy_region_tbl(hpb);
		if (hpb->alloc_mctx != 0)
			WARN_MSG("warning: alloc_mctx (%d)", hpb->alloc_mctx);

		ufshpb_map_req_mempool_remove(hpb);

		ufshpb_pre_req_mempool_remove(hpb);

		ufshpb_remove_sysfs(hpb);

		kfree(hpb);
	}

	ufshpb_set_state(ufsf, state);

	INFO_MSG("end release");
}

void ufshpb_reset(struct ufsf_feature *ufsf)
{
	ufshpb_set_state(ufsf, HPB_PRESENT);
}

void ufshpb_reset_host(struct ufsf_feature *ufsf)
{
	struct ufshpb_lu *hpb;
	int lun;

	ufshpb_set_state(ufsf, HPB_RESET);
	seq_scan_lu(lun) {
		hpb = ufsf->hpb_lup[lun];
		if (hpb) {
			INFO_MSG("UFSHPB lun %d reset", lun);
			ufshpb_cancel_jobs(hpb);
			ufshpb_drop_retry_list(hpb);
			ufshpb_drop_rsp_lists(hpb);
		}
	}
}

static inline int ufshpb_probe_lun_done(struct ufsf_feature *ufsf)
{
	return (ufsf->num_lu == ufsf->slave_conf_cnt);
}

void ufshpb_init_handler(struct work_struct *work)
{
	struct ufsf_feature *ufsf;
	int ret = 0;

	ufsf = container_of(work, struct ufsf_feature, hpb_init_work);

	INFO_MSG("probe_done(%d)", ufshpb_probe_lun_done(ufsf));

	ret = wait_event_timeout(ufsf->hpb_wait,
				 ufshpb_probe_lun_done(ufsf),
				 msecs_to_jiffies(10000));
	if (ret == 0)
		ERR_MSG("Probing LU is not fully complete.");

	INFO_MSG("probe_done(%d) ret(%d)", ufshpb_probe_lun_done(ufsf), ret);
	INFO_MSG("HPB_INIT_START");

	ret = ufshpb_init(ufsf);
	if (ret)
		ERR_MSG("UFSHPB driver init failed. (%d)", ret);
}

void ufshpb_suspend(struct ufsf_feature *ufsf)
{
	struct ufshpb_lu *hpb;
	int lun;

	seq_scan_lu(lun) {
		hpb = ufsf->hpb_lup[lun];
		if (hpb) {
			INFO_MSG("ufshpb_lu %d goto suspend", lun);
			INFO_MSG("ufshpb_lu %d changes suspend state", lun);
			ufshpb_set_state(ufsf, HPB_SUSPEND);
			ufshpb_cancel_jobs(hpb);
		}
	}
}

void ufshpb_resume(struct ufsf_feature *ufsf)
{
	struct ufshpb_lu *hpb;
	int lun;

	seq_scan_lu(lun) {
		hpb = ufsf->hpb_lup[lun];
		if (hpb) {
			bool do_task_work = false;
			bool do_retry_work = false;

			ufshpb_set_state(ufsf, HPB_PRESENT);

			do_task_work = !ufshpb_is_empty_rsp_lists(hpb);
			do_retry_work =
				!list_empty_careful(&hpb->lh_map_req_retry);

			INFO_MSG("ufshpb_lu %d resume. do_task_work %d retry %d",
				 lun, do_task_work, do_retry_work);

			if (do_task_work)
				schedule_work(&hpb->task_work);
			if (do_retry_work)
				schedule_delayed_work(&hpb->retry_work,
						      msecs_to_jiffies(100));
		}
	}
}

static void ufshpb_stat_init(struct ufshpb_lu *hpb)
{
	atomic64_set(&hpb->hit, 0);
	atomic64_set(&hpb->miss, 0);
	atomic64_set(&hpb->rb_noti_cnt, 0);
	atomic64_set(&hpb->rb_active_cnt, 0);
	atomic64_set(&hpb->rb_inactive_cnt, 0);
	atomic64_set(&hpb->map_req_cnt, 0);
	atomic64_set(&hpb->pre_req_cnt, 0);
	ufsf_para.hit = 0;
	ufsf_para.miss = 0;
	ufsf_para.hit_4k = 0;
	ufsf_para.hit_8_32k = 0;
	ufsf_para.span = 0;
	ufsf_para.span_hit = 0;
	ufsf_para.noti = 0;
	ufsf_para.noti_act = 0;
	ufsf_para.noti_inact = 0;
	ufsf_para.rgn_act = 0;
	ufsf_para.map_req = 0;
	ufsf_para.pre_req = 0;
}

/* SYSFS functions */
static ssize_t ufshpb_sysfs_prep_disable_show(struct ufshpb_lu *hpb, char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "force_hpb_read_disable %d\n",
		       hpb->force_disable);

	INFO_MSG("force_hpb_read_disable %d", hpb->force_disable);
	return ret;
}

static ssize_t ufshpb_sysfs_prep_disable_store(struct ufshpb_lu *hpb,
					       const char *buf, size_t cnt)
{
	unsigned long value;

	if (kstrtoul(buf, 0, &value))
		return -EINVAL;

	if (value > 1)
		return -EINVAL;

	if (value == 1)
		hpb->force_disable = true;
	else if (value == 0)
		hpb->force_disable = false;

	INFO_MSG("force_hpb_read_disable %d", hpb->force_disable);

	return cnt;
}

static ssize_t ufshpb_sysfs_map_disable_show(struct ufshpb_lu *hpb, char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "force_map_req_disable %d\n",
		       hpb->force_map_req_disable);

	INFO_MSG("force_map_req_disable %d", hpb->force_map_req_disable);

	return ret;
}

static ssize_t ufshpb_sysfs_map_disable_store(struct ufshpb_lu *hpb,
					      const char *buf, size_t cnt)
{
	unsigned long value;

	if (kstrtoul(buf, 0, &value))
		return -EINVAL;

	if (value > 1)
		return -EINVAL;

	if (value == 1)
		hpb->force_map_req_disable = true;
	else if (value == 0)
		hpb->force_map_req_disable = false;

	INFO_MSG("force_map_req_disable %d", hpb->force_map_req_disable);

	return cnt;
}

static ssize_t ufshpb_sysfs_throttle_map_req_show(struct ufshpb_lu *hpb,
						  char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "throttle_map_req %d\n",
		       hpb->throttle_map_req);

	INFO_MSG("throttle_map_req %d", hpb->throttle_map_req);

	return ret;
}

static ssize_t ufshpb_sysfs_throttle_map_req_store(struct ufshpb_lu *hpb,
						   const char *buf, size_t cnt)
{
	unsigned long throttle_map_req;

	if (kstrtoul(buf, 0, &throttle_map_req))
		return -EINVAL;

	hpb->throttle_map_req = (int)throttle_map_req;

	INFO_MSG("throttle_map_req %d", hpb->throttle_map_req);

	return cnt;
}

static ssize_t ufshpb_sysfs_throttle_pre_req_show(struct ufshpb_lu *hpb,
						  char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "throttle_pre_req %d\n",
		       hpb->throttle_pre_req);

	INFO_MSG("throttle_pre_req %d", hpb->throttle_pre_req);

	return ret;
}

static ssize_t ufshpb_sysfs_throttle_pre_req_store(struct ufshpb_lu *hpb,
						   const char *buf, size_t cnt)
{
	unsigned long throttle_pre_req;

	if (kstrtoul(buf, 0, &throttle_pre_req))
		return -EINVAL;

	hpb->throttle_pre_req = (int)throttle_pre_req;

	INFO_MSG("throttle_pre_req %d", hpb->throttle_pre_req);

	return cnt;
}

static ssize_t ufshpb_sysfs_pre_req_min_tr_len_show(struct ufshpb_lu *hpb,
						    char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "%d", hpb->pre_req_min_tr_len);
	INFO_MSG("pre_req min transfer len %d", hpb->pre_req_min_tr_len);

	return ret;
}

static ssize_t ufshpb_sysfs_pre_req_min_tr_len_store(struct ufshpb_lu *hpb,
						     const char *buf,
						     size_t count)
{
	unsigned long val;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val < 0)
		val = 0;

	if (hpb->pre_req_max_tr_len < val || val < HPB_MULTI_CHUNK_LOW)
		INFO_MSG("value is wrong. pre_req transfer len %d ~ %d",
			 HPB_MULTI_CHUNK_LOW, hpb->pre_req_max_tr_len);
	else
		hpb->pre_req_min_tr_len = val;

	INFO_MSG("pre_req min transfer len %d", hpb->pre_req_min_tr_len);

	return count;
}

static ssize_t ufshpb_sysfs_pre_req_max_tr_len_show(struct ufshpb_lu *hpb,
						    char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "%d", hpb->pre_req_max_tr_len);
	INFO_MSG("pre_req max transfer len %d", hpb->pre_req_max_tr_len);

	return ret;
}

static ssize_t ufshpb_sysfs_pre_req_max_tr_len_store(struct ufshpb_lu *hpb,
						     const char *buf,
						     size_t count)
{
	unsigned long val;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (hpb->pre_req_min_tr_len > val || val > HPB_MULTI_CHUNK_HIGH)
		INFO_MSG("value is wrong. pre_req transfer len %d ~ %d",
			 hpb->pre_req_min_tr_len, HPB_MULTI_CHUNK_HIGH);
	else
		hpb->pre_req_max_tr_len = val;

	INFO_MSG("pre_req max transfer len %d", hpb->pre_req_max_tr_len);

	return count;
}

static ssize_t ufshpb_sysfs_debug_show(struct ufshpb_lu *hpb, char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "debug %d\n", hpb->debug);

	INFO_MSG("debug %d", hpb->debug);

	return ret;
}

static ssize_t ufshpb_sysfs_debug_store(struct ufshpb_lu *hpb,
					const char *buf, size_t cnt)
{
	unsigned long debug;

	if (kstrtoul(buf, 0, &debug))
		return -EINVAL;

	if (debug >= 1)
		hpb->debug = 1;
	else
		hpb->debug = 0;

	INFO_MSG("debug %d", hpb->debug);

	return cnt;
}

#if defined(CONFIG_HPB_ERR_INJECTION)
static ssize_t ufshpb_sysfs_err_injection_select_show(struct ufshpb_lu *hpb,
						      char *buf)
{
	char error_injection[8];
	int ret;

	if (hpb->err_injection_select == HPB_ERR_INJECTION_DISABLE)
		strlcpy(error_injection, "disable", 8);
	else if (hpb->err_injection_select == HPB_ERR_INJECTION_BITFLIP)
		strlcpy(error_injection, "bitflip", 8);
	else if (hpb->err_injection_select == HPB_ERR_INJECTION_OFFSET)
		strlcpy(error_injection, "offset", 7);
	else if (hpb->err_injection_select == HPB_ERR_INJECTION_RANDOM)
		strlcpy(error_injection, "random", 7);

	ret = snprintf(buf, PAGE_SIZE, "err_injection_select %s\n",
		       error_injection);

	INFO_MSG("err_injection_select %s", error_injection);

	return ret;
}

static ssize_t ufshpb_sysfs_err_injection_select_store(struct ufshpb_lu *hpb,
						       const char *buf,
						       size_t cnt)
{
	char error_injection[8];
	int ret;

	error_injection[7] = '\0';

	ret = sscanf(buf, "%7s", error_injection);
	if (!ret) {
		INFO_MSG("input failed...");
		return cnt;
	}

	if (!strncmp(error_injection, "disable", 7))
		hpb->err_injection_select = HPB_ERR_INJECTION_DISABLE;
	else if (!strncmp(error_injection, "bitflip", 7))
		hpb->err_injection_select = HPB_ERR_INJECTION_BITFLIP;
	else if (!strncmp(error_injection, "offset", 6))
		hpb->err_injection_select = HPB_ERR_INJECTION_OFFSET;
	else if (!strncmp(error_injection, "random", 6))
		hpb->err_injection_select = HPB_ERR_INJECTION_RANDOM;
	else {
		hpb->err_injection_select = HPB_ERR_INJECTION_DISABLE;
		strlcpy(error_injection, "disable", 8);
	}

	INFO_MSG("err_injection_select %s", error_injection);

	return cnt;
}

static ssize_t ufshpb_sysfs_err_injection_bitflip_show(struct ufshpb_lu *hpb,
						       char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "err_injection_bitflip %llx\n",
		       hpb->err_injection_bitflip);

	INFO_MSG("err_injection_bitflip %llx", hpb->err_injection_bitflip);

	return ret;
}

static ssize_t ufshpb_sysfs_err_injection_bitflip_store(struct ufshpb_lu *hpb,
							const char *buf,
							size_t cnt)
{
	unsigned long long err_injection_bitflip;

	if (kstrtoull(buf, 0, &err_injection_bitflip))
		return -EINVAL;

	hpb->err_injection_bitflip = err_injection_bitflip;

	INFO_MSG("err_injection_bitflip %llx", hpb->err_injection_bitflip);

	return cnt;
}

static ssize_t ufshpb_sysfs_err_injection_offset_show(struct ufshpb_lu *hpb,
						      char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "err_injection_offset %lld\n",
		       hpb->err_injection_offset);

	INFO_MSG("err_injection_offset %lld", hpb->err_injection_offset);

	return ret;
}

static ssize_t ufshpb_sysfs_err_injection_offset_store(struct ufshpb_lu *hpb,
						       const char *buf,
						       size_t cnt)
{
	unsigned long long err_injection_offset;

	if (kstrtoull(buf, 0, &err_injection_offset))
		return -EINVAL;

	hpb->err_injection_offset = err_injection_offset;

	INFO_MSG("err_injection_offset %lld", hpb->err_injection_offset);

	return cnt;
}

static ssize_t ufshpb_sysfs_err_injection_random_show(struct ufshpb_lu *hpb,
						      char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "err_injection_random %d\n",
		       hpb->err_injection_random);

	INFO_MSG("err_injection_random %d", hpb->err_injection_random);

	return ret;
}

static ssize_t ufshpb_sysfs_err_injection_random_store(struct ufshpb_lu *hpb,
						       const char *buf,
						       size_t cnt)
{
	unsigned long err_injection_random;

	if (kstrtoul(buf, 0, &err_injection_random))
		return -EINVAL;

	if (err_injection_random >= 1)
		hpb->err_injection_random = 1;
	else
		hpb->err_injection_random = 0;

	INFO_MSG("err_injection_random %d", hpb->err_injection_random);

	return cnt;
}
#endif

static ssize_t ufshpb_sysfs_version_show(struct ufshpb_lu *hpb, char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "HPB version %.4X D/D version %.6X%s\n",
		       hpb->ufsf->hpb_dev_info.version,
		       UFSHPB_DD_VER, UFSHPB_DD_VER_POST);

	INFO_MSG("HPB version %.4X D/D version %.6X%s",
		 hpb->ufsf->hpb_dev_info.version,
		 UFSHPB_DD_VER, UFSHPB_DD_VER_POST);

	return ret;
}

static ssize_t ufshpb_sysfs_hit_show(struct ufshpb_lu *hpb, char *buf)
{
	long long hit_cnt, hit_4k_cnt, hit_8_32k_cnt, span_cnt, span_hit_cnt;
	int ret;

	hit_cnt = atomic64_read(&hpb->hit);
	hit_4k_cnt = ufsf_para.hit_4k;
	hit_8_32k_cnt = ufsf_para.hit_8_32k;
	span_cnt = ufsf_para.span;
	span_hit_cnt = ufsf_para.span_hit;

	ret = snprintf(buf, PAGE_SIZE,
		       "hit_count %lld hit_4k_count %lld hit_8-32k_count %lld"
		       " span_cnt %lld span_hit_cnt %lld\n", hit_cnt,
		       hit_4k_cnt, hit_8_32k_cnt, span_cnt, span_hit_cnt);
	INFO_MSG("%s", buf);

	return ret;
}

static ssize_t ufshpb_sysfs_miss_show(struct ufshpb_lu *hpb, char *buf)
{
	long long miss_cnt;
	int ret;

	miss_cnt = atomic64_read(&hpb->miss);

	ret = snprintf(buf, PAGE_SIZE, "miss_count %lld\n", miss_cnt);

	INFO_MSG("miss_count %lld", miss_cnt);

	return ret;
}

static ssize_t ufshpb_sysfs_map_req_show(struct ufshpb_lu *hpb, char *buf)
{
	long long rb_noti_cnt, rb_active_cnt, rb_inactive_cnt, map_req_cnt;
	int ret;

	rb_noti_cnt = atomic64_read(&hpb->rb_noti_cnt);
	rb_active_cnt = atomic64_read(&hpb->rb_active_cnt);
	rb_inactive_cnt = atomic64_read(&hpb->rb_inactive_cnt);
	map_req_cnt = atomic64_read(&hpb->map_req_cnt);

	ret = snprintf(buf, PAGE_SIZE,
		       "rb_noti %lld ACT %lld INACT %lld map_req_count %lld\n",
		       rb_noti_cnt, rb_active_cnt, rb_inactive_cnt,
		       map_req_cnt);

	INFO_MSG("rb_noti %lld ACT %lld INACT %lld map_req_count %lld",
		 rb_noti_cnt, rb_active_cnt, rb_inactive_cnt,
		 map_req_cnt);

	return ret;
}

static ssize_t ufshpb_sysfs_pre_req_show(struct ufshpb_lu *hpb, char *buf)
{
	long long pre_req_cnt;
	int ret;

	pre_req_cnt = atomic64_read(&hpb->pre_req_cnt);

	ret = snprintf(buf, PAGE_SIZE, "pre_req_count %lld\n", pre_req_cnt);

	INFO_MSG("pre_req_count %lld", pre_req_cnt);

	return ret;
}

static ssize_t ufshpb_sysfs_region_stat_show(struct ufshpb_lu *hpb, char *buf)
{
	int ret, pin_cnt = 0, act_cnt = 0, inact_cnt = 0, rgn_idx;
	enum HPB_RGN_STATE state;

	for (rgn_idx = 0; rgn_idx < hpb->rgns_per_lu; rgn_idx++) {
		state = hpb->rgn_tbl[rgn_idx].rgn_state;
		if (state == HPB_RGN_PINNED)
			pin_cnt++;
		else if (state == HPB_RGN_ACTIVE)
			act_cnt++;
		else if (state == HPB_RGN_INACTIVE)
			inact_cnt++;
	}

	ret = snprintf(buf, PAGE_SIZE,
		       "Total %d pinned %d active %d inactive %d\n",
		       hpb->rgns_per_lu, pin_cnt, act_cnt, inact_cnt);

	INFO_MSG("Total %d pinned %d active %d inactive %d",
		 hpb->rgns_per_lu, pin_cnt, act_cnt, inact_cnt);

	return ret;
}

static ssize_t ufshpb_sysfs_count_reset_store(struct ufshpb_lu *hpb,
					      const char *buf, size_t cnt)
{
	unsigned long debug;

	if (kstrtoul(buf, 0, &debug))
		return -EINVAL;

	INFO_MSG("Stat Init");

	ufshpb_stat_init(hpb);

	return cnt;
}

static ssize_t ufshpb_sysfs_info_lba_store(struct ufshpb_lu *hpb,
					   const char *buf, size_t cnt)
{
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	u64 ppn = 0;
	unsigned long value, lpn, flags;
	int rgn_idx, srgn_idx, srgn_offset, error = 0;

	if (kstrtoul(buf, 0, &value)) {
		ERR_MSG("kstrtoul error");
		return -EINVAL;
	}

	if (value > hpb->lu_num_blocks * SECTORS_PER_BLOCK) {
		ERR_MSG("value %lu > lu_num_blocks %d error",
			value, hpb->lu_num_blocks);
		return -EINVAL;
	}

	lpn = value / SECTORS_PER_BLOCK;

	ufshpb_get_pos_from_lpn(hpb, lpn, &rgn_idx, &srgn_idx, &srgn_offset);

	rgn = hpb->rgn_tbl + rgn_idx;
	srgn = rgn->srgn_tbl + srgn_idx;

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	INFO_MSG("lba %lu lpn %lu region %d state %d subregion %d state %d",
		 value, lpn, rgn_idx, rgn->rgn_state, srgn_idx,
		 srgn->srgn_state);

	if (!ufshpb_valid_srgn(rgn, srgn)) {
		INFO_MSG("[region %d subregion %d] has not valid hpb info.",
			 rgn_idx, srgn_idx);
		goto out;
	}

	if (!srgn->mctx) {
		INFO_MSG("mctx is NULL");
		goto out;
	}

	ppn = ufshpb_get_ppn(srgn->mctx, srgn_offset, &error);
	if (error) {
		INFO_MSG("getting ppn is fail from a page.");
		goto out;
	}

	INFO_MSG("ppn %llx is_dirty %d", ppn,
		 ufshpb_ppn_dirty_check(hpb, lpn, 1));
out:
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	return cnt;
}

static ssize_t ufshpb_sysfs_info_region_store(struct ufshpb_lu *hpb,
					      const char *buf, size_t cnt)
{
	unsigned long rgn_idx;
	int srgn_idx;

	if (kstrtoul(buf, 0, &rgn_idx))
		return -EINVAL;

	if (rgn_idx >= hpb->rgns_per_lu)
		ERR_MSG("error region %ld max %d", rgn_idx, hpb->rgns_per_lu);
	else {
		INFO_MSG("(region state : PINNED=%d ACTIVE=%d INACTIVE=%d)",
			 HPB_RGN_PINNED, HPB_RGN_ACTIVE, HPB_RGN_INACTIVE);

		INFO_MSG("region %ld state %d", rgn_idx,
			 hpb->rgn_tbl[rgn_idx].rgn_state);

		for (srgn_idx = 0; srgn_idx < hpb->rgn_tbl[rgn_idx].srgn_cnt;
		     srgn_idx++) {
			INFO_MSG("--- subregion %d state %d", srgn_idx,
				 hpb->rgn_tbl[rgn_idx].srgn_tbl[srgn_idx].srgn_state);
		}
	}

	return cnt;
}

static ssize_t ufshpb_sysfs_ufshpb_release_store(struct ufshpb_lu *hpb,
						 const char *buf, size_t cnt)
{
	unsigned long value;

	INFO_MSG("start release function");

	if (kstrtoul(buf, 0, &value)) {
		ERR_MSG("kstrtoul error");
		return -EINVAL;
	}

	if (value == 0xab) {
		INFO_MSG("magic number %lu release start", value);
		goto err_out;
	} else
		INFO_MSG("wrong magic number %lu", value);

	return cnt;
err_out:
	INFO_MSG("ref_cnt %d",
		 atomic_read(&hpb->ufsf->hpb_kref.refcount.refs));
	ufshpb_failed(hpb, __func__);

	return cnt;
}

static struct ufshpb_sysfs_entry ufshpb_sysfs_entries[] = {
	__ATTR(hpb_read_disable, 0644,
	       ufshpb_sysfs_prep_disable_show, ufshpb_sysfs_prep_disable_store),
	__ATTR(map_cmd_disable, 0644,
	       ufshpb_sysfs_map_disable_show, ufshpb_sysfs_map_disable_store),
	__ATTR(throttle_map_req, 0644,
	       ufshpb_sysfs_throttle_map_req_show,
	       ufshpb_sysfs_throttle_map_req_store),
	__ATTR(throttle_pre_req, 0644,
	       ufshpb_sysfs_throttle_pre_req_show,
	       ufshpb_sysfs_throttle_pre_req_store),
	__ATTR(pre_req_min_tr_len, 0644,
	       ufshpb_sysfs_pre_req_min_tr_len_show,
	       ufshpb_sysfs_pre_req_min_tr_len_store),
	__ATTR(pre_req_max_tr_len, 0644,
	       ufshpb_sysfs_pre_req_max_tr_len_show,
	       ufshpb_sysfs_pre_req_max_tr_len_store),
	__ATTR(debug, 0644,
	       ufshpb_sysfs_debug_show, ufshpb_sysfs_debug_store),
#if defined(CONFIG_HPB_ERR_INJECTION)
	__ATTR(err_injection_select, 0644,
	       ufshpb_sysfs_err_injection_select_show,
	       ufshpb_sysfs_err_injection_select_store),
	__ATTR(err_injection_bitflip, 0644,
	       ufshpb_sysfs_err_injection_bitflip_show,
	       ufshpb_sysfs_err_injection_bitflip_store),
	__ATTR(err_injection_offset, 0644,
	       ufshpb_sysfs_err_injection_offset_show,
	       ufshpb_sysfs_err_injection_offset_store),
	__ATTR(err_injection_random, 0644,
	       ufshpb_sysfs_err_injection_random_show,
	       ufshpb_sysfs_err_injection_random_store),
#endif
	__ATTR(hpb_version, 0444, ufshpb_sysfs_version_show, NULL),
	__ATTR(hit_count, 0444, ufshpb_sysfs_hit_show, NULL),
	__ATTR(miss_count, 0444, ufshpb_sysfs_miss_show, NULL),
	__ATTR(map_req_count, 0444, ufshpb_sysfs_map_req_show, NULL),
	__ATTR(pre_req_count, 0444, ufshpb_sysfs_pre_req_show, NULL),
	__ATTR(region_stat_count, 0444, ufshpb_sysfs_region_stat_show, NULL),
	__ATTR(count_reset, 0200, NULL, ufshpb_sysfs_count_reset_store),
	__ATTR(get_info_from_lba, 0200, NULL, ufshpb_sysfs_info_lba_store),
	__ATTR(get_info_from_region, 0200, NULL,
	       ufshpb_sysfs_info_region_store),
	__ATTR(release, 0200, NULL, ufshpb_sysfs_ufshpb_release_store),
	__ATTR_NULL
};

static ssize_t ufshpb_attr_show(struct kobject *kobj, struct attribute *attr,
				char *page)
{
	struct ufshpb_sysfs_entry *entry;
	struct ufshpb_lu *hpb;
	ssize_t error;

	entry = container_of(attr, struct ufshpb_sysfs_entry, attr);
	hpb = container_of(kobj, struct ufshpb_lu, kobj);

	if (!entry->show)
		return -EIO;

	mutex_lock(&hpb->sysfs_lock);
	error = entry->show(hpb, page);
	mutex_unlock(&hpb->sysfs_lock);
	return error;
}

static ssize_t ufshpb_attr_store(struct kobject *kobj, struct attribute *attr,
				 const char *page, size_t len)
{
	struct ufshpb_sysfs_entry *entry;
	struct ufshpb_lu *hpb;
	ssize_t error;

	entry = container_of(attr, struct ufshpb_sysfs_entry, attr);
	hpb = container_of(kobj, struct ufshpb_lu, kobj);

	if (!entry->store)
		return -EIO;

	mutex_lock(&hpb->sysfs_lock);
	error = entry->store(hpb, page, len);
	mutex_unlock(&hpb->sysfs_lock);
	return error;
}

static const struct sysfs_ops ufshpb_sysfs_ops = {
	.show = ufshpb_attr_show,
	.store = ufshpb_attr_store,
};

static struct kobj_type ufshpb_ktype = {
	.sysfs_ops = &ufshpb_sysfs_ops,
	.release = NULL,
};

static int ufshpb_create_sysfs(struct ufsf_feature *ufsf, struct ufshpb_lu *hpb)
{
	struct device *dev = ufsf->hba->dev;
	struct ufshpb_sysfs_entry *entry;
	int err;

	hpb->sysfs_entries = ufshpb_sysfs_entries;

	ufshpb_stat_init(hpb);

	kobject_init(&hpb->kobj, &ufshpb_ktype);
	mutex_init(&hpb->sysfs_lock);

	INFO_MSG("ufshpb creates sysfs lu %d %p dev->kobj %p", hpb->lun,
		 &hpb->kobj, &dev->kobj);

	err = kobject_add(&hpb->kobj, kobject_get(&dev->kobj),
			  "ufshpb_lu%d", hpb->lun);
	if (!err) {
		for (entry = hpb->sysfs_entries; entry->attr.name != NULL;
		     entry++) {
			INFO_MSG("ufshpb_lu%d sysfs attr creates: %s",
				 hpb->lun, entry->attr.name);
			if (sysfs_create_file(&hpb->kobj, &entry->attr))
				break;
		}
		INFO_MSG("ufshpb_lu%d sysfs adds uevent", hpb->lun);
		kobject_uevent(&hpb->kobj, KOBJ_ADD);
	}

	return err;
}

static inline void ufshpb_remove_sysfs(struct ufshpb_lu *hpb)
{
	kobject_uevent(&hpb->kobj, KOBJ_REMOVE);
	INFO_MSG("ufshpb removes sysfs lu %d %p ", hpb->lun, &hpb->kobj);
	kobject_del(&hpb->kobj);
}

static inline void hpbfn_enable_ctrl(struct ufshpb_lu *hpb, long val)
{
	switch (val) {
	case 0:
		hpb->force_map_req_disable = true;
		hpb->force_disable = true;
		break;
	case 1:
		hpb->force_map_req_disable = false;
		hpb->force_disable = false;
		break;
	case 2:
		ufshpb_failed(hpb, __func__);
		break;
	default:
		break;
	}

	return;
}

static ssize_t hpbfn_enable_write(struct file *filp, const char *ubuf,
				  size_t cnt, loff_t *data)
{
	char buf[64] = {0};
	long val = 64;
	int lun;

	if (!ufsf_para.ufsf)
		return -EFAULT;
	if (cnt >= sizeof(buf))
		return -EINVAL;
	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;

	if (buf[0] == '0')
		val = 0;
	else if (buf[0] == '1')
		val = 1;
	else if (buf[0] == '2')
		val = 2;
	else
		val = 64;

	seq_scan_lu(lun) {
		if (ufsf_para.ufsf->hpb_lup[lun])
			hpbfn_enable_ctrl(ufsf_para.ufsf->hpb_lup[lun],
					  val);
	}

	return cnt;
}

static const struct file_operations hpbfn_enable_fops = {
	.write = hpbfn_enable_write,
};

static int create_hpbfn_enable_proc(void)
{
	struct proc_dir_entry *d_entry;

	if (!ufsf_para.ctrl_dir)
		return -EFAULT;

	d_entry = proc_create("hpbfn_enable", S_IWUGO, ufsf_para.ctrl_dir,
			      &hpbfn_enable_fops);
	if(!d_entry)
		return -ENOMEM;

	return 0;
}

static void remove_hpbfn_enable_proc(void)
{
	if (ufsf_para.ctrl_dir)
		remove_proc_entry("hpbfn_enable", ufsf_para.ctrl_dir);

	return;
}
