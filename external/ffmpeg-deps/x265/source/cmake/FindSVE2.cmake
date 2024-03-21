include(FindPackageHandleStandardArgs)

# Check the version of SVE2 supported by the ARM CPU
if(APPLE)
    execute_process(COMMAND sysctl -a
                    COMMAND grep "hw.optional.sve2: 1"
                    OUTPUT_VARIABLE sve2_version
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    execute_process(COMMAND cat /proc/cpuinfo
                    COMMAND grep Features
                    COMMAND grep sve2
                    OUTPUT_VARIABLE sve2_version
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

if(sve2_version)
    set(CPU_HAS_SVE 1)
    set(CPU_HAS_SVE2 1)
endif()
