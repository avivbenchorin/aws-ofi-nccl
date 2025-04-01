/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef RDMA_REQ_FLUSH_H_
#define RDMA_REQ_FLUSH_H_
#include "config.h"

/*
 * @brief	Data of request responsible for flush operatoin
 */
typedef struct {
	/* Buffer to read flush data from */
	void *data;
	/* MR handles for the data buffer */
	nccl_net_ofi_rdma_mr_handle_t *mr_handle;
	/* Total number of completions. Expect completions from all NIC rail */
	int total_num_compls;
} rdma_req_flush_data_t;

#endif // End RDMA_REQ_FLUSH_H_