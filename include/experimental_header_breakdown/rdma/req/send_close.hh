/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef RDMA_REQ_SEND_CLOSE_H_
#define RDMA_REQ_SEND_CLOSE_H_
#include "config.h"

/*
 * @brief	Data of request responsible for sending the close message
 */
typedef struct {
	/* Pointer to the allocated control buffer from freelist */
	nccl_ofi_freelist_elem_t *ctrl_fl_elem;
	/* Schedule used to transfer the close buffer. We save the
	 * pointer to reference it when transferring the buffer over
	 * network. */
	nccl_net_ofi_schedule_t *ctrl_schedule;
} rdma_req_send_close_data_t;

#endif // End RDMA_REQ_SEND_CLOSE_H_