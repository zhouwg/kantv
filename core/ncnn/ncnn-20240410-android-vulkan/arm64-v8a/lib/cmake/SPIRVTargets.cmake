
        message(WARNING "Using `SPIRVTargets.cmake` is deprecated: use `find_package(glslang)` to find glslang CMake targets.")

        if (NOT TARGET glslang::SPIRV)
            include("/home/runner/work/ncnn/ncnn/build-aarch64/install/lib/cmake/glslang/glslang-targets.cmake")
        endif()

        add_library(SPIRV ALIAS glslang::SPIRV)
    