/* SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause */
/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 */

#ifndef __CXIDV_H__
#define __CXIDV_H__

#include <stdio.h>
#include <sys/types.h>
#include <stdbool.h>

#include <infiniband/verbs.h>
#include <rdma/ib_user_ioctl_cmds.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CXI Direct Verbs - Vendor-specific extensions */

/* Forward declarations of kernel UAPI structures */
struct cxi_ibv_alloc_ucontext_cmd {
	uint32_t comp_mask;
	uint8_t reserved_20[4];
};

struct cxi_ibv_alloc_ucontext_resp {
	uint32_t comp_mask;
	uint16_t uarn;
	uint8_t reserved_22[6];
};

struct cxi_ibv_alloc_pd_resp {
	uint32_t comp_mask;
	uint16_t pdn;
	uint8_t reserved_22[6];
};

struct cxi_ibv_create_cq_cmd {
	uint32_t comp_mask;
	uint32_t cq_depth;
	uint16_t eqn;
	uint8_t reserved_26[6];
};

struct cxi_ibv_create_cq_resp {
	uint32_t comp_mask;
	uint16_t cq_idx;
	uint16_t actual_depth;
	uint32_t db_off;
	uint8_t reserved_30[4];
};

struct cxi_ibv_create_qp_cmd {
	uint32_t comp_mask;
	uint32_t sq_depth;
	uint32_t rq_depth;
	uint16_t send_cq_idx;
	uint16_t recv_cq_idx;
	uint8_t reserved_34[4];
};

struct cxi_ibv_create_qp_resp {
	uint32_t comp_mask;
	uint32_t qp_handle;
	uint32_t qp_num;
	uint32_t sq_db_offset;
	uint32_t rq_db_offset;
	uint8_t reserved_38[4];
};

struct cxi_ibv_reg_mr_cmd {
	uint32_t comp_mask;
	uint64_t start __attribute__((aligned(8)));
	uint64_t length __attribute__((aligned(8)));
	uint64_t virt_addr __attribute__((aligned(8)));
	uint32_t access_flags;
	uint8_t reserved_44[4];
};

struct cxi_ibv_reg_mr_resp {
	uint32_t comp_mask;
	uint32_t l_key;
	uint32_t r_key;
	uint8_t reserved_30[4];
};

struct cxi_ibv_method1_resp {
	uint32_t comp_mask;
	uint32_t nic_addr;
	uint32_t pid_granule;
	uint32_t pid_count;
	uint32_t pid_bits;
	uint32_t min_free_shift;
	uint8_t reserved_48[4];
};

struct cxi_ibv_method2_resp {
	uint32_t comp_mask;
	uint32_t md_handle;
	uint64_t iova __attribute__((aligned(8)));
	uint64_t length __attribute__((aligned(8)));
	uint32_t access_flags;
	uint8_t reserved_54[4];
};

struct cxi_ibv_method3_resp {
	uint32_t comp_mask;
	uint32_t txq_handle;
	uint32_t tgq_handle;
	uint32_t cmdq_handle;
	uint32_t eq_handle;
	uint32_t state;
	uint8_t reserved_50[4];
};

/* Vendor-specific method and attribute IDs */
enum {
	CXI_IB_ATTR_METHOD1_RESP_NIC_ADDR = (1U << 29), /* UVERBS_ID_NS_SHIFT */
	CXI_IB_ATTR_METHOD1_RESP_PID_GRANULE,
	CXI_IB_ATTR_METHOD1_RESP_PID_COUNT,
	CXI_IB_ATTR_METHOD1_RESP_PID_BITS,
	CXI_IB_ATTR_METHOD1_RESP_MIN_FREE_SHIFT,

	CXI_IB_ATTR_METHOD2_MR_HANDLE,
	CXI_IB_ATTR_METHOD2_RESP_MD_HANDLE,
	CXI_IB_ATTR_METHOD2_RESP_IOVA,
	CXI_IB_ATTR_METHOD2_RESP_LENGTH,
	CXI_IB_ATTR_METHOD2_RESP_ACCESS_FLAGS,

