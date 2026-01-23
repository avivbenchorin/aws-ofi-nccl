/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef NCCL_OFI_STATS_RDTSC_CALIBRATION_H
#define NCCL_OFI_STATS_RDTSC_CALIBRATION_H

#include <cstdint>

/**
 * @file rdtsc_calibration.h
 * @brief RDTSC frequency calibration and cycle-to-time conversion
 *
 * This file provides a singleton class for calibrating RDTSC frequency and
 * converting between CPU cycles and nanoseconds. Calibration is performed
 * once at first use and cached for subsequent conversions.
 *
 * Calibration algorithm:
 * 1. Measure RDTSC cycles over a known time period (using std::chrono)
 * 2. Calculate cycles-per-nanosecond ratio
 * 3. Cache the result for fast conversions
 * 4. Support manual override via OFI_NCCL_RDTSC_FREQ_MHZ environment variable
 *
 * Usage:
 *   auto& cal = nccl_ofi_rdtsc::calibration::instance();
 *   uint64_t ns = cal.cycles_to_ns(cycles);
 *
 * Environment variables:
 * - OFI_NCCL_RDTSC_FREQ_MHZ: Override detected frequency (in MHz)
 */

namespace nccl_ofi_rdtsc {

/**
 * @class calibration
 * @brief Singleton class for RDTSC frequency calibration
 *
 * This class manages the calibration of RDTSC frequency and provides
 * conversion functions between CPU cycles and nanoseconds. It uses the
 * singleton pattern to ensure calibration is performed only once.
 *
 * Thread safety: The singleton instance is thread-safe (C++11 guarantees).
 * Calibration is performed on first access and cached thereafter.
 */
class calibration {
public:
	/**
	 * @brief Get the singleton instance
	 *
	 * Returns the singleton calibration instance, performing calibration
	 * on first access. Thread-safe in C++11 and later.
	 *
	 * @return Reference to the singleton calibration instance
	 */
	static calibration& instance();

	/**
	 * @brief Get cycles per nanosecond
	 *
	 * Returns the calibrated ratio of CPU cycles to nanoseconds. This
	 * value is typically in the range of 0.5 to 5.0 for modern CPUs
	 * (corresponding to 0.5 GHz to 5.0 GHz).
	 *
	 * @return Cycles per nanosecond (e.g., 2.4 for a 2.4 GHz CPU)
	 */
	double get_cycles_per_ns() const { return cycles_per_ns_; }

	/**
	 * @brief Get nanoseconds per cycle
	 *
	 * Returns the inverse of cycles_per_ns, representing the time in
	 * nanoseconds for each CPU cycle. This is the reciprocal of the
	 * CPU frequency in GHz.
	 *
	 * @return Nanoseconds per cycle (e.g., 0.417 for a 2.4 GHz CPU)
	 */
	double get_ns_per_cycle() const { return ns_per_cycle_; }

	/**
	 * @brief Convert CPU cycles to nanoseconds
	 *
	 * Converts a cycle count to nanoseconds using the calibrated frequency.
	 * This is the primary conversion function for timing measurements.
	 *
	 * @param cycles Number of CPU cycles to convert
	 * @return Equivalent time in nanoseconds
	 *
	 * @note Conversion uses floating-point multiplication for accuracy.
	 *       For very large cycle counts (>2^53), precision may be reduced.
	 */
	uint64_t cycles_to_ns(uint64_t cycles) const {
		return static_cast<uint64_t>(cycles * ns_per_cycle_);
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
	 *
	 * @note Conversion uses floating-point multiplication for accuracy.
	 */
	uint64_t ns_to_cycles(uint64_t ns) const {
		return static_cast<uint64_t>(ns * cycles_per_ns_);
	}

	/**
	 * @brief Force recalibration of frequency
	 *
	 * Performs a new calibration measurement, updating the cached frequency
	 * values. This can be useful if CPU frequency scaling has occurred or
	 * if the initial calibration was inaccurate.
	 *
	 * @note Recalibration takes ~10-100ms depending on the calibration
	 *       duration. Use sparingly in performance-critical code.
	 */
	void recalibrate();

	/**
	 * @brief Check if calibration has been performed
	 *
	 * Returns true if the frequency has been calibrated (either automatically
	 * or via environment variable override).
	 *
	 * @return true if calibrated, false otherwise
	 */
	bool is_calibrated() const { return calibrated_; }

private:
	/**
	 * @brief Private constructor (singleton pattern)
	 *
	 * Performs initial calibration on construction. Checks for environment
	 * variable override before performing automatic calibration.
	 */
	calibration();

	/**
	 * @brief Perform frequency calibration
	 *
	 * Measures RDTSC frequency by comparing cycle counts against
	 * std::chrono::high_resolution_clock over a known time period.
	 * Updates cycles_per_ns_ and ns_per_cycle_ with the results.
	 *
	 * Calibration algorithm:
	 * 1. Record start time (chrono) and start cycles (RDTSC)
	 * 2. Wait for calibration period (default 10ms)
	 * 3. Record end time (chrono) and end cycles (RDTSC)
	 * 4. Calculate cycles_per_ns = (end_cycles - start_cycles) / (end_ns - start_ns)
	 *
	 * @note Uses rdtscp() for serialized reads to ensure accuracy.
	 */
	void calibrate_frequency();

	// Calibration results
	double cycles_per_ns_;   ///< CPU cycles per nanosecond (e.g., 2.4 for 2.4 GHz)
	double ns_per_cycle_;    ///< Nanoseconds per CPU cycle (reciprocal of cycles_per_ns_)
	bool calibrated_;        ///< True if calibration has been performed

	// Prevent copying and assignment
	calibration(const calibration&) = delete;
	calibration& operator=(const calibration&) = delete;
};

} // namespace nccl_ofi_rdtsc

#endif // NCCL_OFI_STATS_RDTSC_CALIBRATION_H
