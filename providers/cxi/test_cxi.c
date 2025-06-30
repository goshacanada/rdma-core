// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 *
 * Simple test program for CXI provider
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <infiniband/verbs.h>
#include "cxidv.h"

static void print_device_info(struct ibv_context *context)
{
	struct ibv_device_attr device_attr;
	struct cxidv_device_attr cxi_device_attr;
	struct cxidv_method1_attr method1_attr;
	int ret;

	printf("=== CXI Device Information ===\n");

	/* Query standard device attributes */
	ret = ibv_query_device(context, &device_attr);
	if (ret) {
		fprintf(stderr, "Failed to query device: %s\n", strerror(ret));
		return;
	}

	printf("Device: %s\n", ibv_get_device_name(context->device));
	printf("FW Version: %s\n", device_attr.fw_ver);
	printf("Max QP WR: %d\n", device_attr.max_qp_wr);
	printf("Max SGE: %d\n", device_attr.max_sge);

	/* Check if CXI Direct Verbs is supported */
	if (!cxidv_is_supported(context)) {
		printf("CXI Direct Verbs: Not supported\n");
		return;
	}

	printf("CXI Direct Verbs: Supported (version %s)\n", cxidv_get_version());

	/* Query CXI device attributes */
	ret = cxidv_query_device(context, &cxi_device_attr, sizeof(cxi_device_attr));
	if (ret == 0) {
		printf("\n=== CXI Device Attributes ===\n");
		printf("Max SQ WR: %u\n", cxi_device_attr.max_sq_wr);
		printf("Max RQ WR: %u\n", cxi_device_attr.max_rq_wr);
		printf("Max SQ SGE: %u\n", cxi_device_attr.max_sq_sge);
		printf("Max RQ SGE: %u\n", cxi_device_attr.max_rq_sge);
		printf("Device Caps: 0x%x\n", cxi_device_attr.device_caps);
		printf("Max RDMA Size: %u\n", cxi_device_attr.max_rdma_size);
	} else {
		printf("Failed to query CXI device attributes: %s\n", strerror(ret));
	}

	/* Query device information using Method 1 */
	ret = cxidv_method1(context, &method1_attr, sizeof(method1_attr));
	if (ret == 0) {
		printf("\n=== CXI Method 1 Results ===\n");
		printf("NIC Address: 0x%x\n", method1_attr.nic_addr);
		printf("PID Granule: %u\n", method1_attr.pid_granule);
		printf("PID Count: %u\n", method1_attr.pid_count);
		printf("PID Bits: %u\n", method1_attr.pid_bits);
		printf("Min Free Shift: %u\n", method1_attr.min_free_shift);
	} else {
		printf("Failed to execute CXI Method 1: %s\n", strerror(ret));
	}
}

static void test_memory_region(struct ibv_context *context)
{
	struct ibv_pd *pd;
	struct ibv_mr *mr;
	struct cxidv_method2_attr method2_attr;
	void *buffer;
	size_t buffer_size = 4096;
	int ret;

	printf("\n=== CXI Memory Region Test ===\n");

	/* Allocate protection domain */
	pd = ibv_alloc_pd(context);
	if (!pd) {
		fprintf(stderr, "Failed to allocate PD\n");
		return;
	}

	/* Allocate and register memory */
	buffer = malloc(buffer_size);
	if (!buffer) {
		fprintf(stderr, "Failed to allocate buffer\n");
		goto cleanup_pd;
	}

	mr = ibv_reg_mr(pd, buffer, buffer_size, IBV_ACCESS_LOCAL_WRITE);
	if (!mr) {
		fprintf(stderr, "Failed to register MR\n");
		goto cleanup_buffer;
	}

	printf("MR registered: lkey=0x%x, rkey=0x%x\n", mr->lkey, mr->rkey);

	/* Query MR using Method 2 */
	ret = cxidv_method2(mr, &method2_attr, sizeof(method2_attr));
	if (ret == 0) {
		printf("CXI Method 2 Results:\n");
		printf("  MD Handle: 0x%x\n", method2_attr.md_handle);
		printf("  IOVA: 0x%lx\n", method2_attr.iova);
		printf("  Length: %lu\n", method2_attr.length);
		printf("  Access Flags: 0x%x\n", method2_attr.access_flags);
	} else {
		printf("Failed to execute CXI Method 2: %s\n", strerror(ret));
	}

	/* Cleanup */
	ibv_dereg_mr(mr);
cleanup_buffer:
	free(buffer);
cleanup_pd:
	ibv_dealloc_pd(pd);
}

