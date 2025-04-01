/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef RDMA_REQ_EAGER_COPY_H_
#define RDMA_REQ_EAGER_COPY_H_
#include "config.h"

typedef struct {
	/* Pointer to rx buffer containing eager data */
	nccl_net_ofi_rdma_req_t *eager_rx_buff_req;
	/* Pointer to recv parent request */
	nccl_net_ofi_rdma_req_t *recv_req;
} rdma_req_eager_copy_data_t;

#endif // End RDMA_REQ_EAGER_COPY_H_