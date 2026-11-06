cmake_minimum_required(VERSION 3.22.1)
project(gta5port_android LANGUAGES CXX)

add_library(gta5port SHARED port_status.cpp)
target_compile_features(gta5port PRIVATE cxx_std_20)
target_link_libraries(gta5port PRIVATE android log)
