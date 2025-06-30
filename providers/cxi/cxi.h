/* SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause */
/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 */

#ifndef __CXI_H__
#define __CXI_H__

#include <inttypes.h>
#include <pthread.h>
#include <stddef.h>

#include <infiniband/driver.h>
#include <util/udma_barrier.h>

#include "cxi-abi.h"
#include "cxidv.h"

struct cxi_device {
	struct verbs_device vdev;
	int page_size;
};

#define CXI_GET(ptr, mask) FIELD_GET(mask##_MASK, *(ptr))

#define CXI_SET(ptr, mask, value)                                              \
	({                                                                     \
		typeof(ptr) _ptr = ptr;                                        \
		*_ptr = (*_ptr & ~(mask##_MASK)) |                             \
			FIELD_PREP(mask##_MASK, value);                        \
	})

struct cxi_context {
	struct verbs_context ibvctx;
	uint32_t cmds_supp_udata_mask;
	uint16_t uarn;
	uint32_t device_caps;
	uint32_t max_sq_wr;
	uint32_t max_rq_wr;
	uint16_t max_sq_sge;
	uint16_t max_rq_sge;
	uint32_t max_rdma_size;
	uint16_t max_wr_rdma_sge;
	struct cxi_qp **qp_table;
	unsigned int qp_table_sz_m1;
	pthread_spinlock_t qp_table_lock;
};

struct cxi_pd {
	struct ibv_pd ibvpd;
	uint16_t pdn;
};

struct cxi_cq {
	struct verbs_cq verbs_cq;
	struct cxidv_cq dv_cq;
	uint32_t cqn;
	uint16_t cq_idx;
	size_t cqe_size;
	uint8_t *buf;
	size_t buf_size;
	uint32_t *db;
	uint8_t *db_mmap_addr;
	uint16_t cc; /* Consumer Counter */
	uint8_t cmd_sn;
	pthread_spinlock_t lock;
	struct ibv_device *dev;
};

struct cxi_wq {
	uint64_t *wrid;
	uint32_t *wrid_idx_pool;
	uint32_t wqe_cnt;
	uint32_t wqe_posted;
	uint32_t wqe_completed;
	uint16_t pc; /* Producer counter */
	uint16_t desc_mask;
	uint16_t wrid_idx_pool_next;
};

struct cxi_qp {
	struct verbs_qp verbs_qp;
	struct cxi_wq sq;
	struct cxi_wq rq;
	uint32_t qp_handle;
	uint32_t qp_num;
	uint32_t sq_db_offset;
	uint32_t rq_db_offset;
	uint8_t *sq_db_mmap_addr;
	uint8_t *rq_db_mmap_addr;
	uint32_t *sq_db;
	uint32_t *rq_db;
	uint8_t *sq_buf;
	uint8_t *rq_buf;
	size_t sq_buf_size;
	size_t rq_buf_size;
	enum ibv_qp_state state;
	pthread_spinlock_t sq_lock;
	pthread_spinlock_t rq_lock;
};

struct cxi_mr {
	struct verbs_mr verbs_mr;
	uint32_t md_handle;
};

struct cxi_ah {
	struct ibv_ah ibvah;
	uint16_t ahn;
};

static inline struct cxi_context *to_cxi_context(struct ibv_context *ibvctx)
{
	return container_of(ibvctx, struct cxi_context, ibvctx.context);
}

static inline struct cxi_pd *to_cxi_pd(struct ibv_pd *ibvpd)
{
	return container_of(ibvpd, struct cxi_pd, ibvpd);
}

static inline struct cxi_cq *to_cxi_cq(struct ibv_cq *ibvcq)
{
	return container_of(ibvcq, struct cxi_cq, verbs_cq.cq);
}

static inline struct cxi_qp *to_cxi_qp(struct ibv_qp *ibvqp)
{
	return container_of(ibvqp, struct cxi_qp, verbs_qp.qp);
}

static inline struct cxi_mr *to_cxi_mr(struct ibv_mr *ibvmr)
{
	return container_of(ibvmr, struct cxi_mr, verbs_mr.ibv_mr);
}

static inline struct cxi_ah *to_cxi_ah(struct ibv_ah *ibvah)
{
	return container_of(ibvah, struct cxi_ah, ibvah);
}

#endif /* __CXI_H__ */
