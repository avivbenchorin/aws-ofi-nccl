/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef RDMA_REQUEST_H_
#define RDMA_REQUEST_H_
#include "config.h"

typedef enum nccl_net_ofi_rdma_req_state {
	NCCL_OFI_RDMA_REQ_CREATED = 0,
	NCCL_OFI_RDMA_REQ_PENDING,
	NCCL_OFI_RDMA_REQ_COMPLETED,
	NCCL_OFI_RDMA_REQ_ERROR,
	NCCL_OFI_RDMA_REQ_INVALID_STATE,
} nccl_net_ofi_rdma_req_state_t;

typedef enum nccl_net_ofi_rdma_req_type {
	/* Write request */
	NCCL_OFI_RDMA_WRITE,
	/* Read request */
	NCCL_OFI_RDMA_READ,
	/* Send request */
	NCCL_OFI_RDMA_SEND,
	/* Receive request */
	NCCL_OFI_RDMA_RECV,
	/* Send control request. Subrequest of NCCL_OFI_RDMA_RECV */
	NCCL_OFI_RDMA_SEND_CTRL,
	/* Send close request. */
	NCCL_OFI_RDMA_SEND_CLOSE,
	/* Receive segments request. Subrequest of NCCL_OFI_RDMA_RECV */
	NCCL_OFI_RDMA_RECV_SEGMS,
	/* Eager local copy request. Subrequest of NCCL_OFI_RDMA_RECV */
	NCCL_OFI_RDMA_EAGER_COPY,
	/* Ctrl rx buff post request */
	NCCL_OFI_RDMA_CTRL_RX_BUFF,
	/* Eager rx buff post request */
	NCCL_OFI_RDMA_EAGER_RX_BUFF,
	/* Flush request */
	NCCL_OFI_RDMA_FLUSH,
	/* Connect message send request */
	NCCL_OFI_RDMA_SEND_CONN,
	/* Connect message receive request */
	NCCL_OFI_RDMA_RECV_CONN,
	/* Connect response message receive request */
	NCCL_OFI_RDMA_RECV_CONN_RESP,
	/* Connect response message send request */
	NCCL_OFI_RDMA_SEND_CONN_RESP,
	/* Invalid type */
	NCCL_OFI_RDMA_INVALID_TYPE,
} nccl_net_ofi_rdma_req_type_t;

/*
 * @brief	RDMA request
 */
typedef struct nccl_net_ofi_rdma_req {
	nccl_net_ofi_req_t base;

	struct fi_context2 ctx[MAX_NUM_RAILS];

	/* Associated Comm object */
	nccl_net_ofi_comm_t *comm;

	/* Associated Device ID */
	int dev_id;

	/* Message sequence number */
	uint16_t msg_seq_num;

	/* Number of arrived request completions */
	int ncompls;

	union {
		rdma_req_rma_op_data_t rma_op_data;
		rdma_req_send_data_t send_data;
		rdma_req_recv_data_t recv_data;
		rdma_req_send_ctrl_data_t send_ctrl_data;
		rdma_req_send_close_data_t send_close_data;
		rdma_req_eager_copy_data_t eager_copy_data;
		rdma_req_recv_segms_data_t recv_segms_data;
		rdma_req_flush_data_t flush_data;
		rdma_req_rx_buff_data_t rx_buff_data;
	};

	/* Size of completed request */
	size_t size;

	/*
	 * Protect updating critical fields such as size and ncompls when
	 * network xfer happened over multiple rails
	 */
	pthread_mutex_t req_lock;

	/* State of request */
	nccl_net_ofi_rdma_req_state_t state;

	/* Type of request */
	nccl_net_ofi_rdma_req_type_t type;

	/* Backpointer to freelist element */
	nccl_ofi_freelist_elem_t *elem;

	/* Deinitialzie and free request. This function returns error
	 * in cases where cleanup fails. This function may also return
	 * error if the owner of the request has to deallocate the
	 * request by its own. */
	int (*free)(nccl_net_ofi_rdma_req_t *req,
		    bool dec_inflight_reqs);

} nccl_net_ofi_rdma_req_t;

#endif // End RDMA_CONTROL_H_