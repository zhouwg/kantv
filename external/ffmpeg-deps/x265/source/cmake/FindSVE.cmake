include(FindPackageHandleStandardArgs)

# Check the version of SVE supported by the ARM CPU
if(APPLE)
    execute_process(COMMAND sysctl -a
                    COMMAND grep "hw.optional.sve: 1"
                    OUTPUT_VARIABLE sve_version
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    execute_process(COMMAND cat /proc/cpuinfo
                    COMMAND grep Features
                    COMMAND grep -e "sve$" -e "sve[[:space:]]"
                    OUTPUT_VARIABLE sve_version
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

if(sve_version)
    set(CPU_HAS_SVE 1)
endif()