	CXI_IB_ATTR_METHOD3_QP_HANDLE,
	CXI_IB_ATTR_METHOD3_RESP_TXQ_HANDLE,
	CXI_IB_ATTR_METHOD3_RESP_TGQ_HANDLE,
	CXI_IB_ATTR_METHOD3_RESP_CMDQ_HANDLE,
	CXI_IB_ATTR_METHOD3_RESP_EQ_HANDLE,
	CXI_IB_ATTR_METHOD3_RESP_STATE,
};

enum {
	CXI_IB_METHOD_1 = (1U << 29), /* UVERBS_ID_NS_SHIFT */
	CXI_IB_METHOD_2,
	CXI_IB_METHOD_3,
};

enum {
	CXI_IB_OBJECT_GENERIC = (1U << 29), /* UVERBS_ID_NS_SHIFT */
};

/* CXI device capabilities flags */
enum {
	CXIDV_DEVICE_CAP_ATOMIC_OPS = 1 << 0,
	CXIDV_DEVICE_CAP_RDMA_READ = 1 << 1,
	CXIDV_DEVICE_CAP_RDMA_WRITE = 1 << 2,
	CXIDV_DEVICE_CAP_MULTICAST = 1 << 3,
	CXIDV_DEVICE_CAP_TRIGGERED_OPS = 1 << 4,
	CXIDV_DEVICE_CAP_RESTRICTED_MEMBERS = 1 << 5,
};

/* CXI memory region access flags */
enum {
	CXIDV_MR_ACCESS_LOCAL_READ = 1 << 0,
	CXIDV_MR_ACCESS_LOCAL_WRITE = 1 << 1,
	CXIDV_MR_ACCESS_REMOTE_READ = 1 << 2,
	CXIDV_MR_ACCESS_REMOTE_WRITE = 1 << 3,
	CXIDV_MR_ACCESS_REMOTE_ATOMIC = 1 << 4,
};

/* CXI queue pair states */
enum {
	CXIDV_QP_STATE_RESET = 0,
	CXIDV_QP_STATE_INIT = 1,
	CXIDV_QP_STATE_RTR = 2,
	CXIDV_QP_STATE_RTS = 3,
	CXIDV_QP_STATE_SQD = 4,
	CXIDV_QP_STATE_SQE = 5,
	CXIDV_QP_STATE_ERR = 6,
};

/* CXI device attributes structure */
struct cxidv_device_attr {
	uint64_t comp_mask;
	uint32_t max_sq_wr;
	uint32_t max_rq_wr;
	uint16_t max_sq_sge;
	uint16_t max_rq_sge;
	uint32_t device_caps;
	uint32_t max_rdma_size;
	uint8_t reserved[4];
};

/* CXI Method 1 attributes structure - Device information */
struct cxidv_method1_attr {
	uint64_t comp_mask;
	uint32_t nic_addr;
	uint32_t pid_granule;
	uint32_t pid_count;
	uint32_t pid_bits;
	uint32_t min_free_shift;
	uint8_t reserved[4];
};

/* CXI Method 2 attributes structure - Memory region information */
struct cxidv_method2_attr {
	uint64_t comp_mask;
	uint32_t md_handle;
	uint64_t iova;
	uint64_t length;
	uint32_t access_flags;
	uint8_t reserved[4];
};

/* CXI Method 3 attributes structure - Queue pair information */
struct cxidv_method3_attr {
	uint64_t comp_mask;
	uint32_t txq_handle;
	uint32_t tgq_handle;
	uint32_t cmdq_handle;
	uint32_t eq_handle;
	uint32_t state;
	uint8_t reserved[4];
};

/* CXI completion queue structure */
struct cxidv_cq {
	uint64_t comp_mask;
	/* Future extension points for CXI-specific CQ functionality */
};

/**
 * cxidv_query_device - Query CXI-specific device attributes
 * @context: InfiniBand context
 * @attr: Device attributes structure to fill
 * @inlen: Size of the attributes structure
 *
 * Returns 0 on success, errno on failure
 */
