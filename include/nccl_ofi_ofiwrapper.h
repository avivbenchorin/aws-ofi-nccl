/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef NCCL_OFI_OFIWRAPPER_H_
#define NCCL_OFI_OFIWRAPPER_H_

#include <memory>
#include <rdma/fabric.h>
#include "nccl_ofi_log.h"

/**
 * @brief Custom deleter for fid_ep resources
 * 
 * Provides proper cleanup of libfabric endpoint resources with
 * appropriate error logging.
 */
struct FidEpDeleter {
    void operator()(struct fid_ep* ep) const {
        if (ep) {
            int ret = fi_close(&ep->fid);
            if (ret != 0) {
                NCCL_OFI_WARN("Failed to close fid_ep: %s", fi_strerror(-ret));
            }
        }
    }
};

/**
 * @brief Custom deleter for fid_av resources
 * 
 * Provides proper cleanup of libfabric address vector resources with
 * appropriate error logging.
 */
struct FidAvDeleter {
    void operator()(struct fid_av* av) const {
        if (av) {
            int ret = fi_close(&av->fid);
            if (ret != 0) {
                NCCL_OFI_WARN("Failed to close fid_av: %s", fi_strerror(-ret));
            }
        }
    }
};

/**
 * Type aliases for cleaner code and easier maintenance
 */
using FidEpPtr = std::unique_ptr<struct fid_ep, FidEpDeleter>;
using FidAvPtr = std::unique_ptr<struct fid_av, FidAvDeleter>;

/**
 * @brief Factory function for creating fid_ep smart pointers
 * 
 * @param ep     Raw fid_ep pointer to wrap
 * @return       Smart pointer with proper deleter
 */
inline FidEpPtr make_fid_ep_ptr(struct fid_ep* ep) {
    return FidEpPtr(ep);
}

/**
 * @brief Factory function for creating fid_av smart pointers
 * 
 * @param av     Raw fid_av pointer to wrap
 * @return       Smart pointer with proper deleter
 */
inline FidAvPtr make_fid_av_ptr(struct fid_av* av) {
    return FidAvPtr(av);
}

#endif // NCCL_OFI_OFIWRAPPER_H_
