# CHECK_RDTSC
# -----------
# Check for RDTSC support and add configure option
#
# This macro checks whether the target architecture supports RDTSC or
# equivalent timing instructions and adds a configure option to enable/disable
# RDTSC-based timing for profiling.
#
# Supported architectures:
# - x86_64/AMD64: Uses RDTSC/RDTSCP instructions
# - ARM64/AArch64: Uses CNTVCT_EL0 generic timer
# - Other: Falls back to std::chrono (higher overhead)
#
# Configure options:
# --enable-rdtsc-timing=yes   Force enable (error if unsupported)
# --enable-rdtsc-timing=no    Force disable (use chrono)
# --enable-rdtsc-timing=auto  Auto-detect (default)
#
# Defines:
# ENABLE_RDTSC_TIMING - Preprocessor macro (1 if enabled)
# AM_CONDITIONAL ENABLE_RDTSC - Automake conditional for Makefiles
#
# Output variables:
# rdtsc_enabled - "yes" or "no"
# rdtsc_arch - "x86_64", "arm64", or "none"

AC_DEFUN([CHECK_RDTSC], [
    AC_ARG_ENABLE([rdtsc-timing],
        [AS_HELP_STRING([--enable-rdtsc-timing@<:@=yes|no|auto@:>@],
            [Enable RDTSC-based timing for profiling (default: auto)])],
        [enable_rdtsc=$enableval],
        [enable_rdtsc=auto])

    # Initialize variables
    rdtsc_enabled=no
    rdtsc_arch=none

    AS_IF([test "x$enable_rdtsc" != "xno"], [
        AC_MSG_CHECKING([for RDTSC support])
        
        # Detect architecture
        AS_CASE([$host_cpu],
            [x86_64|amd64|x86-64], [
                rdtsc_arch=x86_64
                rdtsc_arch_name="x86_64 (RDTSC/RDTSCP)"
            ],
            [aarch64|arm64], [
                rdtsc_arch=arm64
                rdtsc_arch_name="ARM64 (CNTVCT_EL0)"
            ],
            [
                rdtsc_arch=none
                rdtsc_arch_name="unsupported"
            ])
        
        # Check if architecture supports RDTSC
        AS_IF([test "x$rdtsc_arch" != "xnone"], [
            # Architecture supports RDTSC
            AC_MSG_RESULT([yes ($rdtsc_arch_name)])
            
            # Verify compiler can generate inline assembly
            AC_MSG_CHECKING([whether compiler supports inline assembly for RDTSC])
            AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
                #include <cstdint>
                static inline uint64_t test_rdtsc() {
                    #if defined(__x86_64__) || defined(__amd64__)
                        uint32_t lo, hi;
                        asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
                        return ((uint64_t)hi << 32) | lo;
                    #elif defined(__aarch64__) || defined(__arm64__)
                        uint64_t val;
                        asm volatile("mrs %0, cntvct_el0" : "=r"(val));
                        return val;
                    #else
                        return 0;
                    #endif
                }
            ]], [[
                uint64_t cycles = test_rdtsc();
                return (cycles > 0) ? 0 : 1;
            ]])],
            [
                AC_MSG_RESULT([yes])
                rdtsc_enabled=yes
            ],
            [
                AC_MSG_RESULT([no])
                AS_IF([test "x$enable_rdtsc" = "xyes"], [
                    AC_MSG_ERROR([RDTSC requested but compiler does not support required inline assembly])
                ])
                rdtsc_enabled=no
            ])
        ], [
            # Architecture does not support RDTSC
            AC_MSG_RESULT([no (architecture: $host_cpu)])
            AS_IF([test "x$enable_rdtsc" = "xyes"], [
                AC_MSG_ERROR([RDTSC requested but not supported on $host_cpu architecture])
            ])
            rdtsc_enabled=no
        ])
    ], [
        # User explicitly disabled RDTSC
        AC_MSG_CHECKING([for RDTSC support])
        AC_MSG_RESULT([disabled by user])
        rdtsc_enabled=no
    ])
    
    # Define preprocessor macro if enabled
    AS_IF([test "x$rdtsc_enabled" = "xyes"], [
        AC_DEFINE([ENABLE_RDTSC_TIMING], [1], 
                 [Define to 1 if RDTSC timing is enabled])
    ])
    
    # Set Automake conditional
    AM_CONDITIONAL([ENABLE_RDTSC], [test "x$rdtsc_enabled" = "xyes"])
    
    # Export variables for use in configure summary
    AC_SUBST([rdtsc_enabled])
    AC_SUBST([rdtsc_arch])
])
