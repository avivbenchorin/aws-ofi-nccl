/*
 * Copyright (c) 2023 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef NCCL_OFI_SCHEDULER_H_
#define NCCL_OFI_SCHEDULER_H_

#include <memory_resource>
#include <stdint.h>
#include <pthread.h>

/*
 * @brief	Transfer information for a rail.
 *
 * The transfer information descripes the stripe of a message that is
 * to be send over a specific rail. This struct is part of the
 * schedule that the scheduler provides.
 */
typedef struct nccl_net_ofi_xfer_info {
	/* Id of the rail */
	uint16_t rail_id;
	/* Offset of the stripe into the message */
	size_t offset;
	/* Size of the stripe in bytes */
	size_t msg_size;
} nccl_net_ofi_xfer_info_t;

/*
 * @brief	Schedule of a message
 *
 * A schedule is a partitioning of a message into stripes, each
 * assigned to a different rail.
 */
typedef struct nccl_net_ofi_schedule {
	/* Number of transfer information entries set by the scheduler */
	size_t num_xfer_infos;

	/* Array of transfer information structs. The array has at
	 * least 'num_xfer_infos' entries. */
	nccl_net_ofi_xfer_info_t rail_xfer_infos[];
} nccl_net_ofi_schedule_t;

/*
 * @brief	Base scheduler class
 */
class nccl_net_ofi_scheduler {
public:
	/*
	 * @brief	Size in bytes of a schedule struct for `num_rails' rails
	 */
	static size_t sizeof_schedule(int num_rails_arg)
	{
		return sizeof(nccl_net_ofi_schedule_t)
			+ num_rails_arg * sizeof(nccl_net_ofi_xfer_info_t);
	}

	/*
	 * @brief	Construct base scheduler
	 *
	 * @param	num_rails
	 *		Number of rails that the scheduler should use.
	 *		This parameter must be the same as the parameter used to invoke
	 *		the `get_schedule' method later.
	 */
	nccl_net_ofi_scheduler(int num_rails_arg, size_t align_arg)
		: num_rails(num_rails_arg),
		  align(align_arg),
		  scheduler_pool(std::pmr::pool_options{
			  .max_blocks_per_chunk = 16,
			  .largest_required_pool_block = sizeof_schedule(num_rails)
		  })
	{}
	virtual ~nccl_net_ofi_scheduler() = default;

	/*
	 * @brief	Create schedule for a message
	 *
	 * @param	size
	 *		Size of the message in bytes
	 * @param	num_rails
	 *		Number of rails. This parameter must match the number of rails
	 *		provided to the initialization routine of the scheduler.
	 *
	 * @return	schedule, on success
	 *		NULL, on others
	 */
	virtual nccl_net_ofi_schedule_t *get_schedule(size_t size, int num_rails_arg) = 0;

	/* Number of rails, fixed at construction */
	const int num_rails;

	/* pool_resource and striping alignment value in bytes */
	const size_t align;

	/* Pool allocator for schedule objects */
	std::pmr::unsynchronized_pool_resource scheduler_pool;
};

/*
 * @brief 	The threshold scheduler
 *
 * Messages smaller or equal to `ROUND_ROBIN_THRESHOLD' bytes are
 * assigned round-robin; larger messages are multiplexed.
 */
class nccl_net_ofi_threshold_scheduler : public nccl_net_ofi_scheduler {
public:
	/*
	 * @brief	Construct threshold scheduler 
	 *
	 * Set alignment to 128 bytes for alignment requirement for LL128 protocol
	 *
	 * @param	num_rails
	 *		Number of rails
	 */
	nccl_net_ofi_threshold_scheduler(int num_rails_arg);
	~nccl_net_ofi_threshold_scheduler() override = default;

	/*
	 * @brief	Create schedule for a message by multiplexing message or
	 *		assigning the message round-robin depending on the message size
	 *
	 *		The caller must be holding the endpoint lock to protect the
	 *		rr_small_counter and rr_counter variables.
	 * 
	 * @param	size
	 *		Size of the message in bytes
	 * @param	num_rails
	 *		Number of rails. This parameter must match the number of rails
	 *		provided to the scheduler initialization routine.
	 *
	 * @return	schedule, on success
	 *		NULL, on others
	 */
	nccl_net_ofi_schedule_t *get_schedule(size_t size, int num_rails_arg) override;

	/* Round robin counter */
	unsigned int rr_small_counter;
	unsigned int rr_counter;
	/* threshold for small messages */
	size_t max_small_msg_size;
	/* Minimum size of the message in bytes before message is
	 * multiplexed */
	size_t min_stripe_size;

	/*
	 * @brief	Calculate optimal number of stripes for the payload size
	 *		based on the min_stripe_size
	 *
	 * @param	size
	 *		The size of the message being transmitted
	 * @param	num_rails
	 *		The number of available rails for transmission
	 *
	 * @return	The adjusted number of stripes
	 */
	inline int get_num_stripes(size_t size, int num_rails_arg);
};

/*
 * @brief	Release schedule by returning it back to the scheduler
 */
void nccl_net_ofi_release_schedule(nccl_net_ofi_scheduler *scheduler,
				   nccl_net_ofi_schedule_t *schedule);

#endif // End NCCL_OFI_SCHEDULER_H_
