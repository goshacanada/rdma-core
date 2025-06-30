// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 */

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <ccan/minmax.h>

#include <util/compiler.h>
#include <util/mmio.h>
#include <util/util.h>

#include "cxi.h"
#include "cxidv.h"
#include "verbs.h"

#define CXI_DEV_CAP(ctx, cap) \
	((ctx)->device_caps & CXIDV_DEVICE_CAP_##cap)

static bool is_buf_cleared(void *buf, size_t len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (((uint8_t *)buf)[i])
			return false;
	}

	return true;
}

#define is_ext_cleared(ptr, inlen) \
	is_buf_cleared((uint8_t *)ptr + sizeof(*ptr), inlen - sizeof(*ptr))

#define is_reserved_cleared(reserved) is_buf_cleared(reserved, sizeof(reserved))

int cxi_query_port(struct ibv_context *ibvctx, uint8_t port,
		   struct ibv_port_attr *port_attr)
{
	struct ibv_query_port cmd;

	return ibv_cmd_query_port(ibvctx, port, port_attr, &cmd, sizeof(cmd));
}

int cxi_query_device_ex(struct ibv_context *context,
			const struct ibv_query_device_ex_input *input,
			struct ibv_device_attr_ex *attr,
			size_t attr_size)
{
	struct cxi_context *ctx = to_cxi_context(context);
	struct ibv_device_attr *a = &attr->orig_attr;
	int err;

	err = ibv_cmd_query_device_any(context, input, attr, attr_size,
				       NULL, NULL);
	if (err) {
		verbs_err(verbs_get_ctx(context), "ibv_cmd_query_device_any failed\n");
		return err;
	}

	/* Adjust device attributes based on CXI capabilities */
	a->max_qp_wr = min_t(int, a->max_qp_wr, ctx->max_sq_wr);
	a->max_sge = min_t(int, a->max_sge, ctx->max_sq_sge);

	return 0;
}

int cxi_query_device_ctx(struct cxi_context *ctx)
{
	struct cxidv_device_attr attr = {};
	int err;

	err = cxidv_query_device(&ctx->ibvctx.context, &attr, sizeof(attr));
	if (err) {
		verbs_err(&ctx->ibvctx, "cxidv_query_device failed\n");
		return err;
	}

	ctx->device_caps = attr.device_caps;
	ctx->max_sq_wr = attr.max_sq_wr;
	ctx->max_rq_wr = attr.max_rq_wr;
	ctx->max_sq_sge = attr.max_sq_sge;
	ctx->max_rq_sge = attr.max_rq_sge;
	ctx->max_rdma_size = attr.max_rdma_size;

	return 0;
}

struct ibv_pd *cxi_alloc_pd(struct ibv_context *context)
{
	struct cxi_alloc_pd_resp resp = {};
	struct cxi_pd *pd;

	pd = calloc(1, sizeof(*pd));
	if (!pd)
		return NULL;

	if (ibv_cmd_alloc_pd(context, &pd->ibvpd, NULL, 0,
			     &resp.ibv_resp, sizeof(resp))) {
		free(pd);
		return NULL;
	}

	pd->pdn = resp.pdn;

	return &pd->ibvpd;
}

int cxi_dealloc_pd(struct ibv_pd *pd)
{
	int ret;

	ret = ibv_cmd_dealloc_pd(pd);
	if (ret)
		return ret;

	free(to_cxi_pd(pd));
	return 0;
}

struct ibv_mr *cxi_reg_mr(struct ibv_pd *pd, void *addr, size_t length,
			  int access)
{
	struct cxi_reg_mr_resp resp = {};
	struct cxi_reg_mr_cmd cmd = {};
	struct cxi_mr *mr;
	int ret;

	mr = calloc(1, sizeof(*mr));
	if (!mr)
		return NULL;

	cmd.start = (uintptr_t)addr;
	cmd.length = length;
	cmd.virt_addr = (uintptr_t)addr;
	cmd.access_flags = access;

	ret = ibv_cmd_reg_mr(pd, addr, length, (uintptr_t)addr, access,
			     &mr->verbs_mr, &cmd.ibv_cmd, sizeof(cmd),
			     &resp.ibv_resp, sizeof(resp));
	if (ret) {
		free(mr);
		return NULL;
	}

	mr->md_handle = resp.l_key; /* Store MD handle for vendor queries */

	return &mr->verbs_mr.ibv_mr;
}

int cxi_dereg_mr(struct verbs_mr *vmr)
{
	int ret;

	ret = ibv_cmd_dereg_mr(vmr);
	if (ret)
		return ret;

	free(to_cxi_mr(&vmr->ibv_mr));
	return 0;
}

