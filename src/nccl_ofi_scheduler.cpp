/*
 * Copyright (c) 2023 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#include "config.h"

#include <algorithm>
#include <assert.h>
#include <errno.h>
#include <pthread.h>

#include "nccl_ofi_scheduler.h"
#include "nccl_ofi_log.h"
#include "nccl_ofi_math.h"
#include "nccl_ofi_param.h"
#include "nccl_ofi_pthread.h"

/*
 * @brief  This function calculates the optimal number of stripes
 * for the payload size based on the min_stripe_size.
 * It first determines the initial number of stripes based on the message size
 * and the minimum stripe size, ensuring that the result is at least 1 and at
 * most the number of available rails.
 * The function then adjusts the number of stripes to be the largest factor of
 * the number of rails that is less than or equal to the initial number of stripes.
 *
 * @param	size
 * 		The size of the message being transmitted.
 * @param 	scheduler_p
 * 		Pointer to threshold scheduler
 * @param 	num_rails
 * 		The number of available rails for transmission.
 *
 * @return	Returns the adjusted number of stripes.
 *
 */
inline int nccl_net_ofi_threshold_scheduler::get_num_stripes(size_t size, int num_rails_arg)
{
	/* Number of stripes is at least 1 for zero-sized messages and at most equal to num of rails */
	int num_stripes = (int)std::max(1UL, std::min(NCCL_OFI_DIV_CEIL(size,
									this->min_stripe_size),
						      static_cast<long unsigned>(num_rails_arg)));

	/* Start the loop from num_stripes and skip 1, as num_rails % 1 is always true.
	 * This avoids the overhead of the mod operation in latency-sensitive cases.
	 */
	for (int i = num_stripes; i > 1; i--) {
		if ((num_rails_arg % i) == 0) {
			num_stripes = i;
			break;
		}
	}
	return num_stripes;
}

/*
 * Internal: Set schedule that multiplexes messages to all rails.
 *
 * A mininal stripe size `max_stripe_size' is calculated (multiple of
 * `align') that is sufficient to assign the whole message. Rails are
 * filled from low id to large id. The last rail may get assigned less
 * data. The number of rails are calculated based on the ratio of
 * (`data_size` / `min_stripe_size`)
 * 
 * The caller must be holding the endpoint lock to protect the rr_small_counter and
 * rr_counter variables.
 */
static inline int set_schedule_by_threshold(nccl_net_ofi_threshold_scheduler *scheduler,
					    size_t size,
					    int num_rails_arg,
					    size_t align_arg,
					    nccl_net_ofi_schedule_t *schedule)
{
	int ret = 0;
	int num_stripes = 0;

	assert(num_rails_arg > 0);

	if (size < scheduler->max_small_msg_size) {
		int curr_rail_id = scheduler->rr_small_counter;
		scheduler->rr_small_counter = (scheduler->rr_small_counter + 1) % num_rails_arg;

		schedule->num_xfer_infos = 1;

		schedule->rail_xfer_infos[0].rail_id = curr_rail_id;
		schedule->rail_xfer_infos[0].offset = 0;
		schedule->rail_xfer_infos[0].msg_size = size;
		NCCL_OFI_TRACE(NCCL_NET, "scheduler: short size %lu rail %d", size, curr_rail_id);
	} else {
		num_stripes = scheduler->get_num_stripes(size, num_rails_arg);
		assert(num_stripes <= num_rails_arg);

		int curr_rail_id = scheduler->rr_counter;
		scheduler->rr_counter = (scheduler->rr_counter + num_stripes) % num_rails_arg;

		/* Number of bytes left to assign */
		size_t left = size;
		/* Offset into message */
		size_t offset = 0;

		/* Calculate max stripe size as a multiple of 128 for alignment.
		 * Split message size across stripes, ensuring each stripe is within max_stripe_size and LL128 aligned */
		size_t max_stripe_size = NCCL_OFI_DIV_CEIL(NCCL_OFI_DIV_CEIL(size, num_stripes), align_arg) * align_arg;

		schedule->num_xfer_infos = num_stripes;

		NCCL_OFI_TRACE(NCCL_NET, "scheduler: long size %lu start rail %d num_rails %d", size, curr_rail_id, num_stripes);
		/* Compute stripes and assign to rails */
		for (int stripe_idx = 0; stripe_idx < num_stripes; ++stripe_idx) {
			size_t stripe_size = std::min(left, max_stripe_size);

			schedule->rail_xfer_infos[stripe_idx].rail_id = curr_rail_id;
			schedule->rail_xfer_infos[stripe_idx].offset = offset;
			schedule->rail_xfer_infos[stripe_idx].msg_size = stripe_size;

			offset += stripe_size;
			left -= stripe_size;

			curr_rail_id = (curr_rail_id + 1) % num_rails_arg;
		}
	}

	return ret;
}

void nccl_net_ofi_release_schedule(nccl_net_ofi_scheduler *scheduler_p,
				   nccl_net_ofi_schedule_t *schedule)
{
	assert(scheduler_p != NULL);

	scheduler_p->scheduler_pool.deallocate(
		schedule,
		nccl_net_ofi_scheduler::sizeof_schedule(scheduler_p->num_rails),
		scheduler_p->align);
}

/*
 * @brief	Create schedule for a message by myltiplexing message or
 *		assigning the message round-robin depending on the message size.
 *
 * Messages smaller or equal to `ROUND_ROBIN_THRESHOLD' bytes are
 * assigned round-robin; larger messages are multiplexed.
 * 
 * The caller must be holding the endpoint lock to protect the rr_small_counter
 * and rr_counter variables.
 *
 * @param	scheduler_p
 *		Pointer to threshold scheduler
 * @param	size
 *		Size of the message in bytes
 * @param	num_rails
 *		Number of rails. This parameter must match the number of rails
 *		provided to the scheduler initialization routine.
 *
 * @return	schedule, on success
 *		NULL, on others
 */
nccl_net_ofi_schedule_t *nccl_net_ofi_threshold_scheduler::get_schedule(size_t size,
									int num_rails_arg)
{
	void *mem = this->scheduler_pool.allocate(sizeof_schedule(num_rails_arg), this->align);
	if (OFI_UNLIKELY(!mem)) {
		NCCL_OFI_WARN("Failed to allocate schedule");
		return NULL;
	}

	nccl_net_ofi_schedule_t *schedule = (nccl_net_ofi_schedule_t *)mem;

	int ret = set_schedule_by_threshold(this, size, num_rails_arg, this->align, schedule);
	if (OFI_UNLIKELY(ret)) {
		nccl_net_ofi_release_schedule(this, schedule);
		return NULL;
	}

	return schedule;
}


nccl_net_ofi_threshold_scheduler::nccl_net_ofi_threshold_scheduler(int num_rails_arg)
	: nccl_net_ofi_scheduler(num_rails_arg, 128),
	  rr_small_counter(0),
	  rr_counter(0),
	  max_small_msg_size(ofi_nccl_sched_max_small_msg_size()),
	  min_stripe_size(ofi_nccl_min_stripe_size())
{}
