/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef RDMA_MESSAGE_TYPE_H_
#define RDMA_MESSAGE_TYPE_H_
#include "config.h"

enum nccl_ofi_rdma_msg_type {
	NCCL_OFI_RDMA_MSG_CONN = 0,
	NCCL_OFI_RDMA_MSG_CONN_RESP,
	NCCL_OFI_RDMA_MSG_CTRL,
	NCCL_OFI_RDMA_MSG_EAGER,
	NCCL_OFI_RDMA_MSG_CLOSE,
	NCCL_OFI_RDMA_MSG_CTRL_NO_COMPLETION,
	NCCL_OFI_RDMA_MSG_INVALID = 15,
	NCCL_OFI_RDMA_MSG_MAX = NCCL_OFI_RDMA_MSG_INVALID,
};

static_assert(NCCL_OFI_RDMA_MSG_MAX <= (0x10),
			  "Out of space in nccl_ofi_rdma_msg_type; must fit in a nibble");

/* This goes on the wire, so we want the datatype
 * size to be fixed.
 */
typedef uint16_t nccl_ofi_rdma_msg_type_t;

#endif // End RDMA_MESSAGE_TYPE_H_