/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef RDMA_DOMAIN_NAME_H_
#define RDMA_DOMAIN_NAME_H_
#include "config.h"

/* Metadata about dummy flush buffer */
typedef struct nccl_net_ofi_rdma_flush_buffer {
	void *host_buffer;
	size_t size;
	/* Memory registration handle of the local buffer */
	nccl_net_ofi_rdma_mr_handle_t *mr_handle;
} nccl_net_ofi_rdma_flush_buffer_t;


typedef struct nccl_net_ofi_rdma_domain_rail {
	/* Access domain handles */
	struct fid_domain *domain;

	struct fid_cq *cq;
} nccl_net_ofi_rdma_domain_rail_t;


typedef struct nccl_net_ofi_rdma_domain {
	nccl_net_ofi_domain_t base;

	int num_rails;
	nccl_net_ofi_rdma_domain_rail_t *domain_rails;

	/* The flush buffer */
	nccl_net_ofi_rdma_flush_buffer_t flush_buff;

	/* List of endpoints and set of addresses they have connections to */
	nccl_ofi_ep_addr_list_t *ep_addr_list;
} nccl_net_ofi_rdma_domain_t;

#endif // End RDMA_DOMAIN_NAME_H_