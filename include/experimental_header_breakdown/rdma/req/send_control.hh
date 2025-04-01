/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef RDMA_REQ_SEND_CONTROL_H_
#define RDMA_REQ_SEND_CONTROL_H_
#include "config.h"

/*
 * @brief	Data of request responsible for sending the control message
 */
typedef struct {
	/* Pointer to the allocated control buffer from freelist */
	nccl_ofi_freelist_elem_t *ctrl_fl_elem;
	/* Schedule used to transfer the control buffer. We save the
	 * pointer to reference it when transferring the buffer over
	 * network. */
	nccl_net_ofi_schedule_t *ctrl_schedule;
	/* Pointer to recv parent request */
	nccl_net_ofi_rdma_req_t *recv_req;
#if HAVE_NVTX_TRACING
	nvtxRangeId_t trace_id;
#endif
} rdma_req_send_ctrl_data_t;

#endif // End RDMA_REQ_SEND_CONTROL_H_