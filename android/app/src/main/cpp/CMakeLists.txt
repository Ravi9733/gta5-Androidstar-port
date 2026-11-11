cmake_minimum_required(VERSION 3.22.1)
project(gta5_android LANGUAGES C CXX)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(PORT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../")
set(PORT_SOURCE_DIR "${PORT_ROOT}/app/Source")
set(PORT_SCRIPT_DIR "${PORT_ROOT}/app/Scripts")
set(ANDROID_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(ANDROID_GPU_PROTOCOL_JSON "${ANDROID_GENERATED_DIR}/android-gpu-protocol.json")
set(ANDROID_GPU_PROTOCOL_HEADER "${ANDROID_GENERATED_DIR}/PGAndroidGpuProtocol.h")

add_custom_command(
    OUTPUT "${ANDROID_GPU_PROTOCOL_JSON}" "${ANDROID_GPU_PROTOCOL_HEADER}"
    COMMAND "${Python3_EXECUTABLE}" "${PORT_SCRIPT_DIR}/prepare_android_gpu_protocol.py"
        --renderer "${PORT_SOURCE_DIR}/MetalRenderer/MRRenderer.mm"
        --internal-header "${PORT_SOURCE_DIR}/MetalRenderer/MRInternal.h"
        --json "${ANDROID_GPU_PROTOCOL_JSON}"
        --header "${ANDROID_GPU_PROTOCOL_HEADER}"
    DEPENDS
        "${PORT_SCRIPT_DIR}/prepare_android_gpu_protocol.py"
        "${PORT_SOURCE_DIR}/MetalRenderer/MRRenderer.mm"
        "${PORT_SOURCE_DIR}/MetalRenderer/MRInternal.h"
    COMMENT "Extracting the original GTA GPU command-ring ABI for Android"
    VERBATIM
)
add_custom_target(android_gpu_protocol ALL
    DEPENDS "${ANDROID_GPU_PROTOCOL_JSON}" "${ANDROID_GPU_PROTOCOL_HEADER}"
)

add_library(gta5_android SHARED vulkan_host.cpp gpu_command_ring.cpp)
add_dependencies(gta5_android android_gpu_protocol)
target_compile_features(gta5_android PRIVATE cxx_std_20)
target_compile_options(gta5_android PRIVATE -Wall -Wextra -Werror=return-type)
target_include_directories(gta5_android PRIVATE "${ANDROID_GENERATED_DIR}")

find_library(android-lib android REQUIRED)
find_library(log-lib log REQUIRED)
find_library(vulkan-lib vulkan REQUIRED)
target_link_libraries(gta5_android PRIVATE ${android-lib} ${log-lib} ${vulkan-lib})

# Compile portable Android-compatible C modules as a separate object target.
add_library(port_android_compat OBJECT
    "${PORT_SOURCE_DIR}/NativeMeshRelease.c"
    "${PORT_SOURCE_DIR}/NativeMemoryBreakdown.c"
    "${PORT_SOURCE_DIR}/NativeThreadCPU.c"
    "${PORT_SOURCE_DIR}/NativeStreamingHealth.c"
)
target_include_directories(port_android_compat PRIVATE "${PORT_SOURCE_DIR}")
target_compile_features(port_android_compat PRIVATE c_std_11)
target_compile_options(port_android_compat PRIVATE -Wall -Wextra)