struct ibv_cq *cxi_create_cq(struct ibv_context *context, int cqe,
			     struct ibv_comp_channel *channel,
			     int comp_vector)
{
	struct cxi_create_cq_resp resp = {};
	struct cxi_create_cq_cmd cmd = {};
	struct cxi_cq *cq;
	int ret;

	cq = calloc(1, sizeof(*cq));
	if (!cq)
		return NULL;

	cmd.cq_depth = cqe;
	cmd.eqn = comp_vector;

	ret = ibv_cmd_create_cq(context, cqe, channel, comp_vector,
				&cq->verbs_cq.cq, &cmd.ibv_cmd, sizeof(cmd),
				&resp.ibv_resp, sizeof(resp));
	if (ret) {
		free(cq);
		return NULL;
	}

	cq->cq_idx = resp.cq_idx;
	cq->cqn = resp.cq_idx; /* Use same value for compatibility */
	cq->cqe_size = 64; /* Default CQE size */
	pthread_spin_init(&cq->lock, PTHREAD_PROCESS_PRIVATE);

	return &cq->verbs_cq.cq;
}

struct ibv_cq_ex *cxi_create_cq_ex(struct ibv_context *context,
				   struct ibv_cq_init_attr_ex *cq_attr)
{
	struct cxi_create_cq_resp resp = {};
	struct cxi_create_cq_cmd cmd = {};
	struct cxi_cq *cq;
	int ret;

	cq = calloc(1, sizeof(*cq));
	if (!cq)
		return NULL;

	cmd.cq_depth = cq_attr->cqe;
	cmd.eqn = cq_attr->comp_vector;

	ret = ibv_cmd_create_cq_ex(context, cq_attr, &cq->verbs_cq,
				   &cmd.ibv_cmd, sizeof(cmd),
				   &resp.ibv_resp, sizeof(resp));
	if (ret) {
		free(cq);
		return NULL;
	}

	cq->cq_idx = resp.cq_idx;
	cq->cqn = resp.cq_idx;
	cq->cqe_size = 64;
	pthread_spin_init(&cq->lock, PTHREAD_PROCESS_PRIVATE);

	return &cq->verbs_cq.cq_ex;
}

int cxi_destroy_cq(struct ibv_cq *cq)
{
	struct cxi_cq *cxi_cq = to_cxi_cq(cq);
	int ret;

	ret = ibv_cmd_destroy_cq(cq);
	if (ret)
		return ret;

	pthread_spin_destroy(&cxi_cq->lock);
	free(cxi_cq);
	return 0;
}

int cxi_poll_cq(struct ibv_cq *cq, int ne, struct ibv_wc *wc)
{
	/* Basic polling implementation - would need actual CXI CQE processing */
	return 0; /* No completions for now */
}

int cxi_arm_cq(struct ibv_cq *cq, int solicited)
{
	return ibv_cmd_req_notify_cq(cq, solicited);
}

void cxi_cq_event(struct ibv_cq *cq)
{
	/* Handle CQ events */
}

struct ibv_qp *cxi_create_qp(struct ibv_pd *pd,
			     struct ibv_qp_init_attr *attr)
{
	struct cxi_create_qp_resp resp = {};
	struct cxi_create_qp_cmd cmd = {};
	struct cxi_qp *qp;
	int ret;

	qp = calloc(1, sizeof(*qp));
	if (!qp)
		return NULL;

	cmd.sq_depth = attr->cap.max_send_wr;
	cmd.rq_depth = attr->cap.max_recv_wr;
	cmd.send_cq_idx = attr->send_cq ? to_cxi_cq(attr->send_cq)->cq_idx : 0;
	cmd.recv_cq_idx = attr->recv_cq ? to_cxi_cq(attr->recv_cq)->cq_idx : 0;

	ret = ibv_cmd_create_qp(pd, &qp->verbs_qp.qp, attr,
				&cmd.ibv_cmd, sizeof(cmd),
				&resp.ibv_resp, sizeof(resp));
	if (ret) {
		free(qp);
		return NULL;
	}

	qp->qp_handle = resp.qp_handle;
	qp->qp_num = resp.qp_num;
	qp->sq_db_offset = resp.sq_db_offset;
	qp->rq_db_offset = resp.rq_db_offset;
	qp->state = IBV_QPS_RESET;

	pthread_spin_init(&qp->sq_lock, PTHREAD_PROCESS_PRIVATE);
	pthread_spin_init(&qp->rq_lock, PTHREAD_PROCESS_PRIVATE);

	return &qp->verbs_qp.qp;
}

struct ibv_qp *cxi_create_qp_ex(struct ibv_context *context,
				struct ibv_qp_init_attr_ex *qp_init_attr_ex)
{
	struct cxi_create_qp_resp resp = {};
	struct cxi_create_qp_cmd cmd = {};
	struct cxi_qp *qp;
	int ret;

