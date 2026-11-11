cmake_minimum_required(VERSION 3.22.1)
project(gta5_android LANGUAGES C CXX)

add_library(gta5_android SHARED
    vulkan_host.cpp
)

target_compile_features(gta5_android PRIVATE cxx_std_20)
target_compile_options(gta5_android PRIVATE -Wall -Wextra -Werror=return-type)

find_library(android-lib android REQUIRED)
find_library(log-lib log REQUIRED)
find_library(vulkan-lib vulkan REQUIRED)

target_link_libraries(gta5_android
    PRIVATE
    ${android-lib}
    ${log-lib}
    ${vulkan-lib}
)

# Compile the platform-neutral/Android fallback C modules as object files. They
# are not linked into the app shell yet because the generated WASM engine and
# Vulkan command renderer are still separate porting stages.
set(PORT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../../app/Source")
add_library(port_android_compat OBJECT
    "${PORT_SOURCE_DIR}/NativeMeshRelease.c"
    "${PORT_SOURCE_DIR}/NativeMemoryBreakdown.c"
    "${PORT_SOURCE_DIR}/NativeThreadCPU.c"
    "${PORT_SOURCE_DIR}/NativeStreamingHealth.c"
)
target_include_directories(port_android_compat PRIVATE "${PORT_SOURCE_DIR}")
target_compile_features(port_android_compat PRIVATE c_std_11)
target_compile_options(port_android_compat PRIVATE -Wall -Wextra)
