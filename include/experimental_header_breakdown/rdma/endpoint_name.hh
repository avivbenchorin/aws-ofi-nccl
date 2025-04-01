/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef RDMA_ENDPOINT_NAME_H_
#define RDMA_ENDPOINT_NAME_H_
#include "config.h"

/*
 * Rdma endpoint name
 *
 * Length of the name is limited to `MAX_EP_ADDR`.
 */
typedef struct nccl_ofi_rdma_ep_name {
	char ep_name[MAX_EP_ADDR];
	size_t ep_name_len;
} nccl_ofi_rdma_ep_name_t;

#endif // End RDMA_ENDPOINT_NAME_H_