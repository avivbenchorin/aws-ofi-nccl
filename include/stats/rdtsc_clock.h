/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef NCCL_OFI_STATS_RDTSC_CLOCK_H
#define NCCL_OFI_STATS_RDTSC_CLOCK_H

#include <cstdint>
#include <chrono>
#include "rdtsc_platform.h"
#include "rdtsc_calibration.h"

/**
 * @file rdtsc_clock.h
 * @brief Clock adapter for RDTSC that satisfies C++ Clock concept
 *
 * This file provides a clock adapter that wraps RDTSC functionality in a
 * C++ Clock-compatible interface. This allows RDTSC to be used with
 * standard C++ timing utilities and the existing timer_histogram class.
 *
 * Key features:
 * - Satisfies C++ Clock named requirements
 * - Stores time points as raw CPU cycles (no conversion overhead)
 * - Provides conversion to nanoseconds when needed
 * - Compatible with std::chrono duration and time_point types
 *
 * Usage:
 *   auto start = rdtsc_clock::now();
 *   // ... timed operation ...
 *   auto end = rdtsc_clock::now();
 *   auto cycles = (end - start).count();
 *   auto ns = rdtsc_clock::to_nanoseconds(cycles);
 */

namespace nccl_ofi_rdtsc {

/**
 * @struct rdtsc_clock
 * @brief Clock adapter that satisfies C++ Clock concept using RDTSC
 *
 * This struct provides a Clock-compatible interface for RDTSC timing.
 * Time points are stored as raw CPU cycles, with conversion to nanoseconds
 * performed only when explicitly requested via to_nanoseconds().
 *
 * Clock properties:
 * - is_steady: true (monotonically increasing)
 * - rep: uint64_t (cycle count representation)
 * - period: std::nano (nanosecond precision after conversion)
 *
 * Thread safety: now() and now_serialized() are thread-safe. Calibration
 * is performed once on first use of the calibration singleton.
 */
struct rdtsc_clock {
	/// Representation type for cycle counts
	using rep = uint64_t;
	
	/// Period type (nanoseconds, though we store cycles)
	using period = std::nano;
	
	/// Duration type (cycles stored as nanoseconds for compatibility)
	using duration = std::chrono::duration<rep, period>;
	
	/// Time point type
	using time_point = std::chrono::time_point<rdtsc_clock>;
	
	/// Clock is steady (monotonically increasing)
	static constexpr bool is_steady = true;

	/**
	 * @brief Get current time point (non-serializing)
	 *
	 * Returns the current time as a time_point containing raw CPU cycles.
	 * Uses the non-serializing rdtsc() for minimal overhead. Suitable for
	 * start timing where instruction reordering is acceptable.
	 *
	 * @return Current time point (in CPU cycles)
	 *
	 * @note The returned value is in CPU cycles, not nanoseconds. Use
	 *       to_nanoseconds() to convert to nanoseconds.
	 */
	static time_point now() noexcept {
		uint64_t cycles = rdtsc();
		return time_point(duration(cycles));
	}

	/**
	 * @brief Get current time point with serialization
	 *
	 * Returns the current time as a time_point containing raw CPU cycles.
	 * Uses the serializing rdtscp() to ensure all previous instructions
	 * have completed. Suitable for stop timing where accuracy is critical.
	 *
	 * @return Current time point (in CPU cycles)
	 *
	 * @note The returned value is in CPU cycles, not nanoseconds. Use
	 *       to_nanoseconds() to convert to nanoseconds.
	 */
	static time_point now_serialized() noexcept {
		uint64_t cycles = rdtscp();
		return time_point(duration(cycles));
	}

	/**
	 * @brief Convert CPU cycles to nanoseconds
	 *
	 * Converts a cycle count to nanoseconds using the calibrated frequency.
	 * This is the primary conversion function for interpreting timing results.
	 *
	 * @param cycles Number of CPU cycles to convert
	 * @return Equivalent time in nanoseconds
	 *
	 * @note This function uses the calibration singleton, which performs
	 *       calibration on first use. Subsequent calls use cached values.
	 */
	static uint64_t to_nanoseconds(uint64_t cycles) {
		return calibration::instance().cycles_to_ns(cycles);
	}

	/**
	 * @brief Convert nanoseconds to CPU cycles
	 *
	 * Converts a time in nanoseconds to the equivalent number of CPU cycles
	 * using the calibrated frequency. Useful for setting cycle-based
	 * thresholds or overhead values.
	 *
	 * @param ns Time in nanoseconds to convert
	 * @return Equivalent number of CPU cycles
	 */
	static uint64_t to_cycles(uint64_t ns) {
		return calibration::instance().ns_to_cycles(ns);
	}
};

} // namespace nccl_ofi_rdtsc

#endif // NCCL_OFI_STATS_RDTSC_CLOCK_H
