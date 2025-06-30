/* SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause */
/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 */

#ifndef __CXI_VERBS_H__
#define __CXI_VERBS_H__

#include <infiniband/verbs.h>

/* Function prototypes for CXI verbs operations */

/* Context operations */
int cxi_query_device_ex(struct ibv_context *context,
			const struct ibv_query_device_ex_input *input,
			struct ibv_device_attr_ex *attr,
			size_t attr_size);
int cxi_query_port(struct ibv_context *ibvctx, uint8_t port,
		   struct ibv_port_attr *port_attr);

/* Protection domain operations */
struct ibv_pd *cxi_alloc_pd(struct ibv_context *context);
int cxi_dealloc_pd(struct ibv_pd *pd);

/* Memory region operations */
struct ibv_mr *cxi_reg_mr(struct ibv_pd *pd, void *addr, size_t length,
			  int access);
int cxi_dereg_mr(struct verbs_mr *vmr);

/* Completion queue operations */
struct ibv_cq *cxi_create_cq(struct ibv_context *context, int cqe,
			     struct ibv_comp_channel *channel,
			     int comp_vector);
struct ibv_cq_ex *cxi_create_cq_ex(struct ibv_context *context,
				   struct ibv_cq_init_attr_ex *cq_attr);
int cxi_destroy_cq(struct ibv_cq *cq);
int cxi_poll_cq(struct ibv_cq *cq, int ne, struct ibv_wc *wc);
int cxi_arm_cq(struct ibv_cq *cq, int solicited);
void cxi_cq_event(struct ibv_cq *cq);

/* Queue pair operations */
struct ibv_qp *cxi_create_qp(struct ibv_pd *pd,
			     struct ibv_qp_init_attr *attr);
struct ibv_qp *cxi_create_qp_ex(struct ibv_context *context,
				struct ibv_qp_init_attr_ex *qp_init_attr_ex);
int cxi_destroy_qp(struct ibv_qp *qp);
int cxi_modify_qp(struct ibv_qp *qp, struct ibv_qp_attr *attr,
		  int attr_mask);
int cxi_query_qp(struct ibv_qp *qp, struct ibv_qp_attr *attr,
		 int attr_mask, struct ibv_qp_init_attr *init_attr);

/* Address handle operations */
struct ibv_ah *cxi_create_ah(struct ibv_pd *pd, struct ibv_ah_attr *attr);
int cxi_destroy_ah(struct ibv_ah *ah);

/* Work request operations */
int cxi_post_send(struct ibv_qp *ibvqp, struct ibv_send_wr *wr,
		  struct ibv_send_wr **bad_wr);
int cxi_post_recv(struct ibv_qp *ibvqp, struct ibv_recv_wr *wr,
		  struct ibv_recv_wr **bad_wr);

/* CXI-specific device query */
int cxi_query_device_ctx(struct cxi_context *ctx);

#endif /* __CXI_VERBS_H__ */
