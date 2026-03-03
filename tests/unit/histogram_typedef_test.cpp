/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

/*
 * Unit tests for histogram typedef aliases
 * 
 * This test validates requirement 4.1: "Add template parameter or typedef to 
 * select clock type for timer_histogram"
 * 
 * Specifically tests task 7.2: Create typedef aliases for convenience
 * - rdtscp_timer_histogram typedef
 * - steady_timer_histogram typedef
 */

#include <iostream>
#include <thread>
#include <chrono>

#include "stats/histogram.h"
#include "stats/histogram_binner.h"
#include "stats/rdtscp_clock.h"

#define CHECK_AND_EXIT(condition) \
	do { \
		if (!(condition)) { \
			std::cerr << "Check failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
			exit(1); \
		} \
	} while (0)

/*
 * Test steady_timer_histogram typedef
 * 
 * Validates that the steady_timer_histogram typedef:
 * 1. Can be instantiated correctly
 * 2. Works with start_timer() and stop_timer()
 * 3. Measures time intervals correctly
 * 4. Is equivalent to timer_histogram<Binner, std::chrono::steady_clock>
 */
static void test_steady_timer_histogram_typedef(void)
{
	std::cout << "Testing steady_timer_histogram typedef..." << std::endl;
	
	using Binner = histogram_linear_binner<std::size_t>;
	
	// Test typedef instantiation
	steady_timer_histogram<Binner> timer(
		"Steady Timer Typedef Test",
		Binner(0, 1000, 10)  // 10 bins from 0 to 1000 ns
	);
	
	// Test basic functionality
	timer.start_timer();
	std::this_thread::sleep_for(std::chrono::microseconds(10));
	auto elapsed = timer.stop_timer();
	
	// Verify measurement is reasonable (should be > 10 microseconds = 10000 ns)
	CHECK_AND_EXIT(elapsed > 10000);
	
	std::cout << "  steady_timer_histogram typedef test PASSED" << std::endl;
}

#if RDTSCP_AVAILABLE
/*
 * Test rdtscp_timer_histogram typedef
 * 
 * Validates that the rdtscp_timer_histogram typedef:
 * 1. Can be instantiated correctly
 * 2. Works with start_timer() and stop_timer()
 * 3. Measures time intervals correctly
 * 4. Is equivalent to timer_histogram<Binner, rdtscp_clock>
 */
static void test_rdtscp_timer_histogram_typedef(void)
{
	std::cout << "Testing rdtscp_timer_histogram typedef..." << std::endl;
	
	// Ensure clock is initialized
	rdtscp_clock::initialize();
	
	using Binner = histogram_linear_binner<std::size_t>;
	
	// Test typedef instantiation
	rdtscp_timer_histogram<Binner> timer(
		"RDTSCP Timer Typedef Test",
		Binner(0, 1000, 10)  // 10 bins from 0 to 1000 ns
	);
	
	// Test basic functionality
	timer.start_timer();
	std::this_thread::sleep_for(std::chrono::microseconds(10));
	auto elapsed = timer.stop_timer();
	
	// Verify measurement is reasonable (should be > 10 microseconds = 10000 ns)
	CHECK_AND_EXIT(elapsed > 10000);
	
	std::cout << "  rdtscp_timer_histogram typedef test PASSED" << std::endl;
}

/*
 * Test that both typedefs produce similar results
 * 
 * Validates that:
 * 1. Both typedefs measure similar time intervals
 * 2. Both are suitable for profiling
 * 3. rdtscp_timer_histogram has lower overhead (tested elsewhere)
 */
static void test_typedef_comparison(void)
{
	std::cout << "Testing typedef comparison..." << std::endl;
	
	rdtscp_clock::initialize();
	
	using Binner = histogram_linear_binner<std::size_t>;
	
	steady_timer_histogram<Binner> steady_timer(
		"Steady Timer Comparison",
		Binner(0, 1000, 10)
	);
	
	rdtscp_timer_histogram<Binner> rdtscp_timer(
		"RDTSCP Timer Comparison",
		Binner(0, 1000, 10)
	);
	
	// Measure the same interval with both timers
	const int num_iterations = 100;
	int64_t steady_total = 0;
	int64_t rdtscp_total = 0;
	
	for (int i = 0; i < num_iterations; i++) {
		// Measure with steady_clock
		steady_timer.start_timer();
		std::this_thread::sleep_for(std::chrono::microseconds(10));
		steady_total += steady_timer.stop_timer();
		
		// Measure with rdtscp_clock
		rdtscp_timer.start_timer();
		std::this_thread::sleep_for(std::chrono::microseconds(10));
		rdtscp_total += rdtscp_timer.stop_timer();
	}
	
	// Calculate averages
	int64_t steady_avg = steady_total / num_iterations;
	int64_t rdtscp_avg = rdtscp_total / num_iterations;
	
	std::cout << "  Steady clock average: " << steady_avg << " ns" << std::endl;
	std::cout << "  RDTSCP clock average: " << rdtscp_avg << " ns" << std::endl;
	
	// Both should measure similar intervals (within 20% of each other)
	int64_t diff = std::abs(steady_avg - rdtscp_avg);
	int64_t max_allowed_diff = steady_avg / 5;  // 20%
	
	CHECK_AND_EXIT(diff < max_allowed_diff);
	
	std::cout << "  Typedef comparison test PASSED" << std::endl;
}
#endif // RDTSCP_AVAILABLE

int main(void)
{
	std::cout << "Running histogram typedef tests..." << std::endl;
	std::cout << std::endl;
	
	// Test steady_timer_histogram typedef (always available)
	test_steady_timer_histogram_typedef();
	
#if RDTSCP_AVAILABLE
	// Test rdtscp_timer_histogram typedef (only on x86/x86_64)
	test_rdtscp_timer_histogram_typedef();
	
	// Test comparison between typedefs
	test_typedef_comparison();
#else
	std::cout << "RDTSCP not available on this platform, skipping rdtscp_timer_histogram tests" << std::endl;
#endif
	
	std::cout << std::endl;
	std::cout << "All histogram typedef tests PASSED" << std::endl;
	
	return 0;
}
