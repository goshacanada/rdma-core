// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <util/util.h>

#include "cxi.h"
#include "verbs.h"

static void cxi_free_context(struct ibv_context *ibvctx);

#define PCI_VENDOR_ID_HPE 0x1590
#define PCI_VENDOR_ID_CRAY 0x17db

static const struct verbs_match_ent cxi_table[] = {
	VERBS_DRIVER_ID(RDMA_DRIVER_UNKNOWN), /* Will need proper driver ID */
	VERBS_PCI_MATCH(PCI_VENDOR_ID_HPE, 0x0371, NULL),   /* Cassini 2 */
	VERBS_PCI_MATCH(PCI_VENDOR_ID_CRAY, 0x0501, NULL),  /* Cassini 1 */
	{}
};

static const struct verbs_context_ops cxi_ctx_ops = {
	.alloc_pd = cxi_alloc_pd,
	.create_ah = cxi_create_ah,
	.create_cq = cxi_create_cq,
	.create_cq_ex = cxi_create_cq_ex,
	.create_qp = cxi_create_qp,
	.create_qp_ex = cxi_create_qp_ex,
	.cq_event = cxi_cq_event,
	.dealloc_pd = cxi_dealloc_pd,
	.dereg_mr = cxi_dereg_mr,
	.destroy_ah = cxi_destroy_ah,
	.destroy_cq = cxi_destroy_cq,
	.destroy_qp = cxi_destroy_qp,
	.modify_qp = cxi_modify_qp,
	.poll_cq = cxi_poll_cq,
	.post_recv = cxi_post_recv,
	.post_send = cxi_post_send,
	.query_device_ex = cxi_query_device_ex,
	.query_port = cxi_query_port,
	.query_qp = cxi_query_qp,
	.reg_mr = cxi_reg_mr,
	.req_notify_cq = cxi_arm_cq,
	.free_context = cxi_free_context,
};

static struct verbs_context *cxi_alloc_context(struct ibv_device *vdev,
					       int cmd_fd,
					       void *private_data)
{
	struct cxi_ibv_alloc_ucontext_resp resp = {};
	struct cxi_ibv_alloc_ucontext_cmd cmd = {};
	struct cxi_context *ctx;

	ctx = verbs_init_and_alloc_context(vdev, cmd_fd, ctx, ibvctx,
					   RDMA_DRIVER_UNKNOWN);
	if (!ctx)
		return NULL;

	if (ibv_cmd_get_context(&ctx->ibvctx, &cmd.ibv_cmd, sizeof(cmd),
				NULL, &resp.ibv_resp, sizeof(resp))) {
		verbs_err(&ctx->ibvctx, "ibv_cmd_get_context failed\n");
		goto err_free_ctx;
	}

	ctx->uarn = resp.uarn;
	ctx->cmds_supp_udata_mask = 0; /* Initialize as needed */
	pthread_spin_init(&ctx->qp_table_lock, PTHREAD_PROCESS_PRIVATE);

	verbs_set_ops(&ctx->ibvctx, &cxi_ctx_ops);

	if (cxi_query_device_ctx(ctx))
		goto err_free_spinlock;

	return &ctx->ibvctx;

err_free_spinlock:
	pthread_spin_destroy(&ctx->qp_table_lock);
err_free_ctx:
	verbs_uninit_context(&ctx->ibvctx);
	free(ctx);
	return NULL;
}

static void cxi_free_context(struct ibv_context *ibvctx)
{
	struct cxi_context *ctx = to_cxi_context(ibvctx);

	pthread_spin_destroy(&ctx->qp_table_lock);
	free(ctx->qp_table);
	verbs_uninit_context(&ctx->ibvctx);
	free(ctx);
}

static void cxi_uninit_device(struct verbs_device *verbs_dev)
{
	struct cxi_device *dev = container_of(verbs_dev, struct cxi_device, vdev);

	free(dev);
}

static struct verbs_device *cxi_device_alloc(struct verbs_sysfs_dev *sysfs_dev)
{
	struct cxi_device *dev;

	dev = calloc(1, sizeof(*dev));
	if (!dev)
		return NULL;

	dev->page_size = sysconf(_SC_PAGESIZE);

	return &dev->vdev;
}

static const struct verbs_device_ops cxi_dev_ops = {
	.name = "cxi",
	.match_min_abi_version = 0,
	.match_max_abi_version = INT_MAX,
	.match_table = cxi_table,
	.alloc_device = cxi_device_alloc,
	.uninit_device = cxi_uninit_device,
	.alloc_context = cxi_alloc_context,
};

PROVIDER_DRIVER(cxi, cxi_dev_ops);
