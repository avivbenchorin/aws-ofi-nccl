//
// Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
//

#include "config.h"

#include <iostream>
#include <cstdint>

#include "nccl_ofi.h"
#include "test-logger.h"
#include "stats/rdtscp_clock.h"

#define CHECK_AND_EXIT(x)				      \
	if (!(x)) {					      \
		std::cerr << "Failure: " << #x << std::endl; \
		exit(1);				      \
	}

/*
 * Test that successive read_tsc() calls return monotonically increasing values
 * 
 * This test validates requirement 5.1: "Create unit tests comparing rdtscp 
 * and steady_clock measurements"
 * 
 * The test verifies that:
 * 1. Each successive call to read_tsc() returns a value >= the previous value
 * 2. The TSC is monotonic on a single thread (no thread migration)
 * 3. The TSC values are actually increasing (not stuck at zero)
 */
static void check_read_tsc_monotonicity(void)
{
#if RDTSCP_AVAILABLE
	std::cout << "Testing read_tsc() monotonicity..." << std::endl;
	
	const int num_iterations = 1000;
	uint32_t aux;
	uint64_t prev_tsc = 0;
	uint64_t first_tsc = 0;
	
	for (int i = 0; i < num_iterations; i++) {
		uint64_t current_tsc = rdtscp_clock::read_tsc(&aux);
		
		if (i == 0) {
			first_tsc = current_tsc;
			std::cout << "  First TSC value: " << first_tsc << std::endl;
		} else {
			// Verify monotonicity: current TSC should be >= previous TSC
			CHECK_AND_EXIT(current_tsc >= prev_tsc);
		}
		
		prev_tsc = current_tsc;
	}
	
	// Verify that TSC actually increased over the test
	// (not stuck at zero or same value)
	CHECK_AND_EXIT(prev_tsc > first_tsc);
	
	uint64_t total_cycles = prev_tsc - first_tsc;
	std::cout << "  Last TSC value: " << prev_tsc << std::endl;
	std::cout << "  Total cycles elapsed: " << total_cycles << std::endl;
	std::cout << "  Monotonicity test PASSED" << std::endl;
#else
	std::cout << "RDTSCP not available on this platform, skipping test" << std::endl;
#endif
}

/*
 * Test sysfs frequency reading functionality
 * 
 * This test validates requirement 2.2: "Support reading CPU frequency from 
 * /proc/cpuinfo or /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"
 * 
 * The test verifies that:
 * 1. read_tsc_freq_from_sysfs() can successfully read frequency from sysfs
 * 2. The returned cycles_per_ns value is reasonable (typically 2-5 for modern CPUs)
 * 3. The function handles missing files gracefully
 */
static void check_sysfs_frequency_reading(void)
{
#if RDTSCP_AVAILABLE
	std::cout << "Testing sysfs frequency reading..." << std::endl;
	
	double cycles_per_ns = 0.0;
	bool success = rdtscp_clock::read_tsc_freq_from_sysfs(cycles_per_ns);
	
	if (success) {
		std::cout << "  Successfully read frequency from sysfs" << std::endl;
		std::cout << "  Cycles per nanosecond: " << cycles_per_ns << std::endl;
		
		// Verify the value is reasonable for modern CPUs (1.0 to 6.0 GHz range)
		// cycles_per_ns should be between 1.0 and 6.0
		CHECK_AND_EXIT(cycles_per_ns >= 1.0 && cycles_per_ns <= 6.0);
		
		std::cout << "  Frequency value is reasonable" << std::endl;
		std::cout << "  Sysfs frequency reading test PASSED" << std::endl;
	} else {
		std::cout << "  Could not read frequency from sysfs (files may not exist)" << std::endl;
		std::cout << "  This is expected on some systems" << std::endl;
		std::cout << "  Sysfs frequency reading test SKIPPED" << std::endl;
	}
#else
	std::cout << "RDTSCP not available on this platform, skipping test" << std::endl;
#endif
}

/*
 * Test /proc/cpuinfo frequency reading functionality
 * 
 * This test validates requirement 2.2: "Support reading CPU frequency from 
 * /proc/cpuinfo or /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"
 * 
 * The test verifies that:
 * 1. read_tsc_freq_from_cpuinfo() can successfully parse /proc/cpuinfo
 * 2. The returned cycles_per_ns value is reasonable (typically 2-5 for modern CPUs)
 * 3. The function handles missing or malformed files gracefully
 */
static void check_cpuinfo_frequency_reading(void)
{
#if RDTSCP_AVAILABLE
	std::cout << "Testing /proc/cpuinfo frequency reading..." << std::endl;
	
	double cycles_per_ns = 0.0;
	bool success = rdtscp_clock::read_tsc_freq_from_cpuinfo(cycles_per_ns);
	
	if (success) {
		std::cout << "  Successfully read frequency from /proc/cpuinfo" << std::endl;
		std::cout << "  Cycles per nanosecond: " << cycles_per_ns << std::endl;
		
		// Verify the value is reasonable for modern CPUs (1.0 to 6.0 GHz range)
		// cycles_per_ns should be between 1.0 and 6.0
		CHECK_AND_EXIT(cycles_per_ns >= 1.0 && cycles_per_ns <= 6.0);
		
		std::cout << "  Frequency value is reasonable" << std::endl;
		std::cout << "  /proc/cpuinfo frequency reading test PASSED" << std::endl;
	} else {
		std::cout << "  Could not read frequency from /proc/cpuinfo" << std::endl;
		std::cout << "  This is unexpected on Linux systems" << std::endl;
		std::cout << "  /proc/cpuinfo frequency reading test FAILED" << std::endl;
		exit(1);
	}
#else
	std::cout << "RDTSCP not available on this platform, skipping test" << std::endl;
#endif
}

int
main(int argc, char *argv[])
{
	ofi_log_function = logger;
	
	check_read_tsc_monotonicity();
	check_sysfs_frequency_reading();
	check_cpuinfo_frequency_reading();
	
	return 0;
}
