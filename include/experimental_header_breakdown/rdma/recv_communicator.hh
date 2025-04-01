/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef RDMA_RECV_COMMUNICATOR_H_
#define RDMA_RECV_COMMUNICATOR_H_
#include "config.h"

/*
 * @brief	Receive communicator rail
 *
 * Communicator rail encapsulates data of a communicator for a
 * specific rail.
 */
typedef struct nccl_net_ofi_rdma_recv_comm_rail {
	/* Fabric address of remote endpoint */
	fi_addr_t remote_addr;

	/* Pointer to libfabric endpoint of corresponding rdma
	 * endpoint rail */
	struct fid_ep *local_ep;

	/* Libfabric address of local endpoint used for flushing */
	fi_addr_t local_addr;
} nccl_net_ofi_rdma_recv_comm_rail_t;

/*
 * @brief	RDMA receive communicator
 *
 * Use function `calloc_rdma_recv_comm(int num_rails, int num_control_rails)' to
 * allocate a RDMA receive communicator with `num_rails'+`num_control_rails' rails.
 */
typedef struct nccl_net_ofi_rdma_recv_comm {
	/* This base receive communicator must be the first member of
	 * this struct. This allows casting between pointers of this
	 * struct and its base struct. */
	nccl_net_ofi_recv_comm_t base;

	uint64_t num_inflight_reqs;
	nccl_ofi_freelist_t *nccl_ofi_reqs_fl;

	/* Comm ID provided by the local endpoint */
	uint32_t local_comm_id;

	/* Comm ID provided by remote endpoint */
	uint32_t remote_comm_id;

	uint16_t next_msg_seq_num;

	nccl_ofi_msgbuff_t *msgbuff;

	/* Free list to track control buffers, for sending RDMA control messages */
	nccl_ofi_freelist_t *ctrl_buff_fl;

#if HAVE_NVTX_TRACING
	nvtxDomainHandle_t nvtx_domain[NCCL_OFI_N_NVTX_DOMAIN_PER_COMM];
#endif
	nccl_net_ofi_rdma_req_t *send_close_req;

	/* Counters for total sent and received control messages */
	pthread_mutex_t ctrl_counter_lock;
	uint64_t n_ctrl_sent;
	uint64_t n_ctrl_delivered;

	/* Number of rails */
	int num_rails;
	/* Number of control rails */
	int num_control_rails;

	bool comm_active;

	/* free list item containing a nccl_ofi_rdma_connection_info_t */
	nccl_ofi_freelist_elem_t *conn_msg;

	/* Array of `num_rails` communicator rails */
	nccl_net_ofi_rdma_recv_comm_rail_t *rails;
	/* Array of `num_control_rails` communicator rails */
	nccl_net_ofi_rdma_recv_comm_rail_t *control_rails;
} nccl_net_ofi_rdma_recv_comm_t;

#endif // End RDMA_RECV_COMMUNICATOR_H_