cmake_minimum_required(VERSION 3.10.2)
project(ppid_spoofer)

add_library(ppid_spoofer SHARED module.cpp)
target_link_libraries(ppid_spoofer log dl)

# 直接包含当前目录，不联网下载了
target_include_directories(ppid_spoofer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
