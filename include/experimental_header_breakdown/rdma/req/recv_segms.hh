/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef RDMA_REQ_RECV_SEGMS_H_
#define RDMA_REQ_RECV_SEGMS_H_
#include "config.h"

/*
 * @brief	Data of request responsible for receiving segements
 */
typedef struct {
	/* Pointer to recv parent request */
	nccl_net_ofi_rdma_req_t *recv_req;
} rdma_req_recv_segms_data_t;

#endif // End RDMA_REQ_RECV_SEGMS_H_