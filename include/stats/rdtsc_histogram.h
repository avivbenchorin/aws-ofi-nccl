/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef NCCL_OFI_STATS_RDTSC_HISTOGRAM_H
#define NCCL_OFI_STATS_RDTSC_HISTOGRAM_H

#include <cstdint>
#include "histogram.h"
#include "rdtsc_clock.h"
#include "rdtsc_platform.h"

/**
 * @file rdtsc_histogram.h
 * @brief RDTSC-based timer histogram for low-overhead profiling
 *
 * This file provides a specialization of timer_histogram that uses RDTSC
 * for timing instead of std::chrono. This reduces timing overhead from
 * ~270ns to <50ns, making it suitable for profiling very fast operations
 * like mutex unlocks.
 *
 * Key features:
 * - Inherits from histogram<T, Binner> for compatibility
 * - Uses RDTSC for minimal timing overhead (~10-30 cycles)
 * - Stores raw cycles internally, converts to nanoseconds on stop
 * - Memory barriers prevent compiler/CPU reordering
 * - Overhead subtraction in cycle domain for accuracy
 *
 * Performance characteristics:
 * - start_timer(): ~10-20 cycles (non-serializing RDTSC)
 * - stop_timer(): ~20-30 cycles (serializing RDTSCP + conversion)
 * - Total overhead: <50ns on modern CPUs (vs ~270ns for chrono)
 *
 * Usage:
 *   std::vector<size_t> bins = {0, 50, 100, 200, 500, 1000};
 *   rdtsc_timer_histogram<histogram_custom_binner<size_t>> hist(
 *       "mutex_unlock", histogram_custom_binner<size_t>(bins));
 *   
 *   hist.start_timer();
 *   // ... operation to time ...
 *   hist.stop_timer();  // Returns elapsed time in nanoseconds
 */

/**
 * @class rdtsc_timer_histogram
 * @brief Timer histogram using RDTSC for low-overhead timing
 *
 * This class provides a drop-in replacement for timer_histogram that uses
 * RDTSC instead of std::chrono for timing. It maintains API compatibility
 * while providing significantly lower overhead.
 *
 * Template parameters:
 * @tparam Binner Histogram binner type (e.g., histogram_custom_binner)
 * @tparam T Data type for histogram bins (default: std::size_t)
 *
 * Thread safety: Each instance should be used by a single thread. The
 * underlying calibration singleton is thread-safe.
 */
template <typename Binner, typename T = std::size_t>
class rdtsc_timer_histogram : public histogram<T, Binner> {
public:
	using rep = T;
	using histogram<T, Binner>::insert;

	/**
	 * @brief Constructor with cycle-based overhead
	 *
	 * Creates a new RDTSC timer histogram with the specified description,
	 * binner, and optional overhead value in CPU cycles.
	 *
	 * @param description_arg Human-readable description for logging
	 * @param binner_arg Binner instance defining histogram bins
	 * @param overhead_cycles Optional overhead to subtract (in CPU cycles)
	 *
	 * @note The overhead parameter is in CPU cycles, not nanoseconds. This
	 *       allows more accurate overhead subtraction.
	 */
	rdtsc_timer_histogram(const std::string &description_arg, 
	                      Binner binner_arg, 
	                      T overhead_cycles = 0)
		: histogram<T, Binner>(description_arg, binner_arg, overhead_cycles),
		  start_cycles_(0)
	{
	}

	/**
	 * @brief Constructor with nanosecond-based overhead (for compatibility)
	 *
	 * Creates a new RDTSC timer histogram with the specified description,
	 * binner, and optional overhead value in nanoseconds. The nanosecond
	 * overhead is automatically converted to CPU cycles using calibration.
	 *
	 * @param description_arg Human-readable description for logging
	 * @param binner_arg Binner instance defining histogram bins
	 * @param overhead_ns Overhead to subtract (in nanoseconds)
	 *
	 * @note This constructor provides compatibility with the chrono-based
	 *       timer_histogram API. The nanosecond value is converted to cycles
	 *       for more accurate overhead subtraction.
	 */
	template<typename Rep, typename Period>
	rdtsc_timer_histogram(const std::string &description_arg, 
	                      Binner binner_arg, 
	                      const std::chrono::duration<Rep, Period> &overhead_ns)
		: histogram<T, Binner>(description_arg, binner_arg, 
		                       static_cast<T>(nccl_ofi_rdtsc::rdtsc_clock::to_cycles(
		                           std::chrono::duration_cast<std::chrono::nanoseconds>(overhead_ns).count()))),
		  start_cycles_(0)
	{
	}

	/**
	 * @brief Start timing an operation
	 *
	 * Records the current cycle count using non-serializing RDTSC. Memory
	 * barriers prevent compiler reordering but allow CPU-level reordering
	 * for minimal overhead.
	 *
	 * @note Use this for the start of timing. The non-serializing nature
	 *       provides minimal overhead while memory barriers prevent compiler
	 *       reordering. For stop timing, use stop_timer() which uses the
	 *       serializing RDTSCP instruction.
	 */
	void start_timer() {
		asm volatile("" : : : "memory");  // Compiler barrier (prevent reordering)
		start_cycles_ = nccl_ofi_rdtsc::rdtsc();
		asm volatile("" : : : "memory");  // Compiler barrier (prevent reordering)
	}

