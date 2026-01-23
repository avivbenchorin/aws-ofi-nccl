/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#include "stats/rdtsc_calibration.h"
#include "stats/rdtsc_platform.h"
#include "nccl_ofi_log.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>

namespace nccl_ofi_rdtsc {

// Singleton instance accessor
calibration& calibration::instance() {
	static calibration instance;
	return instance;
}

// Private constructor - performs initial calibration
calibration::calibration()
	: cycles_per_ns_(0.0), ns_per_cycle_(0.0), calibrated_(false)
{
	// Check for environment variable override first
	const char* freq_env = std::getenv("OFI_NCCL_RDTSC_FREQ_MHZ");
	if (freq_env != nullptr) {
		// Parse frequency from environment variable
		char* endptr = nullptr;
		double freq_mhz = std::strtod(freq_env, &endptr);
		
		if (endptr != freq_env && freq_mhz > 0.0 && freq_mhz < 10000.0) {
			// Valid frequency in MHz (0-10 GHz range)
			cycles_per_ns_ = freq_mhz / 1000.0;  // Convert MHz to GHz (cycles/ns)
			ns_per_cycle_ = 1.0 / cycles_per_ns_;
			calibrated_ = true;
			
			NCCL_OFI_INFO(NCCL_INIT | NCCL_NET,
			             "RDTSC frequency override: %.3f MHz (%.3f GHz) from OFI_NCCL_RDTSC_FREQ_MHZ",
			             freq_mhz, cycles_per_ns_);
			return;
		} else {
			NCCL_OFI_WARN("Invalid OFI_NCCL_RDTSC_FREQ_MHZ value: %s (expected 0-10000 MHz), performing auto-calibration",
			             freq_env);
		}
	}

#if defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
	// ARM64: Use hardware-provided frequency
	uint64_t freq_hz = get_frequency();
	if (freq_hz > 0) {
		cycles_per_ns_ = static_cast<double>(freq_hz) / 1000000000.0;
		ns_per_cycle_ = 1.0 / cycles_per_ns_;
		calibrated_ = true;
		
		NCCL_OFI_INFO(NCCL_INIT | NCCL_NET,
		             "RDTSC frequency from hardware: %lu Hz (%.3f GHz)",
		             freq_hz, cycles_per_ns_);
		return;
	}
#endif

	// Perform automatic calibration
	calibrate_frequency();
}

void calibration::calibrate_frequency() {
	// Calibration parameters
	const int calibration_ms = 10;  // Calibration duration in milliseconds
	const int num_samples = 3;      // Number of calibration samples to average
	
	double total_cycles_per_ns = 0.0;
	int valid_samples = 0;
	
	NCCL_OFI_TRACE(NCCL_INIT | NCCL_NET,
	              "Calibrating RDTSC frequency (platform: %s)...", NCCL_OFI_RDTSC_PLATFORM);
	
	for (int sample = 0; sample < num_samples; ++sample) {
		// Record start time and cycles
		auto start_time = std::chrono::high_resolution_clock::now();
		uint64_t start_cycles = rdtscp();  // Use serializing read
		
		// Wait for calibration period
		std::this_thread::sleep_for(std::chrono::milliseconds(calibration_ms));
		
		// Record end time and cycles
		uint64_t end_cycles = rdtscp();  // Use serializing read
		auto end_time = std::chrono::high_resolution_clock::now();
		
		// Calculate elapsed time in nanoseconds
		auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
			end_time - start_time).count();
		
		// Calculate elapsed cycles
		uint64_t elapsed_cycles = end_cycles - start_cycles;
		
		// Validate measurement
		if (elapsed_ns > 0 && elapsed_cycles > 0) {
			double sample_cycles_per_ns = static_cast<double>(elapsed_cycles) / 
			                               static_cast<double>(elapsed_ns);
			
			// Sanity check: frequency should be in reasonable range (0.1 GHz to 10 GHz)
			if (sample_cycles_per_ns >= 0.1 && sample_cycles_per_ns <= 10.0) {
				total_cycles_per_ns += sample_cycles_per_ns;
				valid_samples++;
				
				NCCL_OFI_TRACE(NCCL_INIT | NCCL_NET,
				              "Calibration sample %d: %lu cycles in %ld ns = %.3f GHz",
				              sample + 1, elapsed_cycles, elapsed_ns, sample_cycles_per_ns);
			} else {
				NCCL_OFI_WARN("Calibration sample %d out of range: %.3f GHz (expected 0.1-10.0 GHz)",
				             sample + 1, sample_cycles_per_ns);
			}
		} else {
			NCCL_OFI_WARN("Calibration sample %d invalid: %lu cycles, %ld ns",
			             sample + 1, elapsed_cycles, elapsed_ns);
		}
	}
	
	if (valid_samples > 0) {
		// Average the valid samples
		cycles_per_ns_ = total_cycles_per_ns / valid_samples;
		ns_per_cycle_ = 1.0 / cycles_per_ns_;
		calibrated_ = true;
		
		NCCL_OFI_INFO(NCCL_INIT | NCCL_NET,
		             "RDTSC calibration complete: %.3f GHz (%.3f ns/cycle) from %d samples",
		             cycles_per_ns_, ns_per_cycle_, valid_samples);
	} else {
		// Calibration failed - use a default value
		cycles_per_ns_ = 2.0;  // Assume 2 GHz as fallback
		ns_per_cycle_ = 0.5;
		calibrated_ = false;
		
		NCCL_OFI_WARN("RDTSC calibration failed, using default 2.0 GHz. "
		             "Set OFI_NCCL_RDTSC_FREQ_MHZ environment variable to override.");
	}
}

void calibration::recalibrate() {
	NCCL_OFI_INFO(NCCL_INIT | NCCL_NET, "Recalibrating RDTSC frequency...");
	calibrate_frequency();
}

} // namespace nccl_ofi_rdtsc
