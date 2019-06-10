project(retrocpu)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

set(CPU_SOURCES
        cpu.cc
        debug_interface.cc)
set(CPU_HEADERS
        host_system.h
        cpu.h)
add_library(retro_cpu_core ${CPU_SOURCES} ${CPU_HEADERS})

set(HOST_LINUX_SOURCES host/host_linux.cc)
add_library(retro_host_linux ${HOST_LINUX_SOURCES})
target_include_directories(retro_host_linux PUBLIC ./)

set(CPU_65816_SOURCES
    cpu/65816/cpu_65c816.cc)
set(CPU_65816_HEADERS
    cpu/65816/cpu_65c816.h)

add_library(retro_cpu_65816 ${CPU_65816_SOURCES} ${CPU_65816_HEADERS})
add_dependencies(retro_cpu_65816 retro_host_linux retro_cpu_core)
target_include_directories(retro_cpu_65816 PUBLIC ./)