	qp = calloc(1, sizeof(*qp));
	if (!qp)
		return NULL;

	cmd.sq_depth = qp_init_attr_ex->cap.max_send_wr;
	cmd.rq_depth = qp_init_attr_ex->cap.max_recv_wr;
	cmd.send_cq_idx = qp_init_attr_ex->send_cq ?
			  to_cxi_cq(qp_init_attr_ex->send_cq)->cq_idx : 0;
	cmd.recv_cq_idx = qp_init_attr_ex->recv_cq ?
			  to_cxi_cq(qp_init_attr_ex->recv_cq)->cq_idx : 0;

	ret = ibv_cmd_create_qp_ex(context, &qp->verbs_qp, qp_init_attr_ex,
				   &cmd.ibv_cmd, sizeof(cmd),
				   &resp.ibv_resp, sizeof(resp));
	if (ret) {
		free(qp);
		return NULL;
	}

	qp->qp_handle = resp.qp_handle;
	qp->qp_num = resp.qp_num;
	qp->sq_db_offset = resp.sq_db_offset;
	qp->rq_db_offset = resp.rq_db_offset;
	qp->state = IBV_QPS_RESET;

	pthread_spin_init(&qp->sq_lock, PTHREAD_PROCESS_PRIVATE);
	pthread_spin_init(&qp->rq_lock, PTHREAD_PROCESS_PRIVATE);

	return &qp->verbs_qp.qp;
}

int cxi_destroy_qp(struct ibv_qp *qp)
{
	struct cxi_qp *cxi_qp = to_cxi_qp(qp);
	int ret;

	ret = ibv_cmd_destroy_qp(qp);
	if (ret)
		return ret;

	pthread_spin_destroy(&cxi_qp->sq_lock);
	pthread_spin_destroy(&cxi_qp->rq_lock);
	free(cxi_qp);
	return 0;
}

int cxi_modify_qp(struct ibv_qp *qp, struct ibv_qp_attr *attr,
		  int attr_mask)
{
	struct cxi_qp *cxi_qp = to_cxi_qp(qp);
	int ret;

	ret = ibv_cmd_modify_qp(qp, attr, attr_mask, NULL, 0, NULL, 0);
	if (ret)
		return ret;

	if (attr_mask & IBV_QP_STATE)
		cxi_qp->state = attr->qp_state;

	return 0;
}

int cxi_query_qp(struct ibv_qp *qp, struct ibv_qp_attr *attr,
		 int attr_mask, struct ibv_qp_init_attr *init_attr)
{
	return ibv_cmd_query_qp(qp, attr, attr_mask, init_attr, NULL, 0, NULL, 0);
}

struct ibv_ah *cxi_create_ah(struct ibv_pd *pd, struct ibv_ah_attr *attr)
{
	struct cxi_ah *ah;

	ah = calloc(1, sizeof(*ah));
	if (!ah)
		return NULL;

	if (ibv_cmd_create_ah(pd, &ah->ibvah, attr, NULL, 0, NULL, 0)) {
		free(ah);
		return NULL;
	}

	ah->ahn = 0; /* Would be set from response */

	return &ah->ibvah;
}

int cxi_destroy_ah(struct ibv_ah *ah)
{
	int ret;

	ret = ibv_cmd_destroy_ah(ah, NULL, 0, NULL, 0);
	if (ret)
		return ret;

	free(to_cxi_ah(ah));
	return 0;
}

int cxi_post_send(struct ibv_qp *ibvqp, struct ibv_send_wr *wr,
		  struct ibv_send_wr **bad_wr)
{
	/* Basic implementation - would need actual CXI work request processing */
	*bad_wr = wr;
	return ENOSYS;
}

int cxi_post_recv(struct ibv_qp *ibvqp, struct ibv_recv_wr *wr,
		  struct ibv_recv_wr **bad_wr)
{
	/* Basic implementation - would need actual CXI work request processing */
	*bad_wr = wr;
	return ENOSYS;
}

/* CXI Direct Verbs vendor-specific method implementations */

int cxidv_query_device(struct ibv_context *context,
		       struct cxidv_device_attr *attr,
		       uint32_t inlen)
{
	struct cxi_context *ctx = to_cxi_context(context);

	if (!attr || inlen < sizeof(*attr))
		return EINVAL;

	memset(attr, 0, inlen);
	attr->comp_mask = 0;
	attr->max_sq_wr = ctx->max_sq_wr;
	attr->max_rq_wr = ctx->max_rq_wr;
	attr->max_sq_sge = ctx->max_sq_sge;
	attr->max_rq_sge = ctx->max_rq_sge;
	attr->device_caps = ctx->device_caps;
	attr->max_rdma_size = ctx->max_rdma_size;

	return 0;
}