int cxidv_query_device(struct ibv_context *context,
		       struct cxidv_device_attr *attr,
		       uint32_t inlen);

/**
 * cxidv_method1 - CXI Method 1 - Query device information
 * @context: InfiniBand context
 * @attr: Method 1 attributes structure to fill
 * @inlen: Size of the attributes structure
 *
 * Returns 0 on success, errno on failure
 */
int cxidv_method1(struct ibv_context *context,
		  struct cxidv_method1_attr *attr,
		  uint32_t inlen);

/**
 * cxidv_method2 - CXI Method 2 - Query memory region information
 * @mr: Memory region to query
 * @attr: Method 2 attributes structure to fill
 * @inlen: Size of the attributes structure
 *
 * Returns 0 on success, errno on failure
 */
int cxidv_method2(struct ibv_mr *mr,
		  struct cxidv_method2_attr *attr,
		  uint32_t inlen);

/**
 * cxidv_method3 - CXI Method 3 - Query queue pair information
 * @qp: Queue pair to query
 * @attr: Method 3 attributes structure to fill
 * @inlen: Size of the attributes structure
 *
 * Returns 0 on success, errno on failure
 */
int cxidv_method3(struct ibv_qp *qp,
		  struct cxidv_method3_attr *attr,
		  uint32_t inlen);

/**
 * cxidv_query_mr - Query CXI-specific memory region attributes (alias for method2)
 * @mr: Memory region to query
 * @attr: Memory region attributes structure to fill
 * @inlen: Size of the attributes structure
 *
 * Returns 0 on success, errno on failure
 */
static inline int cxidv_query_mr(struct ibv_mr *mr,
				 struct cxidv_method2_attr *attr,
				 uint32_t inlen)
{
	return cxidv_method2(mr, attr, inlen);
}

/**
 * cxidv_query_qp - Query CXI-specific queue pair attributes (alias for method3)
 * @qp: Queue pair to query
 * @attr: Queue pair attributes structure to fill
 * @inlen: Size of the attributes structure
 *
 * Returns 0 on success, errno on failure
 */
static inline int cxidv_query_qp(struct ibv_qp *qp,
				 struct cxidv_method3_attr *attr,
				 uint32_t inlen)
{
	return cxidv_method3(qp, attr, inlen);
}

/**
 * cxidv_is_supported - Check if the device supports CXI direct verbs
 * @context: InfiniBand context
 *
 * Returns 1 if supported, 0 if not supported
 */
static inline int cxidv_is_supported(struct ibv_context *context)
{
	/* Check if this is a CXI device by looking at the device name */
	return (strncmp(context->device->name, "cxi_", 4) == 0);
}

/**
 * cxidv_get_version - Get the CXI direct verbs library version
 *
 * Returns version string
 */
static inline const char *cxidv_get_version(void)
{
	return "1.0.0";
}

/* Helper macros for checking field availability */
#define CXIDV_FIELD_AVAIL(type, field, inlen) \
	(offsetof(type, field) + sizeof(((type *)0)->field) <= (inlen))

#define CXIDV_DEVICE_ATTR_FIELD_AVAIL(field, inlen) \
	CXIDV_FIELD_AVAIL(struct cxidv_device_attr, field, inlen)

#define CXIDV_METHOD1_ATTR_FIELD_AVAIL(field, inlen) \
	CXIDV_FIELD_AVAIL(struct cxidv_method1_attr, field, inlen)

#define CXIDV_METHOD2_ATTR_FIELD_AVAIL(field, inlen) \
	CXIDV_FIELD_AVAIL(struct cxidv_method2_attr, field, inlen)

#define CXIDV_METHOD3_ATTR_FIELD_AVAIL(field, inlen) \
	CXIDV_FIELD_AVAIL(struct cxidv_method3_attr, field, inlen)

#ifdef __cplusplus
}
#endif

#endif /* __CXIDV_H__ */
