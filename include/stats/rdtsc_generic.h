/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef NCCL_OFI_STATS_RDTSC_GENERIC_H
#define NCCL_OFI_STATS_RDTSC_GENERIC_H

#include <cstdint>
#include <chrono>

/**
 * @file rdtsc_generic.h
 * @brief Generic fallback timing implementation using std::chrono
 *
 * This file provides a fallback implementation for platforms that don't
 * support native RDTSC or equivalent instructions. It uses std::chrono
 * to provide timing functionality with the same API as the platform-specific
 * implementations.
 *
 * Key characteristics:
 * - Higher overhead than native RDTSC (~100-200ns vs ~10-30ns)
 * - Portable across all platforms
 * - Uses std::chrono::steady_clock for monotonic timing
 * - Automatically selected on unsupported architectures
 *
 * Usage notes:
 * - This is a compatibility layer, not optimized for performance
 * - Consider using platform-specific timing if available
 * - Overhead may be significant for fine-grained profiling
 */

namespace nccl_ofi_rdtsc {

/**
 * @brief Read current time using std::chrono (non-serializing)
 *
 * Returns the current time in nanoseconds using std::chrono::steady_clock.
 * This provides a portable but higher-overhead alternative to RDTSC.
 *
 * @return Current time in nanoseconds since an unspecified epoch
 *
 * @warning This fallback has significantly higher overhead (~100-200ns)
 *          compared to native RDTSC implementations (~10-30ns). Use
 *          platform-specific implementations when available.
 */
static inline uint64_t rdtsc() {
	auto now = std::chrono::steady_clock::now();
	auto duration = now.time_since_epoch();
	return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

/**
 * @brief Read current time using std::chrono (serializing)
 *
 * Returns the current time in nanoseconds with a memory barrier to prevent
 * reordering. Functionally equivalent to rdtsc() in this implementation
 * since std::chrono calls are already serializing.
 *
 * @return Current time in nanoseconds since an unspecified epoch
 *
 * @note In this fallback implementation, rdtscp() is identical to rdtsc()
 *       since std::chrono operations are inherently serializing.
 */
static inline uint64_t rdtscp() {
	asm volatile("" : : : "memory");  // Compiler barrier
	auto now = std::chrono::steady_clock::now();
	auto duration = now.time_since_epoch();
	asm volatile("" : : : "memory");  // Compiler barrier
	return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

/**
 * @brief Serialize instruction execution (compiler barrier)
 *
 * Provides a compiler-level memory barrier to prevent instruction reordering.
 * This is a lightweight operation that only affects compiler optimizations,
 * not CPU-level reordering.
 *
 * @note This is a compiler barrier only. For true CPU-level serialization,
 *       use platform-specific implementations.
 */
static inline void serialize() {
	asm volatile("" : : : "memory");
}

/**
 * @brief Get the "frequency" for chrono-based timing
 *
 * Returns 1,000,000,000 (1 GHz) since the chrono implementation already
 * returns nanoseconds. This maintains API compatibility with ARM64's
 * get_frequency() function.
 *
 * @return 1,000,000,000 (nanoseconds are the native unit)
 *
 * @note This is a dummy value for API compatibility. The chrono
 *       implementation already returns nanoseconds, so no conversion
 *       is needed.
 */
static inline uint64_t get_frequency() {
	return 1000000000ULL;  // 1 GHz (nanoseconds)
}

} // namespace nccl_ofi_rdtsc

#endif // NCCL_OFI_STATS_RDTSC_GENERIC_H
