/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef NCCL_OFI_STATS_RDTSC_X86_64_H
#define NCCL_OFI_STATS_RDTSC_X86_64_H

#include <cstdint>

/**
 * @file rdtsc_x86_64.h
 * @brief x86_64-specific RDTSC (Read Time-Stamp Counter) implementation
 *
 * This file provides low-overhead timing primitives using the x86_64 RDTSC
 * instruction for high-precision cycle counting. RDTSC reads the processor's
 * time-stamp counter, which increments at a constant rate on modern CPUs
 * (invariant TSC).
 *
 * Key characteristics:
 * - rdtsc(): Non-serializing read (~10-20 cycles overhead)
 * - rdtscp(): Serializing read (~20-30 cycles overhead)
 * - serialize(): Full memory barrier using CPUID
 *
 * Usage notes:
 * - Use rdtsc() for start timing (minimal overhead)
 * - Use rdtscp() for stop timing (ensures completion)
 * - Requires invariant TSC support (Intel Nehalem+, AMD K10+)
 * - TSC may not be synchronized across CPU cores on older systems
 */

namespace nccl_ofi_rdtsc {

/**
 * @brief Read the Time-Stamp Counter (non-serializing)
 *
 * Reads the processor's time-stamp counter without serialization. This
 * instruction may be reordered with respect to other instructions, providing
 * minimal overhead but potentially less accurate timing if not used with
 * memory barriers.
 *
 * @return Current TSC value in CPU cycles
 *
 * @note This is the fastest RDTSC variant but may be reordered by the CPU.
 *       Use memory barriers (asm volatile) to prevent reordering.
 */
static inline uint64_t rdtsc() {
	uint32_t lo, hi;
	asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
	return ((uint64_t)hi << 32) | lo;
}

/**
 * @brief Read the Time-Stamp Counter with serialization (RDTSCP)
 *
 * Reads the processor's time-stamp counter with serialization. RDTSCP waits
 * until all previous instructions have executed before reading the counter,
 * ensuring accurate timing of completed operations.
 *
 * @return Current TSC value in CPU cycles
 *
 * @note This is slower than rdtsc() but provides more accurate timing by
 *       ensuring all previous instructions have completed. The auxiliary
 *       counter (ECX) is read but discarded.
 */
static inline uint64_t rdtscp() {
	uint32_t lo, hi, aux;
	asm volatile("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux));
	return ((uint64_t)hi << 32) | lo;
}

/**
 * @brief Serialize instruction execution (CPUID barrier)
 *
 * Executes CPUID instruction to create a full serialization barrier. This
 * ensures that all previous instructions have completed and no subsequent
 * instructions begin execution until CPUID completes.
 *
 * @note This is the most heavyweight serialization method. Use sparingly
 *       as it has significant overhead (~100+ cycles). For most timing
 *       purposes, rdtscp() provides sufficient serialization.
 */
static inline void serialize() {
	asm volatile("cpuid" : : : "rax", "rbx", "rcx", "rdx", "memory");
}

} // namespace nccl_ofi_rdtsc

#endif // NCCL_OFI_STATS_RDTSC_X86_64_H
