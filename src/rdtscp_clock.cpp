/*
 * Copyright (c) 2024 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#include "stats/rdtscp_clock.h"

#if RDTSCP_AVAILABLE

#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <atomic>

#if defined(__GNUC__) || defined(__clang__)
#include <cpuid.h>
#endif

// Static member definitions for rdtscp_clock
double rdtscp_clock::cycles_per_ns_ = 0.0;
bool rdtscp_clock::initialized_ = false;

/*
 * Read TSC frequency from CPUID leaf 0x15
 * 
 * CPUID leaf 0x15 provides Time Stamp Counter and Nominal Core Crystal Clock Information.
 * This is the most accurate method for modern Intel processors (Skylake and newer).
 * 
 * Formula: TSC Frequency = (ECX * EBX) / EAX
 * Where:
 *   EAX = Denominator of the TSC/"core crystal clock" ratio
 *   EBX = Numerator of the TSC/"core crystal clock" ratio
 *   ECX = Core crystal clock frequency in Hz
 * 
 * Important: On some Intel SoCs (Skylake, Kaby Lake), ECX may be 0, meaning
 * the crystal clock frequency is not enumerated. In this case, fall back to
 * CPUID leaf 0x16.
 * 
 * @param cycles_per_ns Output parameter to store the calculated cycles per nanosecond
 * @return true if frequency was successfully read, false otherwise
 */
bool rdtscp_clock::read_tsc_freq_from_cpuid_0x15(double& cycles_per_ns)
{
#if defined(__GNUC__) || defined(__clang__)
    uint32_t eax, ebx, ecx, edx;
    
    // Check if CPUID leaf 0x15 is supported
    __cpuid(0, eax, ebx, ecx, edx);
    if (eax < 0x15) {
        return false;  // Leaf 0x15 not supported
    }
    
    // Query leaf 0x15: Time Stamp Counter and Nominal Core Crystal Clock Information
    __cpuid(0x15, eax, ebx, ecx, edx);
    
    // Check if all required values are non-zero
    if (eax == 0 || ebx == 0 || ecx == 0) {
        // ECX = 0 is common on some Intel SoCs (Skylake, Kaby Lake)
        // EAX or EBX = 0 means the feature is not supported
        return false;
    }
    
    // Calculate TSC frequency in Hz
    // TSC_freq = (crystal_freq * ebx) / eax
    uint64_t tsc_freq_hz = (static_cast<uint64_t>(ecx) * static_cast<uint64_t>(ebx)) / static_cast<uint64_t>(eax);
    
    // Convert Hz to cycles per nanosecond
    // cycles_per_ns = tsc_freq_hz / 1,000,000,000
    cycles_per_ns = static_cast<double>(tsc_freq_hz) / 1000000000.0;
    
    return true;
#else
    return false;
#endif
}

/*
 * Read TSC frequency from CPUID leaf 0x16
 * 
 * CPUID leaf 0x16 provides Processor Frequency Information.
 * This is a fallback method when leaf 0x15 is not available or returns ECX=0.
 * 
 * Returns:
 *   EAX[15:0] = Processor Base Frequency (in MHz)
 *   EBX[15:0] = Maximum Frequency (in MHz)
 *   ECX[15:0] = Bus (Reference) Frequency (in MHz)
 * 
 * On systems with invariant TSC, the processor base frequency is typically
 * equal to the TSC frequency.
 * 
 * @param cycles_per_ns Output parameter to store the calculated cycles per nanosecond
 * @return true if frequency was successfully read, false otherwise
 */
bool rdtscp_clock::read_tsc_freq_from_cpuid_0x16(double& cycles_per_ns)
{
#if defined(__GNUC__) || defined(__clang__)
    uint32_t eax, ebx, ecx, edx;
    
    // Check if CPUID leaf 0x16 is supported
    __cpuid(0, eax, ebx, ecx, edx);
    if (eax < 0x16) {
        return false;  // Leaf 0x16 not supported
    }
    
    // Query leaf 0x16: Processor Frequency Information
    __cpuid(0x16, eax, ebx, ecx, edx);
    
    // Extract processor base frequency from EAX[15:0] (in MHz)
    uint32_t base_freq_mhz = eax & 0xFFFF;
    
    if (base_freq_mhz == 0) {
        return false;  // Frequency not enumerated
    }
    
    // Convert MHz to cycles per nanosecond
    // cycles_per_ns = base_freq_mhz / 1000
    cycles_per_ns = static_cast<double>(base_freq_mhz) / 1000.0;
    
    return true;
#else
    return false;
#endif
}