	/**
	 * @brief Stop timing and record result
	 *
	 * Records the current cycle count using serializing RDTSCP, calculates
	 * elapsed cycles, subtracts overhead, converts to nanoseconds, and
	 * inserts the result into the histogram.
	 *
	 * @return Elapsed time in nanoseconds (after overhead subtraction)
	 *
	 * @note Uses RDTSCP (serializing) to ensure all previous instructions
	 *       have completed before reading the cycle count. This provides
	 *       accurate timing of completed operations.
	 */
	rep stop_timer() {
		asm volatile("" : : : "memory");  // Compiler barrier (prevent reordering)
		uint64_t end_cycles = nccl_ofi_rdtsc::rdtscp();
		asm volatile("" : : : "memory");  // Compiler barrier (prevent reordering)

		// Calculate elapsed cycles
		uint64_t elapsed_cycles = end_cycles - start_cycles_;
		
		// Subtract overhead in cycle domain (more accurate than ns domain)
		if (elapsed_cycles > this->overhead) {
			elapsed_cycles -= this->overhead;
		} else {
			// If elapsed is less than overhead, clamp to zero
			elapsed_cycles = 0;
		}

		// Convert to nanoseconds for histogram
		uint64_t elapsed_ns = nccl_ofi_rdtsc::rdtsc_clock::to_nanoseconds(elapsed_cycles);
		
		// Insert into histogram
		insert(static_cast<T>(elapsed_ns));
		
		return static_cast<rep>(elapsed_ns);
	}

	/**
	 * @brief Measure RDTSC timing overhead
	 *
	 * Measures the overhead of the RDTSC timing mechanism itself by performing
	 * back-to-back start_timer() and stop_timer() calls with no operation in
	 * between. This overhead can then be subtracted from actual measurements
	 * for more accurate profiling.
	 *
	 * The measurement is performed multiple times and the minimum value is
	 * returned, as this represents the best-case scenario with minimal
	 * interference from interrupts or context switches.
	 *
	 * @param iterations Number of measurements to perform (default: 1000)
	 * @return Minimum measured overhead in CPU cycles
	 *
	 * @note The returned value is in CPU cycles, not nanoseconds. This allows
	 *       for more accurate overhead subtraction in the cycle domain.
	 *
	 * Usage example:
	 *   uint64_t overhead_cycles = rdtsc_timer_histogram<...>::measure_overhead();
	 *   uint64_t overhead_ns = rdtsc_clock::to_nanoseconds(overhead_cycles);
	 *   printf("RDTSC overhead: %lu cycles (~%lu ns)\n", overhead_cycles, overhead_ns);
	 *   
	 *   // Create histogram with overhead subtraction
	 *   rdtsc_timer_histogram<...> hist("operation", binner, overhead_cycles);
	 */
	static uint64_t measure_overhead(size_t iterations = 1000) {
		uint64_t min_overhead = UINT64_MAX;
		
		// Warm up the cache and CPU branch predictor
		for (size_t i = 0; i < 100; ++i) {
			asm volatile("" : : : "memory");
			uint64_t start = nccl_ofi_rdtsc::rdtsc();
			asm volatile("" : : : "memory");
			asm volatile("" : : : "memory");
			uint64_t end = nccl_ofi_rdtsc::rdtscp();
			asm volatile("" : : : "memory");
			(void)start;
			(void)end;
		}
		
		// Measure overhead multiple times and take minimum
		for (size_t i = 0; i < iterations; ++i) {
			// Replicate the exact sequence used in start_timer() and stop_timer()
			asm volatile("" : : : "memory");  // Compiler barrier
			uint64_t start = nccl_ofi_rdtsc::rdtsc();
			asm volatile("" : : : "memory");  // Compiler barrier
			
			// Immediately stop timing (no operation in between)
			asm volatile("" : : : "memory");  // Compiler barrier
			uint64_t end = nccl_ofi_rdtsc::rdtscp();
			asm volatile("" : : : "memory");  // Compiler barrier
			
			uint64_t overhead = end - start;
			
			// Take minimum to avoid interference from interrupts
			if (overhead < min_overhead) {
				min_overhead = overhead;
			}
		}
		
		return min_overhead;
	}

	/**
	 * @brief Measure RDTSC timing overhead in nanoseconds
	 *
	 * Convenience wrapper around measure_overhead() that returns the result
	 * in nanoseconds instead of CPU cycles.
	 *
	 * @param iterations Number of measurements to perform (default: 1000)
	 * @return Minimum measured overhead in nanoseconds
	 *
	 * Usage example:
	 *   uint64_t overhead_ns = rdtsc_timer_histogram<...>::measure_overhead_ns();
	 *   printf("RDTSC overhead: ~%lu ns\n", overhead_ns);
	 */
	static uint64_t measure_overhead_ns(size_t iterations = 1000) {
		uint64_t overhead_cycles = measure_overhead(iterations);
		return nccl_ofi_rdtsc::rdtsc_clock::to_nanoseconds(overhead_cycles);
	}

private:
	uint64_t start_cycles_;  ///< Start cycle count from last start_timer() call
};

#endif // NCCL_OFI_STATS_RDTSC_HISTOGRAM_H
