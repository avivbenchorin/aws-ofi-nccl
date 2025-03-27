/*
 * Copyright (c) 2023 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>

#include "nccl_ofi_msgbuff.h"
#include "nccl_ofi_log.h"
#include "nccl_ofi_pthread.h"

nccl_ofi_msgbuff_t::nccl_ofi_msgbuff_t(uint16_t max_inprogress_, uint16_t bit_width) :
	max_inprogress(max_inprogress_),
	field_size((uint16_t)(1 << bit_width)),
	field_mask((uint16_t)(1 << bit_width) - 1),
	msg_last_incomplete(0),
	msg_next(0)
{
	buff = NULL;
	if (max_inprogress == 0 || (uint16_t)(1 << bit_width) <= 2 * max_inprogress) {
		NCCL_OFI_WARN("Wrong parameters for msgbuff_init max_inprogress %" PRIu16 " bit_width %" PRIu16 "",
			      max_inprogress, bit_width);
		return;
	}

	buff = (elem *)malloc(sizeof(elem) * max_inprogress);
	if (!buff) {
		NCCL_OFI_WARN("Memory allocation (msgbuff->buff) failed");
	}
}

nccl_ofi_msgbuff_t::~nccl_ofi_msgbuff_t()
{
	if (!buff) {
		NCCL_OFI_WARN("msgbuff->buff is NULL");
		return;
	}
	free(buff);
	buff = NULL;
}

uint16_t nccl_ofi_msgbuff_t::distance(const uint16_t front, const uint16_t back)
{
	return (front < back ? field_size : 0) + front - back;
}

uint16_t nccl_ofi_msgbuff_t::num_inflight() {
	/**
	 * Computes the "distance" between msg_last_incomplete and msg_next. This works
	 * correctly even if msg_next is wrapped around and msg_last_incomplete has not.
	 */
	return distance(msg_next, msg_last_incomplete);
}

nccl_ofi_msgbuff_t::elem *nccl_ofi_msgbuff_t::buff_idx(uint16_t idx)
{
	return &buff[idx % max_inprogress];
}

/**
 * Given a msg buffer and an index, returns message status
 * @return
 *  nccl_ofi_msgbuff_t::status::COMPLETED
 *  nccl_ofi_msgbuff_t::status::INPROGRESS
 *  nccl_ofi_msgbuff_t::status::NOTSTARTED
 *  nccl_ofi_msgbuff_t::status::UNAVAILABLE
 */
nccl_ofi_msgbuff_t::status nccl_ofi_msgbuff_t::get_idx_status (uint16_t msg_index)
{
	/* Test for INPROGRESS: index is between msg_last_incomplete (inclusive) and msg_next
	 * (exclusive) */
	if (distance(msg_index, msg_last_incomplete) <
	    distance(msg_next, msg_last_incomplete)) {
		return buff_idx(msg_index)->stat;
	}

	/* Test for COMPLETED: index is within max_inprogress below msg_last_incomplete, including
	 * wraparound */
	if (msg_index != msg_last_incomplete &&
	    distance(msg_last_incomplete, msg_index) <= max_inprogress) {
		return status::COMPLETED;
	}

	/* Test for NOTSTARTED: index is >= msg_next and there is room in the buffer */
	if (distance(msg_index, msg_next) <
	    distance(max_inprogress, num_inflight())) {
		return status::NOTSTARTED;
	}

	/* If none of the above apply, then we do not have space to store this message */
	return status::UNAVAILABLE;
}

nccl_ofi_msgbuff_t::result nccl_ofi_msgbuff_t::insert(uint16_t msg_index, 
												   void *element,
												   nccl_ofi_msgbuff_t::elemtype type,
												   nccl_ofi_msgbuff_t::status *msg_idx_status)
{
	std::lock_guard<std::mutex> l(lock);

	*msg_idx_status = get_idx_status(msg_index);
	result ret = result::ERROR;

	if (*msg_idx_status == status::NOTSTARTED) {
		buff_idx(msg_index)->stat = status::INPROGRESS;
		buff_idx(msg_index)->elem = element;
		buff_idx(msg_index)->type = type;
		/* Update msg_next ptr */
		while (distance(msg_index, msg_next) <= max_inprogress) {
			if (msg_next != msg_index) {
				buff_idx(msg_next)->stat = status::NOTSTARTED;
				buff_idx(msg_next)->elem = NULL;
			}
			msg_next = (msg_next + 1) & field_mask;
		}
		ret = result::SUCCESS;
	} else {
		ret = result::INVALID_IDX;
	}

	return ret;
}

nccl_ofi_msgbuff_t::result nccl_ofi_msgbuff_t::replace(uint16_t msg_index,
													   void *element,
													   nccl_ofi_msgbuff_t::elemtype type,
													   nccl_ofi_msgbuff_t::status *msg_idx_status)
{
	std::lock_guard<std::mutex> l(lock);

	*msg_idx_status = get_idx_status(msg_index);
	result ret = result::ERROR;

	if (*msg_idx_status == status::INPROGRESS) {
		buff_idx(msg_index)->elem = element;
		buff_idx(msg_index)->type = type;
		ret = result::SUCCESS;
	} else {
		ret = result::INVALID_IDX;
	}

	return ret;
}

nccl_ofi_msgbuff_t::result nccl_ofi_msgbuff_t::retrieve(uint16_t msg_index,
														void **element,
														nccl_ofi_msgbuff_t::elemtype *type,
														nccl_ofi_msgbuff_t::status *msg_idx_status)
{
	std::lock_guard<std::mutex> l(lock);

	if (OFI_UNLIKELY(!element)) {
		NCCL_OFI_WARN("elem is NULL");
		return result::ERROR;
	}

	*msg_idx_status = get_idx_status(msg_index);
	result ret = result::ERROR;

	if (*msg_idx_status == status::INPROGRESS) {
		*element = buff_idx(msg_index)->elem;
		*type = buff_idx(msg_index)->type;
		ret = result::SUCCESS;
	} else  {
		if (*msg_idx_status == status::UNAVAILABLE) {
			// UNAVAILABLE really only applies to insert, so return NOTSTARTED here
			*msg_idx_status = status::NOTSTARTED;
		}
		ret = result::INVALID_IDX;
	}

	return ret;
}

nccl_ofi_msgbuff_t::result nccl_ofi_msgbuff_t::complete(uint16_t msg_index,
														nccl_ofi_msgbuff_t::status *msg_idx_status)
{
	std::lock_guard<std::mutex> l(lock);

	*msg_idx_status = get_idx_status(msg_index);
	result ret = result::ERROR;

	if (*msg_idx_status == status::INPROGRESS) {
		buff_idx(msg_index)->stat = status::COMPLETED;
		buff_idx(msg_index)->elem = NULL;
		/* Move up tail msg_last_incomplete ptr */
		while (msg_last_incomplete != msg_next &&
				buff_idx(msg_last_incomplete)->stat == status::COMPLETED)
		{
			msg_last_incomplete = (msg_last_incomplete + 1) & field_mask;
		}
		ret = result::SUCCESS;
	} else {
		if (*msg_idx_status == status::UNAVAILABLE) {
			// UNAVAILABLE really only applies to insert, so return NOTSTARTED here
			*msg_idx_status = status::NOTSTARTED;
		}
		ret = result::INVALID_IDX;
	}

	return ret;
}
