/*
 * Copyright (c) 2024 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef NCCL_OFI_STATS_RDTSCP_CLOCK_H
#define NCCL_OFI_STATS_RDTSCP_CLOCK_H

#include <chrono>
#include <cstdint>

/*
 * RDTSCP-based high-precision clock for performance profiling
 * 
 * This clock uses the x86/x86_64 RDTSCP instruction to provide lower-overhead
 * timing measurements compared to std::chrono::steady_clock. The RDTSCP
 * instruction reads the Time Stamp Counter (TSC) with serialization properties,
 * making it suitable for accurate timing measurements.
 * 
 * Platform Requirements:
 * - x86 or x86_64 architecture
 * - CPU with RDTSCP instruction support (Intel Nehalem+, AMD Phenom+)
 * - Invariant TSC recommended for best accuracy
 * 
 * On non-x86 platforms, this header provides a fallback to steady_clock
 * to maintain portability.
 */

// Platform detection: Check for x86/x86_64 architecture
#if defined(__x86_64__) || defined(__i386__)
    #define RDTSCP_AVAILABLE 1
#else
    #define RDTSCP_AVAILABLE 0
#endif

#if RDTSCP_AVAILABLE

// Forward declaration of rdtscp_clock class
// Full implementation will be added in subsequent tasks
class rdtscp_clock {
public:
    // Chrono-compatible type aliases
    using rep = uint64_t;
    using period = std::nano;
    using duration = std::chrono::nanoseconds;
    using time_point = std::chrono::time_point<rdtscp_clock, duration>;
    
    static constexpr bool is_steady = true;
    
    // Public interface (to be implemented in subsequent tasks)
    static time_point now() noexcept;
    static void initialize();
    static double get_cycles_per_ns();
    static bool read_tsc_freq_from_sysfs(double& cycles_per_ns);
    static bool read_tsc_freq_from_cpuinfo(double& cycles_per_ns);
    
    /*
     * Read Time Stamp Counter using RDTSCP instruction
     * 
     * The RDTSCP instruction:
     * - Reads the 64-bit TSC value (returned in EDX:EAX)
     * - Reads the processor ID (returned in ECX via aux parameter)
     * - Serializes with respect to all prior instructions
     * - Does not serialize with respect to subsequent instructions
     * 
     * This function tries to use the compiler intrinsic __rdtscp if available,
     * otherwise falls back to inline assembly.
     * 
     * @param aux Pointer to store processor ID (IA32_TSC_AUX MSR value)
     * @return 64-bit TSC value
     */
    static inline uint64_t read_tsc(uint32_t* aux) noexcept {
#if defined(__GNUC__) || defined(__clang__)
        // Try to use GCC/Clang intrinsic if available
        #ifdef __RDTSCP__
            return __rdtscp(aux);
        #else
            // Fallback to inline assembly
            uint32_t lo, hi;
            __asm__ __volatile__(
                "rdtscp"
                : "=a"(lo), "=d"(hi), "=c"(*aux)
                :
                : "memory"
            );
            return ((uint64_t)hi << 32) | lo;
        #endif
#else
        #error "Compiler not supported for rdtscp (requires GCC or Clang)"
#endif
    }
    
private:
    static double cycles_per_ns_;
    static bool initialized_;
    
    static void calibrate_tsc_frequency();
};

#else // !RDTSCP_AVAILABLE

// Non-x86 platform: Provide fallback to steady_clock
#warning "rdtscp_clock is not available on this platform (requires x86/x86_64). Falling back to std::chrono::steady_clock."

// Alias rdtscp_clock to steady_clock for compatibility
using rdtscp_clock = std::chrono::steady_clock;

#endif // RDTSCP_AVAILABLE

#endif // NCCL_OFI_STATS_RDTSCP_CLOCK_H