static void test_queue_pair(struct ibv_context *context)
{
	struct ibv_pd *pd;
	struct ibv_cq *cq;
	struct ibv_qp *qp;
	struct ibv_qp_init_attr qp_init_attr;
	struct cxidv_method3_attr method3_attr;
	int ret;

	printf("\n=== CXI Queue Pair Test ===\n");

	/* Allocate protection domain */
	pd = ibv_alloc_pd(context);
	if (!pd) {
		fprintf(stderr, "Failed to allocate PD\n");
		return;
	}

	/* Create completion queue */
	cq = ibv_create_cq(context, 16, NULL, NULL, 0);
	if (!cq) {
		fprintf(stderr, "Failed to create CQ\n");
		goto cleanup_pd;
	}

	/* Create queue pair */
	memset(&qp_init_attr, 0, sizeof(qp_init_attr));
	qp_init_attr.send_cq = cq;
	qp_init_attr.recv_cq = cq;
	qp_init_attr.qp_type = IBV_QPT_RC;
	qp_init_attr.cap.max_send_wr = 16;
	qp_init_attr.cap.max_recv_wr = 16;
	qp_init_attr.cap.max_send_sge = 1;
	qp_init_attr.cap.max_recv_sge = 1;

	qp = ibv_create_qp(pd, &qp_init_attr);
	if (!qp) {
		fprintf(stderr, "Failed to create QP\n");
		goto cleanup_cq;
	}

	printf("QP created: qp_num=%u\n", qp->qp_num);

	/* Query QP using Method 3 */
	ret = cxidv_method3(qp, &method3_attr, sizeof(method3_attr));
	if (ret == 0) {
		printf("CXI Method 3 Results:\n");
		printf("  TXQ Handle: 0x%x\n", method3_attr.txq_handle);
		printf("  TGQ Handle: 0x%x\n", method3_attr.tgq_handle);
		printf("  Command Queue Handle: 0x%x\n", method3_attr.cmdq_handle);
		printf("  Event Queue Handle: 0x%x\n", method3_attr.eq_handle);
		printf("  State: %u\n", method3_attr.state);
	} else {
		printf("Failed to execute CXI Method 3: %s\n", strerror(ret));
	}

	/* Cleanup */
	ibv_destroy_qp(qp);
cleanup_cq:
	ibv_destroy_cq(cq);
cleanup_pd:
	ibv_dealloc_pd(pd);
}

int main(int argc, char *argv[])
{
	struct ibv_device **dev_list;
	struct ibv_context *context;
	int num_devices;
	int i;

	printf("CXI Provider Test Program\n");
	printf("========================\n");

	/* Get list of IB devices */
	dev_list = ibv_get_device_list(&num_devices);
	if (!dev_list) {
		fprintf(stderr, "Failed to get device list\n");
		return 1;
	}

	printf("Found %d InfiniBand devices\n", num_devices);

	/* Look for CXI devices */
	for (i = 0; i < num_devices; i++) {
		const char *dev_name = ibv_get_device_name(dev_list[i]);
		printf("Device %d: %s\n", i, dev_name);

		if (strncmp(dev_name, "cxi_", 4) == 0) {
			printf("Found CXI device: %s\n", dev_name);

			context = ibv_open_device(dev_list[i]);
			if (!context) {
				fprintf(stderr, "Failed to open device %s\n", dev_name);
				continue;
			}

			print_device_info(context);
			test_memory_region(context);
			test_queue_pair(context);

			ibv_close_device(context);
			break;
		}
	}

	if (i == num_devices) {
		printf("No CXI devices found\n");
	}

	ibv_free_device_list(dev_list);
	return 0;
}
