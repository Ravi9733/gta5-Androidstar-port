cmake_minimum_required(VERSION 3.22.1)
project(fivestarport_android_mods LANGUAGES CXX)

add_library(android_mod_overlay STATIC
    AndroidModOverlay.cpp
)

target_compile_features(android_mod_overlay PUBLIC cxx_std_17)
target_include_directories(android_mod_overlay PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
