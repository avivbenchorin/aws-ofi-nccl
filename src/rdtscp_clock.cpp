/*
 * Copyright (c) 2024 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#include "stats/rdtscp_clock.h"

#if RDTSCP_AVAILABLE

#include <fstream>
#include <string>

// Static member definitions for rdtscp_clock
double rdtscp_clock::cycles_per_ns_ = 0.0;
bool rdtscp_clock::initialized_ = false;

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

#endif // RDTSCP_AVAILABLE

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
