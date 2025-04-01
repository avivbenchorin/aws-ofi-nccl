/*
 * Copyright (c) 2023-2024 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef NCCL_OFI_RDMA_H_
#define NCCL_OFI_RDMA_H_
#include "config.h"

#include <rdma/fabric.h>

#include <deque>

#include "nccl_ofi.h"
#include "nccl_ofi_ep_addr_list.h"
#include "nccl_ofi_freelist.h"
#include "nccl_ofi_idpool.h"
#include "nccl_ofi_log.h"
#include "nccl_ofi_msgbuff.h"
#include "nccl_ofi_scheduler.h"
#include "nccl_ofi_topo.h"
#if HAVE_NVTX_TRACING
#include <nvtx3/nvToolsExt.h>
#endif

#include "transport/rdma/mr_handle.hh"


struct nccl_net_ofi_rdma_req;
struct nccl_net_ofi_rdma_ep;
struct nccl_net_ofi_ep_rail;
typedef struct nccl_net_ofi_rdma_req nccl_net_ofi_rdma_req_t;
typedef struct nccl_net_ofi_rdma_ep nccl_net_ofi_rdma_ep_t;
typedef struct nccl_net_ofi_ep_rail nccl_net_ofi_ep_rail_t;

#endif // End NCCL_OFI_RDMA_H_
