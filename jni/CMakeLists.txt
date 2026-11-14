cmake_minimum_required(VERSION 3.10.2)
project(ppid_spoofer)

set(CMAKE_CXX_STANDARD 17)

add_library(ppid_spoofer SHARED module.cpp)
target_link_libraries(ppid_spoofer log dl)

target_include_directories(ppid_spoofer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 核心修复：把 Dobby 锁死在稳定的 v1.2.5 版本，跳过 master 分支的破汇编
include(FetchContent)
FetchContent_Declare(
    dobby
    GIT_REPOSITORY https://github.com/jmpews/Dobby.git
    GIT_TAG        v1.2.5
)
FetchContent_MakeAvailable(dobby)

target_link_libraries(ppid_spoofer dobby)
