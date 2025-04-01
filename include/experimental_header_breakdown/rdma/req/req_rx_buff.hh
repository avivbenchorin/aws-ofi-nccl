/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef RDMA_REQ_RX_BUFF_H_
#define RDMA_REQ_RX_BUFF_H_
#include "config.h"

typedef struct {
	/* Rx buffer freelist item */
	nccl_ofi_freelist_elem_t *rx_buff_fl_elem;
	/* Length of rx buffer */
	size_t buff_len;
	/* Length of received data */
	size_t recv_len;

	/*
	 * Keeps tracks of Rail ID which is used to post the rx buffer.
	 * This is useful for re-posting the buffer on the same rail
	 * when it gets completed.
	 */
	nccl_net_ofi_ep_rail_t *rail;
	/*
	 * Back-pointer to associated endpoint
	 */
	nccl_net_ofi_rdma_ep_t *ep;
} rdma_req_rx_buff_data_t;

#endif // End RDMA_REQ_RX_BUFF_H_