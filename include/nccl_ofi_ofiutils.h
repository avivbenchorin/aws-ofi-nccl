/*
 * Copyright (c) 2018-2024 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef NCCL_OFI_OFIUTILS_H
#define NCCL_OFI_OFIUTILS_H

#include <rdma/fabric.h>

#include "nccl_ofi_param.h"
#include "ofi/nccl_ofi_ofiraii.h"

/*
 * Memeory util functions to ensure that the compiler does not optimize
 * these memory accesses.
 */
#define ACCESS_ONCE(x) (*(volatile __typeof__(x) *)&(x))

#define READ_ONCE(x) \
({ __typeof__(x) ___x = ACCESS_ONCE(x); ___x; })

#define WRITE_ONCE(x, val) \
do { ACCESS_ONCE(x) = (val); } while (0)

int nccl_ofi_ofiutils_get_providers(const char *prov_include,
				    uint32_t required_version,
				    struct fi_info *hints,
				    struct fi_info **prov_info_list,
				    unsigned int *num_prov_infos);

/*
 * @brief	Create and initialize libfabric fabric using RAII wrapper
 *
 * @param info		Fabric info for fabric creation
 * @return		Shared RAII fabric wrapper on success, nullptr on failure
 */
shared_fabric_raii nccl_ofi_ofiutils_fabric_create(struct fi_info *info);

/*
 * @brief	Create and initialize libfabric domain using RAII wrapper
 *
 * @param fabric	Shared RAII fabric wrapper
 * @param info		Fabric info for domain creation
 * @return		Shared RAII domain wrapper on success, nullptr on failure
 */
shared_domain_raii nccl_ofi_ofiutils_domain_create(shared_fabric_raii fabric, 
						   struct fi_info *info);

/*
 * @brief	Create and initialize libfabric completion queue using RAII wrapper
 *
 * @param domain	Shared RAII domain wrapper
 * @param cq_attr	CQ attributes for creation
 * @return		Shared RAII CQ wrapper on success, nullptr on failure
 */
shared_cq_raii nccl_ofi_ofiutils_cq_create(shared_domain_raii domain, 
					   struct fi_cq_attr *cq_attr);

/*
 * @brief	Create and initialize libfabric address vector using RAII wrapper
 *
 * @param domain	Shared RAII domain wrapper
 * @return		Shared RAII AV wrapper on success, nullptr on failure
 */
shared_av_raii nccl_ofi_ofiutils_av_create(shared_domain_raii domain);

/*
 * @brief	Create and initialize libfabric endpoint using RAII wrapper
 *
 * Creates endpoint and binds it to the provided AV and CQ. Configures endpoint
 * options and enables it for communication.
 *
 * @param info		Fabric info for endpoint creation
 * @param domain	Shared RAII domain wrapper
 * @param av		Shared RAII address vector wrapper to bind
 * @param cq		Shared RAII completion queue wrapper to bind
 * @return		Shared RAII endpoint wrapper on success, nullptr on failure
 */
shared_ep_raii nccl_ofi_ofiutils_ep_create(struct fi_info *info, 
					   shared_domain_raii domain,
					   shared_av_raii av, 
					   shared_cq_raii cq);

/*
 * @brief	Register memory region with libfabric using RAII wrapper
 *
 * @param domain	Shared RAII domain wrapper
 * @param mr_attr	Memory region attributes structure
 * @param flags		Registration flags
 * @return		Shared RAII MR wrapper on success, nullptr on failure
 */
shared_mr_raii nccl_ofi_ofiutils_mr_regattr(shared_domain_raii domain, 
					    struct fi_mr_attr *mr_attr, 
					    uint64_t flags);

/*
 * @brief	Release libfabric endpoint and address vector using RAII wrappers
 *
 * With RAII wrappers, this function primarily serves as a logging/debugging aid.
 * Actual resource cleanup is handled automatically by the RAII destructors.
 *
 * @param ep		Shared RAII endpoint wrapper to release
 * @param av		Shared RAII address vector wrapper to release
 * @param dev_id	Device ID for logging purposes
 */
void nccl_ofi_ofiutils_ep_release(shared_ep_raii ep, shared_av_raii av, int dev_id);

/*
 * @brief	Free libfabric NIC info list.
 *
 * Frees each node of the list. No operation if list is NULL.
 *
 * @param	info_list
 *		List or circular list of libfabric NIC infos
 */
void nccl_ofi_ofiutils_free_info_list(struct fi_info *info_list);

int nccl_ofi_mr_keys_need_own_key(struct fi_info* provider, bool *provide_own_mr_key);

inline enum fi_progress nccl_ofi_translate_progress_enum(PROGRESS_MODEL model_type)
{
	enum fi_progress ret = FI_PROGRESS_UNSPEC;

	switch (model_type) {
	case PROGRESS_MODEL::UNSPEC:
		ret = FI_PROGRESS_UNSPEC;
		break;
	case PROGRESS_MODEL::AUTO:
		ret = FI_PROGRESS_AUTO;
		break;
	case PROGRESS_MODEL::MANUAL:
		ret = FI_PROGRESS_MANUAL;
		break;
	}

	return ret;
}

#endif
