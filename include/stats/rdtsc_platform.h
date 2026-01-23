/*
 * Copyright (c) 2025 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef NCCL_OFI_STATS_RDTSC_PLATFORM_H
#define NCCL_OFI_STATS_RDTSC_PLATFORM_H

/**
 * @file rdtsc_platform.h
 * @brief Platform detection and RDTSC implementation selection
 *
 * This header automatically detects the target architecture and includes
 * the appropriate RDTSC implementation. It defines NCCL_OFI_RDTSC_AVAILABLE
 * to indicate whether native RDTSC support is available.
 *
 * Supported platforms:
 * - x86_64 / AMD64: Uses RDTSC/RDTSCP instructions
 * - ARM64 / AArch64: Uses CNTVCT_EL0 generic timer
 * - Other: Falls back to std::chrono (higher overhead)
 *
 * Usage:
 *   #include "stats/rdtsc_platform.h"
 *   #if NCCL_OFI_RDTSC_AVAILABLE
 *     // Native RDTSC available
 *   #else
 *     // Using chrono fallback
 *   #endif
 */

// Platform detection and selection
#if defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64) || defined(_M_X64) || defined(_M_AMD64)
	// x86_64 / AMD64 architecture
	#include "rdtsc_x86_64.h"
	#define NCCL_OFI_RDTSC_AVAILABLE 1
	#define NCCL_OFI_RDTSC_PLATFORM "x86_64"

#elif defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
	// ARM64 / AArch64 architecture
	#include "rdtsc_arm64.h"
	#define NCCL_OFI_RDTSC_AVAILABLE 1
	#define NCCL_OFI_RDTSC_PLATFORM "arm64"

#else
	// Unsupported architecture - use chrono fallback
	#include "rdtsc_generic.h"
	#define NCCL_OFI_RDTSC_AVAILABLE 0
	#define NCCL_OFI_RDTSC_PLATFORM "generic"
	
	// Emit a warning during compilation
	#warning "RDTSC not available on this platform, using std::chrono fallback (higher overhead)"

#endif

/**
 * @def NCCL_OFI_RDTSC_AVAILABLE
 * @brief Indicates whether native RDTSC support is available
 *
 * Set to 1 if the platform has native RDTSC or equivalent support (x86_64, ARM64).
 * Set to 0 if using the std::chrono fallback (other architectures).
 */

/**
 * @def NCCL_OFI_RDTSC_PLATFORM
 * @brief String identifying the selected RDTSC implementation
 *
 * Possible values:
 * - "x86_64": Using RDTSC/RDTSCP instructions
 * - "arm64": Using CNTVCT_EL0 generic timer
 * - "generic": Using std::chrono fallback
 */

#endif // NCCL_OFI_STATS_RDTSC_PLATFORM_H