int cxidv_method1(struct ibv_context *context,
		  struct cxidv_method1_attr *attr,
		  uint32_t inlen)
{
	DECLARE_COMMAND_BUFFER(cmd, CXI_IB_OBJECT_GENERIC, CXI_IB_METHOD_1, 5);
	struct cxi_method1_resp resp = {};
	int ret;

	if (!attr || inlen < sizeof(*attr))
		return EINVAL;

	fill_attr_out_ptr(cmd, CXI_IB_ATTR_METHOD1_RESP_NIC_ADDR, &resp.nic_addr);
	fill_attr_out_ptr(cmd, CXI_IB_ATTR_METHOD1_RESP_PID_GRANULE, &resp.pid_granule);
	fill_attr_out_ptr(cmd, CXI_IB_ATTR_METHOD1_RESP_PID_COUNT, &resp.pid_count);
	fill_attr_out_ptr(cmd, CXI_IB_ATTR_METHOD1_RESP_PID_BITS, &resp.pid_bits);
	fill_attr_out_ptr(cmd, CXI_IB_ATTR_METHOD1_RESP_MIN_FREE_SHIFT, &resp.min_free_shift);

	ret = execute_ioctl(context, cmd);
	if (ret)
		return ret;

	memset(attr, 0, inlen);
	attr->comp_mask = resp.comp_mask;
	attr->nic_addr = resp.nic_addr;
	attr->pid_granule = resp.pid_granule;
	attr->pid_count = resp.pid_count;
	attr->pid_bits = resp.pid_bits;
	attr->min_free_shift = resp.min_free_shift;

	return 0;
}

int cxidv_method2(struct ibv_mr *mr,
		  struct cxidv_method2_attr *attr,
		  uint32_t inlen)
{
	DECLARE_COMMAND_BUFFER(cmd, CXI_IB_OBJECT_GENERIC, CXI_IB_METHOD_2, 5);
	struct cxi_method2_resp resp = {};
	int ret;

	if (!attr || inlen < sizeof(*attr))
		return EINVAL;

	fill_attr_in_obj(cmd, CXI_IB_ATTR_METHOD2_MR_HANDLE, mr->handle);
	fill_attr_out_ptr(cmd, CXI_IB_ATTR_METHOD2_RESP_MD_HANDLE, &resp.md_handle);
	fill_attr_out_ptr(cmd, CXI_IB_ATTR_METHOD2_RESP_IOVA, &resp.iova);
	fill_attr_out_ptr(cmd, CXI_IB_ATTR_METHOD2_RESP_LENGTH, &resp.length);
	fill_attr_out_ptr(cmd, CXI_IB_ATTR_METHOD2_RESP_ACCESS_FLAGS, &resp.access_flags);

	ret = execute_ioctl(mr->context, cmd);
	if (ret)
		return ret;

	memset(attr, 0, inlen);
	attr->comp_mask = resp.comp_mask;
	attr->md_handle = resp.md_handle;
	attr->iova = resp.iova;
	attr->length = resp.length;
	attr->access_flags = resp.access_flags;

	return 0;
}

int cxidv_method3(struct ibv_qp *qp,
		  struct cxidv_method3_attr *attr,
		  uint32_t inlen)
{
	DECLARE_COMMAND_BUFFER(cmd, CXI_IB_OBJECT_GENERIC, CXI_IB_METHOD_3, 6);
	struct cxi_method3_resp resp = {};
	int ret;

	if (!attr || inlen < sizeof(*attr))
		return EINVAL;

	fill_attr_in_obj(cmd, CXI_IB_ATTR_METHOD3_QP_HANDLE, qp->handle);
	fill_attr_out_ptr(cmd, CXI_IB_ATTR_METHOD3_RESP_TXQ_HANDLE, &resp.txq_handle);
	fill_attr_out_ptr(cmd, CXI_IB_ATTR_METHOD3_RESP_TGQ_HANDLE, &resp.tgq_handle);
	fill_attr_out_ptr(cmd, CXI_IB_ATTR_METHOD3_RESP_CMDQ_HANDLE, &resp.cmdq_handle);
	fill_attr_out_ptr(cmd, CXI_IB_ATTR_METHOD3_RESP_EQ_HANDLE, &resp.eq_handle);
	fill_attr_out_ptr(cmd, CXI_IB_ATTR_METHOD3_RESP_STATE, &resp.state);

	ret = execute_ioctl(qp->context, cmd);
	if (ret)
		return ret;

	memset(attr, 0, inlen);
	attr->comp_mask = resp.comp_mask;
	attr->txq_handle = resp.txq_handle;
	attr->tgq_handle = resp.tgq_handle;
	attr->cmdq_handle = resp.cmdq_handle;
	attr->eq_handle = resp.eq_handle;
	attr->state = resp.state;

	return 0;
}