/*
 * Read TSC frequency from sysfs files
 * 
 * This function attempts to read the TSC frequency from Linux sysfs files:
 * 1. First tries /sys/devices/system/cpu/cpu0/tsc_freq_khz (direct TSC frequency)
 * 2. Falls back to /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq (CPU frequency)
 * 
 * Note: The cpufreq file may not reflect actual TSC frequency on systems with
 * invariant TSC, but it provides a reasonable approximation.
 * 
 * @param cycles_per_ns Output parameter to store the calculated cycles per nanosecond
 * @return true if frequency was successfully read, false otherwise
 */
bool rdtscp_clock::read_tsc_freq_from_sysfs(double& cycles_per_ns)
{
    // Try reading TSC frequency directly (preferred method)
    std::ifstream tsc_freq_file("/sys/devices/system/cpu/cpu0/tsc_freq_khz");
    if (tsc_freq_file.is_open()) {
        uint64_t freq_khz = 0;
        tsc_freq_file >> freq_khz;
        tsc_freq_file.close();
        
        if (freq_khz > 0) {
            // Convert kHz to cycles per nanosecond
            // freq_khz is in kHz (thousands of cycles per second)
            // cycles_per_ns = (freq_khz * 1000) / 1,000,000,000
            //               = freq_khz / 1,000,000
            cycles_per_ns = static_cast<double>(freq_khz) / 1000000.0;
            return true;
        }
    }
    
    // Fallback: Try reading CPU frequency from cpufreq
    std::ifstream cpufreq_file("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
    if (cpufreq_file.is_open()) {
        uint64_t freq_khz = 0;
        cpufreq_file >> freq_khz;
        cpufreq_file.close();
        
        if (freq_khz > 0) {
            // Convert kHz to cycles per nanosecond
            cycles_per_ns = static_cast<double>(freq_khz) / 1000000.0;
            return true;
        }
    }
    
    // Both methods failed
    return false;
}

/*
 * Read TSC frequency from /proc/cpuinfo
 * 
 * This function parses /proc/cpuinfo to extract the "cpu MHz" value.
 * 
 * Note: On modern CPUs with invariant TSC, the reported CPU MHz may not
 * reflect the actual TSC frequency due to turbo boost and power saving modes.
 * However, on systems with invariant TSC, the TSC frequency is typically
 * constant and may match the base CPU frequency.
 * 
 * @param cycles_per_ns Output parameter to store the calculated cycles per nanosecond
 * @return true if frequency was successfully read, false otherwise
 */
bool rdtscp_clock::read_tsc_freq_from_cpuinfo(double& cycles_per_ns)
{
    std::ifstream cpuinfo_file("/proc/cpuinfo");
    if (!cpuinfo_file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(cpuinfo_file, line)) {
        // Look for "cpu MHz" line
        // Format: "cpu MHz		: 2400.000"
        if (line.find("cpu MHz") != std::string::npos) {
            // Find the colon separator
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                // Extract the value after the colon
                std::string value_str = line.substr(colon_pos + 1);
                
                // Parse the MHz value
                try {
                    double freq_mhz = std::stod(value_str);
                    if (freq_mhz > 0) {
                        // Convert MHz to cycles per nanosecond
                        // freq_mhz is in MHz (millions of cycles per second)
                        // cycles_per_ns = freq_mhz / 1000
                        cycles_per_ns = freq_mhz / 1000.0;
                        cpuinfo_file.close();
                        return true;
                    }
                } catch (...) {
                    // Parsing failed, continue to next line
                    continue;
                }
            }
        }
    }
    
    cpuinfo_file.close();
    return false;
}

/*
 * Calibrate TSC frequency using steady_clock as reference
 * 
 * This function measures the TSC frequency by comparing rdtscp cycle counts
 * against std::chrono::steady_clock over a 100ms interval. This serves as
 * a fallback when sysfs and /proc/cpuinfo methods are unavailable.
 * 
 * The calibration process:
 * 1. Capture start time with both steady_clock and rdtscp
 * 2. Sleep for 100ms to allow sufficient cycles to accumulate
 * 3. Capture end time with both clocks
 * 4. Calculate cycles_per_ns = elapsed_cycles / elapsed_nanoseconds
 * 
 * Note: This method is less accurate than reading from system files because:
 * - Sleep duration may not be exactly 100ms
 * - Thread scheduling can introduce jitter
 * - However, it provides a reasonable approximation for most use cases
 * 
 * @param cycles_per_ns Output parameter to store the calculated cycles per nanosecond
 */
