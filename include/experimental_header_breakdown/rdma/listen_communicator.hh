/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef RDMA_LISTEN_COMMUNICATOR_H_
#define RDMA_LISTEN_COMMUNICATOR_H_
#include "config.h"

typedef struct nccl_net_ofi_rdma_listen_comm {
	/* This base listen communicator must be the first member of
	 * this struct. This allows casting between pointers of this
	 * struct and its base struct. */
	nccl_net_ofi_listen_comm_t base;

	/* Comm ID provided by local endpoint */
	uint32_t comm_id;

	/* Communicator created while accept routine is executed */
	nccl_net_ofi_rdma_recv_comm_t *r_comm;

	/* Reusable request for connect and connect response message */
	nccl_net_ofi_rdma_req_t req;

	/* Stage of connection establishment on listen side */
	nccl_ofi_comm_stage_t stage;

	/* Message struct send connect message and receive connect
	 * response message
	 *
	 * TODO: This should really be a list of outstanding connect
	 * messages to allow multiple connects per listen communicator.
	 */
	nccl_ofi_rdma_connection_info_t conn_msg;
} nccl_net_ofi_rdma_listen_comm_t;

#endif // End RDMA_LISTEN_COMMUNICATOR_H_