/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#include "config.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <thread>

#include "test-logger.h"

#ifdef ENABLE_RDTSC_TIMING
#include "stats/rdtsc_histogram.h"
#include "stats/rdtsc_platform.h"
#include "stats/rdtsc_calibration.h"
#include "stats/histogram_binner.h"

using namespace nccl_ofi_rdtsc;

/**
 * @brief Test RDTSC overhead measurement
 *
 * This test measures the overhead of RDTSC timing operations and compares
 * it with std::chrono timing overhead. It validates that RDTSC provides
 * significantly lower overhead (<50ns target vs ~270ns for chrono).
 */
int main(int argc, char *argv[])
{
	std::cout << "=== RDTSC Overhead Measurement Test ===" << std::endl;
	std::cout << std::endl;

	// Print calibration information
	auto& calib = calibration::instance();
	std::cout << "CPU Frequency: " << std::fixed << std::setprecision(2) 
	          << (calib.get_frequency_mhz()) << " MHz" << std::endl;
	std::cout << std::endl;

	// Measure RDTSC overhead
	std::cout << "Measuring RDTSC timing overhead..." << std::endl;
	uint64_t rdtsc_overhead_cycles = rdtsc_timer_histogram<histogram_custom_binner<size_t>>::measure_overhead(10000);
	uint64_t rdtsc_overhead_ns = rdtsc_clock::to_nanoseconds(rdtsc_overhead_cycles);
	
	std::cout << "  RDTSC overhead: " << rdtsc_overhead_cycles << " cycles" << std::endl;
	std::cout << "  RDTSC overhead: ~" << rdtsc_overhead_ns << " ns" << std::endl;
	std::cout << std::endl;

	// Measure std::chrono overhead for comparison
	std::cout << "Measuring std::chrono timing overhead..." << std::endl;
	uint64_t min_chrono_overhead = UINT64_MAX;
	
	// Warm up
	for (int i = 0; i < 100; ++i) {
		auto start = std::chrono::steady_clock::now();
		auto end = std::chrono::steady_clock::now();
		(void)start;
		(void)end;
	}
	
	// Measure
	for (int i = 0; i < 10000; ++i) {
		auto start = std::chrono::steady_clock::now();
		auto end = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
		if (elapsed >= 0 && static_cast<uint64_t>(elapsed) < min_chrono_overhead) {
			min_chrono_overhead = static_cast<uint64_t>(elapsed);
		}
	}
	
	std::cout << "  std::chrono overhead: ~" << min_chrono_overhead << " ns" << std::endl;
	std::cout << std::endl;

	// Calculate improvement
	if (min_chrono_overhead > 0) {
		double improvement = (1.0 - static_cast<double>(rdtsc_overhead_ns) / static_cast<double>(min_chrono_overhead)) * 100.0;
		std::cout << "Overhead reduction: " << std::fixed << std::setprecision(1) 
		          << improvement << "%" << std::endl;
		std::cout << std::endl;
	}

	// Validate target (<50ns)
	bool passed = (rdtsc_overhead_ns < 50);
	
	if (passed) {
		std::cout << "✓ PASS: RDTSC overhead is less than 50ns target" << std::endl;
	} else {
		std::cout << "✗ FAIL: RDTSC overhead exceeds 50ns target" << std::endl;
	}
	std::cout << std::endl;

	// Test overhead subtraction in histogram
	std::cout << "Testing overhead subtraction in histogram..." << std::endl;
	
	std::vector<size_t> bins = {0, 10, 20, 30, 40, 50, 100, 200, 500, 1000};
	rdtsc_timer_histogram<histogram_custom_binner<size_t>> hist_with_overhead(
		"with_overhead", 
		histogram_custom_binner<size_t>(bins),
		rdtsc_overhead_cycles
	);
	
	rdtsc_timer_histogram<histogram_custom_binner<size_t>> hist_without_overhead(
		"without_overhead", 
		histogram_custom_binner<size_t>(bins),
		0
	);
	
	// Perform some very short operations
	for (int i = 0; i < 100; ++i) {
		// With overhead subtraction
		hist_with_overhead.start_timer();
		asm volatile("" : : : "memory");  // Prevent optimization
		hist_with_overhead.stop_timer();
		
		// Without overhead subtraction
		hist_without_overhead.start_timer();
		asm volatile("" : : : "memory");  // Prevent optimization
		hist_without_overhead.stop_timer();
	}
	
	std::cout << std::endl;
	std::cout << "Histogram WITH overhead subtraction:" << std::endl;
	hist_with_overhead.print_stats();
	std::cout << std::endl;
	
	std::cout << "Histogram WITHOUT overhead subtraction:" << std::endl;
	hist_without_overhead.print_stats();
	std::cout << std::endl;

	// Test with actual work (small sleep)
	std::cout << "Testing with actual work (100ns sleep)..." << std::endl;
	rdtsc_timer_histogram<histogram_custom_binner<size_t>> hist_work(
		"100ns_sleep", 
		histogram_custom_binner<size_t>(bins),
		rdtsc_overhead_cycles
	);
	
	for (int i = 0; i < 50; ++i) {
		hist_work.start_timer();
		std::this_thread::sleep_for(std::chrono::nanoseconds(100));
		hist_work.stop_timer();
	}
	
	std::cout << std::endl;
	hist_work.print_stats();
	std::cout << std::endl;

	return passed ? 0 : 1;
}

#else // !ENABLE_RDTSC_TIMING

int main(int argc, char *argv[])
{
	std::cout << "RDTSC timing is not enabled. Skipping test." << std::endl;
	std::cout << "Configure with --enable-rdtsc-timing to enable this test." << std::endl;
	return 0;
}

#endif // ENABLE_RDTSC_TIMING
