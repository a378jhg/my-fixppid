cmake_minimum_required(VERSION 3.10.2)
project(ppid_spoofer)

add_library(ppid_spoofer SHARED module.cpp)
target_link_libraries(ppid_spoofer log dl)

include(FetchContent)
FetchContent_Declare(zygisk GIT_REPOSITORY https://github.com/Dr-TSNG/Zygisk-API.git)
FetchContent_MakeAvailable(zygisk)
target_include_directories(ppid_spoofer PRIVATE ${zygisk_SOURCE_DIR})
target_link_libraries(ppid_spoofer zygisk)
