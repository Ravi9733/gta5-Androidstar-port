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
