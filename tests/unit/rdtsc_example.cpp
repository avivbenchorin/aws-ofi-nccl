/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

/**
 * @file rdtsc_example.cpp
 * @brief Example demonstrating RDTSC overhead measurement and usage
 *
 * This example shows how to:
 * 1. Measure RDTSC timing overhead
 * 2. Create histograms with overhead subtraction
 * 3. Use RDTSC timing for profiling mutex operations
 */

#include "config.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <pthread.h>

#ifdef ENABLE_RDTSC_TIMING

#include "stats/rdtsc_histogram.h"
#include "stats/rdtsc_platform.h"
#include "stats/rdtsc_calibration.h"
#include "stats/histogram_binner.h"
#include "nccl_ofi_pthread.h"

using namespace nccl_ofi_rdtsc;

int main(int argc, char *argv[])
{
	std::cout << "=== RDTSC Overhead Measurement Example ===" << std::endl;
	std::cout << std::endl;

	// Step 1: Measure RDTSC overhead
	std::cout << "Step 1: Measuring RDTSC timing overhead..." << std::endl;
	uint64_t overhead_cycles = rdtsc_timer_histogram<histogram_custom_binner<size_t>>::measure_overhead();
	uint64_t overhead_ns = rdtsc_clock::to_nanoseconds(overhead_cycles);
	
	std::cout << "  Measured overhead: " << overhead_cycles << " cycles (~" 
	          << overhead_ns << " ns)" << std::endl;
	std::cout << std::endl;

	// Step 2: Create histogram with overhead subtraction
	std::cout << "Step 2: Creating histogram with overhead subtraction..." << std::endl;
	std::vector<size_t> bins = {0, 10, 20, 30, 40, 50, 100, 200, 500, 1000, 2000};
	
	rdtsc_timer_histogram<histogram_custom_binner<size_t>> mutex_hist(
		"mutex_unlock_timing",
		histogram_custom_binner<size_t>(bins),
		overhead_cycles  // Subtract overhead in cycle domain
	);
	
	std::cout << "  Created histogram with " << overhead_cycles 
	          << " cycle overhead subtraction" << std::endl;
	std::cout << std::endl;

	// Step 3: Profile mutex operations
	std::cout << "Step 3: Profiling mutex lock/unlock operations..." << std::endl;
	pthread_mutex_t test_mutex;
	pthread_mutex_init(&test_mutex, nullptr);
	
	// Perform some mutex operations
	for (int i = 0; i < 1000; ++i) {
		pthread_mutex_lock(&test_mutex);
		
		// Time the unlock operation
		mutex_hist.start_timer();
		pthread_mutex_unlock(&test_mutex);
		mutex_hist.stop_timer();
	}
	
	pthread_mutex_destroy(&test_mutex);
	
	std::cout << "  Completed 1000 mutex unlock measurements" << std::endl;
	std::cout << std::endl;

	// Step 4: Display results
	std::cout << "Step 4: Results" << std::endl;
	std::cout << "----------------------------------------" << std::endl;
	mutex_hist.print_stats();
	std::cout << std::endl;

	// Step 5: Demonstrate the difference with/without overhead subtraction
	std::cout << "Step 5: Comparing with/without overhead subtraction..." << std::endl;
	std::cout << std::endl;
	
	rdtsc_timer_histogram<histogram_custom_binner<size_t>> hist_no_overhead(
		"no_overhead_subtraction",
		histogram_custom_binner<size_t>(bins),
		0  // No overhead subtraction
	);
	
	rdtsc_timer_histogram<histogram_custom_binner<size_t>> hist_with_overhead(
		"with_overhead_subtraction",
		histogram_custom_binner<size_t>(bins),
		overhead_cycles
	);
	
	// Measure very short operations
	for (int i = 0; i < 100; ++i) {
		hist_no_overhead.start_timer();
		asm volatile("" : : : "memory");  // Minimal operation
		hist_no_overhead.stop_timer();
		
		hist_with_overhead.start_timer();
		asm volatile("" : : : "memory");  // Minimal operation
		hist_with_overhead.stop_timer();
	}
	
	std::cout << "Without overhead subtraction:" << std::endl;
	hist_no_overhead.print_stats();
	std::cout << std::endl;
	
	std::cout << "With overhead subtraction:" << std::endl;
	hist_with_overhead.print_stats();
	std::cout << std::endl;

	std::cout << "=== Example Complete ===" << std::endl;
	std::cout << std::endl;
	std::cout << "Key takeaways:" << std::endl;
	std::cout << "1. RDTSC overhead is typically 10-30 cycles (~" << overhead_ns << " ns on this system)" << std::endl;
	std::cout << "2. Overhead subtraction in cycle domain provides accurate measurements" << std::endl;
	std::cout << "3. For very short operations, overhead subtraction is essential" << std::endl;
	std::cout << "4. RDTSC provides much lower overhead than std::chrono (~270ns)" << std::endl;
	
	return 0;
}

#else // !ENABLE_RDTSC_TIMING

int main(int argc, char *argv[])
{
	std::cout << "RDTSC timing is not enabled." << std::endl;
	std::cout << "Configure with --enable-rdtsc-timing to run this example." << std::endl;
	return 0;
}

#endif // ENABLE_RDTSC_TIMING