void rdtscp_clock::calibrate_tsc_frequency()
{
    // Capture start time with both clocks
    auto start_steady = std::chrono::steady_clock::now();
    uint32_t aux_start;
    uint64_t start_tsc = read_tsc(&aux_start);
    
    // Sleep for 100ms to accumulate sufficient cycles for accurate measurement
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Capture end time with both clocks
    auto end_steady = std::chrono::steady_clock::now();
    uint32_t aux_end;
    uint64_t end_tsc = read_tsc(&aux_end);
    
    // Calculate elapsed time in nanoseconds using steady_clock
    auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end_steady - start_steady).count();
    
    // Calculate elapsed cycles using rdtscp
    uint64_t elapsed_cycles = end_tsc - start_tsc;
    
    // Calculate cycles per nanosecond
    // This is the conversion factor we'll use in now() to convert TSC cycles to nanoseconds
    cycles_per_ns_ = static_cast<double>(elapsed_cycles) / static_cast<double>(elapsed_ns);
}

/*
 * Detect if the CPU supports invariant TSC
 * 
 * Invariant TSC is a feature where the Time Stamp Counter runs at a constant rate
 * regardless of CPU frequency changes due to power saving modes, turbo boost, etc.
 * This is critical for accurate timing measurements on modern CPUs.
 * 
 * Detection method:
 * - Use CPUID instruction with leaf 0x80000007 (Advanced Power Management)
 * - Check bit 8 of EDX register (InvariantTSC bit)
 * 
 * If invariant TSC is not available, timing measurements may be inaccurate when
 * CPU frequency changes occur.
 * 
 * @return true if CPU supports invariant TSC, false otherwise
 */
bool rdtscp_clock::has_invariant_tsc()
{
#if defined(__GNUC__) || defined(__clang__)
    uint32_t eax, ebx, ecx, edx;
    
    // First check if extended CPUID leaf 0x80000007 is supported
    // Query the maximum extended function number
    __cpuid(0x80000000, eax, ebx, ecx, edx);
    
    // If the maximum extended function is less than 0x80000007, the feature is not supported
    if (eax < 0x80000007) {
        return false;
    }
    
    // Query Advanced Power Management information (leaf 0x80000007)
    __cpuid(0x80000007, eax, ebx, ecx, edx);
    
    // Check bit 8 of EDX for Invariant TSC support
    // Bit 8: InvariantTSC - TSC rate is invariant across P-states, C-states, and frequency changes
    bool invariant_tsc = (edx & (1 << 8)) != 0;
    
    if (!invariant_tsc) {
        std::cerr << "Warning: CPU does not support invariant TSC. "
                  << "RDTSCP timing measurements may be inaccurate when CPU frequency changes occur "
                  << "(e.g., due to turbo boost or power saving modes)." << std::endl;
    }
    
    return invariant_tsc;
#else
    #error "Compiler not supported for CPUID instruction (requires GCC or Clang)"
#endif
}

/*
 * Get the calibrated cycles per nanosecond conversion factor
 * 
 * This accessor method returns the cycles_per_ns_ value that was determined
 * during initialization. This is useful for debugging and validation.
 * 
 * @return The cycles per nanosecond conversion factor
 */
double rdtscp_clock::get_cycles_per_ns()
{
    return cycles_per_ns_;
}

/*
 * Initialize the rdtscp_clock by calibrating TSC frequency
 *
 * This function must be called once at program startup before using rdtscp_clock.
 * It performs the following steps in priority order:
 *
 * 1. Check for invariant TSC support (logs warning if not available)
 * 2. Try CPUID leaf 0x15 (most accurate for modern Intel CPUs)
 * 3. Try CPUID leaf 0x16 (fallback for Intel CPUs)
 * 4. Try reading from sysfs files
 * 5. Try reading from /proc/cpuinfo
 * 6. Fall back to calibration against steady_clock
 * 7. Store the result in cycles_per_ns_ static member
 * 8. Set initialized_ flag to true
 * 9. Log which method succeeded
 *
 * The function is idempotent - calling it multiple times is safe (subsequent
 * calls will be ignored if already initialized).
 */
