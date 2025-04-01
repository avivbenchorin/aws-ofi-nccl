/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef RDMA_CLOSE_MSG_H_
#define RDMA_CLOSE_MSG_H_
#include "config.h"

/* Message from receiver to sender indicating sender can close resources */
typedef struct nccl_net_ofi_rdma_close_msg {
	/* Message type, must be NCCL_OFI_RDMA_MSG_CLOSE */
	uint16_t type:NCCL_OFI_RDMA_CTRL_TYPE_BITS;

	/* Count of number of ctrl messages sent by the r_comm */
	uint64_t ctrl_counter;

	/* Comm ID provided by the sender */
	uint32_t send_comm_id;
} nccl_net_ofi_rdma_close_msg_t;

#endif // End RDMA_CLOSE_MSG_H_