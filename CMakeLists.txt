cmake_minimum_required(VERSION 3.10)

set(CMAKE_BUILD_TYPE Debug)
project(win_env)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# May help with compiler problems
#set(Boost_COMPILER "-clang18")
#if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
#    set(Boost_ARCHITECTURE "-x64")
#else()
#    set(Boost_ARCHITECTURE "-x32")
#endif()

find_package(Boost REQUIRED COMPONENTS json)
include_directories(${Boost_INCLUDE_DIRS}) 

add_subdirectory(src)
