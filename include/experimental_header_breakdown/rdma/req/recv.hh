/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef RDMA_REQ_RECV_H_
#define RDMA_REQ_RECV_H_
#include "config.h"

/*
 * @brief	Data of request responsible for receive operation
 */
typedef struct {
	/* Destination buffer */
	void *dst_buff;
	/* Destination length */
	size_t dst_len;
	/* Mr handle for destination buffer */
	nccl_net_ofi_rdma_mr_handle_t *dest_mr_handle;
	/* Pointer to send control message child request */
	nccl_net_ofi_rdma_req_t *send_ctrl_req;
	/* Pointer to receive segments child request */
	nccl_net_ofi_rdma_req_t *recv_segms_req;
	/* (Eager messages) pointer to eager local copy request */
	nccl_net_ofi_rdma_req_t *eager_copy_req;
	/* Total number of completions. Expect one send ctrl
	 * completion and one completion that indicates that all
	 * segments have arrived.
	 *
	 * For eager messages, the second completion will be received
	 * when the local read into the destination buffer is complete */
	int total_num_compls;
#if HAVE_NVTX_TRACING
	nvtxRangeId_t trace_id;
#endif
} rdma_req_recv_data_t;

#endif // End RDMA_REQ_RECV_H_