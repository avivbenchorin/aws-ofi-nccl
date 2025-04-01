/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef RDMA_REQ_SEND_H_
#define RDMA_REQ_SEND_H_
#include "config.h"

typedef struct {
	/* True for eager messages */
	bool eager;
	/* Remote destination buffer address */
	uint64_t remote_buff;
	/* Remote buffer length */
	uint64_t remote_len;
	/* Remote MR key */
	uint64_t remote_mr_key[MAX_NUM_RAILS];
	/* Write immediate data */
	uint64_t wdata;
	/* Number of rails where we have successfully posted the network xfer.
	 * Used mostly when the network xfer is sliced across multiple rails */
	uint64_t xferred_rail_id;
	/* Application-provided local src/dst buffer */
	void *buff;
	/* Length of application-provided buffer */
	size_t buff_len;
	/* Memory region descriptors associated to `buff' */
	nccl_net_ofi_rdma_mr_handle_t *buff_mr_handle;
	/* Schedule used to transfer this request. We save the pointer to
	 * reference it when transferring the request over network. */
	nccl_net_ofi_schedule_t *schedule;
	/* Total number of completions. Expect one completion for receiving the
	 * control message and one completion for each send segment. */
	int total_num_compls;
	/* 
	 * Flag to indicate target side early completion, so that sender side
	 * uses the corresponding RMA write operation.
	 * True to use fi_write instead of fi_writedata in send() 
	 */
	bool no_target_completion;
#if HAVE_NVTX_TRACING
	nvtxRangeId_t trace_id;
	nvtxRangeId_t seg_trace_id[MAX_NUM_RAILS];
#endif
} rdma_req_send_data_t;

#endif // End RDMA_REQ_SEND_H_