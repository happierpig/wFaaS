cmake_minimum_required(VERSION 3.14)
project(worker VERSION 0.1)  


set (WAMR_BUILD_PLATFORM "linux")
set (WAMR_BUILD_TARGET "X86_64")
set (WAMR_BUILD_INTERP 1)
set (WAMR_BUILD_FAST_INTERP 1)
set (WAMR_BUILD_AOT 0)
set (WAMR_BUILD_LIBC_BUILTIN 1)
set (WAMR_BUILD_LIBC_WASI 1)
set (WAMR_BUILD_LIB_PTHREAD 0)
set (WAMR_DISABLE_HW_BOUND_CHECK 0)
set (WAMR_BUILD_SIMD 0)
set (WAMR_ROOT_DIR /vFaaS/wasm-micro-runtime)

add_compile_options(-pthread)

include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})
add_executable(worker main.cpp ../include/utils.cpp ../include/base64.cpp) 

target_link_libraries (worker PUBLIC vmlib -lpthread)
