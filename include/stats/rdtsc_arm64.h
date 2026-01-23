/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef NCCL_OFI_STATS_RDTSC_ARM64_H
#define NCCL_OFI_STATS_RDTSC_ARM64_H

#include <cstdint>

/**
 * @file rdtsc_arm64.h
 * @brief ARM64-specific timing implementation using generic timer
 *
 * This file provides low-overhead timing primitives using ARM64's generic
 * timer system registers. The virtual counter (CNTVCT_EL0) provides a
 * consistent view of time across all cores and is suitable for performance
 * measurement.
 *
 * Key characteristics:
 * - rdtsc(): Non-serializing counter read
 * - rdtscp(): Counter read with ISB barrier
 * - serialize(): Full instruction synchronization barrier
 * - get_frequency(): Hardware-provided counter frequency
 *
 * Usage notes:
 * - Counter frequency is fixed and available via CNTFRQ_EL0
 * - Virtual counter is synchronized across all cores
 * - ISB (Instruction Synchronization Barrier) ensures ordering
 * - Supported on ARMv8+ (AWS Graviton2/3, Apple Silicon, etc.)
 */

namespace nccl_ofi_rdtsc {

/**
 * @brief Read the virtual counter (non-serializing)
 *
 * Reads the ARM64 virtual counter (CNTVCT_EL0) without serialization. This
 * provides a consistent view of time across all CPU cores with minimal
 * overhead.
 *
 * @return Current counter value in hardware-defined ticks
 *
 * @note The counter increments at a fixed frequency available via
 *       get_frequency(). Use memory barriers to prevent reordering.
 */
static inline uint64_t rdtsc() {
	uint64_t val;
	asm volatile("mrs %0, cntvct_el0" : "=r"(val));
	return val;
}

/**
 * @brief Read the virtual counter with serialization (ISB)
 *
 * Reads the ARM64 virtual counter with an instruction synchronization
 * barrier (ISB) to ensure all previous instructions have completed before
 * reading the counter.
 *
 * @return Current counter value in hardware-defined ticks
 *
 * @note ISB ensures that all context-changing operations before the ISB
 *       have completed before any instructions after the ISB execute.
 */
static inline uint64_t rdtscp() {
	uint64_t val;
	asm volatile("isb; mrs %0, cntvct_el0" : "=r"(val));
	return val;
}

/**
 * @brief Serialize instruction execution (ISB)
 *
 * Executes an instruction synchronization barrier (ISB) to ensure that all
 * instructions before the ISB have completed and all context changes have
 * taken effect before any instructions after the ISB execute.
 *
 * @note ISB is lighter weight than x86_64's CPUID but still provides
 *       strong ordering guarantees. Typical overhead is 10-20 cycles.
 */
static inline void serialize() {
	asm volatile("isb" : : : "memory");
}

/**
 * @brief Get the counter frequency in Hz
 *
 * Reads the ARM64 counter frequency register (CNTFRQ_EL0) which provides
 * the fixed frequency at which the virtual counter increments. This value
 * is set by firmware and is consistent across all cores.
 *
 * @return Counter frequency in Hz (typically 25MHz on AWS Graviton,
 *         24MHz on Apple Silicon, varies by platform)
 *
 * @note This value is constant for the lifetime of the system and can be
 *       cached. Use this to convert counter ticks to nanoseconds.
 */
static inline uint64_t get_frequency() {
	uint64_t freq;
	asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
	return freq;
}

} // namespace nccl_ofi_rdtsc

#endif // NCCL_OFI_STATS_RDTSC_ARM64_H