void rdtscp_clock::initialize()
{
    // If already initialized, skip
    if (initialized_) {
        return;
    }

    // Step 1: Check for invariant TSC support
    // This logs a warning if invariant TSC is not available
    bool invariant_tsc = has_invariant_tsc();

    // Step 2: Try CPUID leaf 0x15 (most accurate for modern Intel CPUs)
    if (read_tsc_freq_from_cpuid_0x15(cycles_per_ns_)) {
        std::cout << "rdtscp_clock: Initialized using CPUID leaf 0x15 (TSC/Crystal Clock). "
                  << "Cycles per nanosecond: " << cycles_per_ns_
                  << " (Invariant TSC: " << (invariant_tsc ? "yes" : "no") << ")"
                  << std::endl;
        initialized_ = true;
        return;
    }

    // Step 3: Try CPUID leaf 0x16 (fallback for Intel CPUs)
    if (read_tsc_freq_from_cpuid_0x16(cycles_per_ns_)) {
        std::cout << "rdtscp_clock: Initialized using CPUID leaf 0x16 (Processor Frequency). "
                  << "Cycles per nanosecond: " << cycles_per_ns_
                  << " (Invariant TSC: " << (invariant_tsc ? "yes" : "no") << ")"
                  << std::endl;
        initialized_ = true;
        return;
    }

    // Step 4: Try reading from sysfs
    if (read_tsc_freq_from_sysfs(cycles_per_ns_)) {
        std::cout << "rdtscp_clock: Initialized using sysfs frequency detection. "
                  << "Cycles per nanosecond: " << cycles_per_ns_
                  << " (Invariant TSC: " << (invariant_tsc ? "yes" : "no") << ")"
                  << std::endl;
        initialized_ = true;
        return;
    }

    // Step 5: Try reading from /proc/cpuinfo
    if (read_tsc_freq_from_cpuinfo(cycles_per_ns_)) {
        std::cout << "rdtscp_clock: Initialized using /proc/cpuinfo frequency detection. "
                  << "Cycles per nanosecond: " << cycles_per_ns_
                  << " (Invariant TSC: " << (invariant_tsc ? "yes" : "no") << ")"
                  << std::endl;
        initialized_ = true;
        return;
    }

    // Step 6: Fall back to calibration against steady_clock
    std::cout << "rdtscp_clock: All hardware detection methods failed. "
              << "Falling back to steady_clock calibration..." << std::endl;

    calibrate_tsc_frequency();

    std::cout << "rdtscp_clock: Initialized using steady_clock calibration. "
              << "Cycles per nanosecond: " << cycles_per_ns_
              << " (Invariant TSC: " << (invariant_tsc ? "yes" : "no") << ")"
              << std::endl;

    // Step 7 & 8: Store result and set initialized flag
    initialized_ = true;
}

/*
 * Get current time point using RDTSCP instruction
 * 
 * This function reads the Time Stamp Counter using rdtscp and converts
 * the cycle count to nanoseconds using the calibrated cycles_per_ns_ factor.
 * 
 * Memory Ordering and Barriers:
 * ==============================
 * The rdtscp instruction has specific serialization properties that affect
 * how we need to handle memory barriers:
 * 
 * 1. RDTSCP Serialization Properties (from Intel SDM):
 *    - Waits for all prior instructions to complete before reading TSC
 *    - Does NOT wait for subsequent instructions (non-serializing forward)
 *    - Reads TSC atomically (64-bit read is atomic on x86-64)
 * 
 * 2. Compiler Barriers in read_tsc():
 *    - The inline assembly uses "memory" clobber: __asm__ __volatile__("rdtscp" : ... : : "memory")
 *    - This acts as a compiler barrier, preventing the compiler from reordering
 *      memory operations across the rdtscp instruction
 *    - This is SUFFICIENT for our use case because:
 *      a) We want to measure code that executes BEFORE the timer read
 *      b) rdtscp already waits for prior instructions to complete (hardware serialization)
 *      c) The "memory" clobber prevents compiler reordering
 * 
 * 3. No Additional Barrier Needed After rdtscp:
 *    - The std::atomic_signal_fence below is REDUNDANT and can be removed
 *    - We don't need to prevent subsequent instructions from executing early
 *    - The timer_histogram::start_timer() and stop_timer() already have
 *      asm volatile barriers that provide sufficient ordering guarantees
 * 
 * 4. Comparison with timer_histogram barriers:
 *    - start_timer(): barrier AFTER clock::now() prevents code motion into timed region
 *    - stop_timer(): barrier BEFORE clock::now() prevents code motion out of timed region
 *    - These barriers work correctly with rdtscp's serialization properties
 * 
 * Conclusion:
 * ===========
 * The existing memory barriers are appropriate and sufficient:
 * - rdtscp's built-in serialization handles prior instruction completion
 * - The "memory" clobber in read_tsc() prevents compiler reordering
 * - The timer_histogram barriers provide the necessary ordering for timing measurements
 * - No additional barriers are needed in now()
 * 
 * @return time_point representing the current time in nanoseconds
 */
rdtscp_clock::time_point rdtscp_clock::now() noexcept
{
    // Read the Time Stamp Counter
    // The read_tsc() function includes a compiler barrier via "memory" clobber
    // in the inline assembly, which is sufficient for our needs
    uint32_t aux;
    uint64_t cycles = read_tsc(&aux);
    
    // Note: No additional barrier needed here
    // The rdtscp instruction's serialization properties combined with the
    // "memory" clobber in read_tsc() provide sufficient ordering guarantees
    // for accurate timing measurements when used with timer_histogram
    
    // Convert cycles to nanoseconds using the calibrated conversion factor
    // Note: We must ensure initialize() has been called before using now()
    uint64_t ns = static_cast<uint64_t>(cycles / cycles_per_ns_);
    
    return time_point(duration(ns));
}

#endif // RDTSCP_AVAILABLE
