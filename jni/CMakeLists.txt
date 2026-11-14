cmake_minimum_required(VERSION 3.10.2)
project(ppid_spoofer)

set(CMAKE_CXX_STANDARD 17)

add_library(ppid_spoofer SHARED module.cpp)
target_link_libraries(ppid_spoofer log dl)

target_include_directories(ppid_spoofer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
