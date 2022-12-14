# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.25

# compile ASM with /usr/bin/cc
# compile C with /usr/bin/cc
ASM_DEFINES = -DBH_FREE=wasm_runtime_free -DBH_MALLOC=wasm_runtime_malloc -DBH_PLATFORM_LINUX -DBUILD_TARGET_X86_64 -DWASM_DISABLE_HW_BOUND_CHECK=0 -DWASM_DISABLE_STACK_HW_BOUND_CHECK=0 -DWASM_ENABLE_BULK_MEMORY=1 -DWASM_ENABLE_FAST_INTERP=1 -DWASM_ENABLE_INTERP=1 -DWASM_ENABLE_LIBC_BUILTIN=1 -DWASM_ENABLE_LIBC_WASI=1 -DWASM_ENABLE_MINI_LOADER=0 -DWASM_ENABLE_MULTI_MODULE=0 -DWASM_ENABLE_SHARED_MEMORY=0 -DWASM_GLOBAL_HEAP_SIZE=10485760

ASM_INCLUDES = -I/home/dreamer/epcc/wasm-micro-runtime/core/iwasm/interpreter -I/home/dreamer/epcc/wasm-micro-runtime/core/iwasm/libraries/libc-builtin -I/home/dreamer/epcc/wasm-micro-runtime/core/iwasm/libraries/libc-wasi/sandboxed-system-primitives/include -I/home/dreamer/epcc/wasm-micro-runtime/core/iwasm/libraries/libc-wasi/sandboxed-system-primitives/src -I/home/dreamer/epcc/wasm-micro-runtime/core/iwasm/include -I/home/dreamer/epcc/wasm-micro-runtime/core/shared/platform/linux -I/home/dreamer/epcc/wasm-micro-runtime/core/shared/platform/linux/../include -I/home/dreamer/epcc/wasm-micro-runtime/core/shared/mem-alloc -I/home/dreamer/epcc/wasm-micro-runtime/core/iwasm/common -I/home/dreamer/epcc/wasm-micro-runtime/core/shared/utils

ASM_FLAGS = -pthread

C_DEFINES = -DBH_FREE=wasm_runtime_free -DBH_MALLOC=wasm_runtime_malloc -DBH_PLATFORM_LINUX -DBUILD_TARGET_X86_64 -DWASM_DISABLE_HW_BOUND_CHECK=0 -DWASM_DISABLE_STACK_HW_BOUND_CHECK=0 -DWASM_ENABLE_BULK_MEMORY=1 -DWASM_ENABLE_FAST_INTERP=1 -DWASM_ENABLE_INTERP=1 -DWASM_ENABLE_LIBC_BUILTIN=1 -DWASM_ENABLE_LIBC_WASI=1 -DWASM_ENABLE_MINI_LOADER=0 -DWASM_ENABLE_MULTI_MODULE=0 -DWASM_ENABLE_SHARED_MEMORY=0 -DWASM_GLOBAL_HEAP_SIZE=10485760

C_INCLUDES = -I/home/dreamer/epcc/wasm-micro-runtime/core/iwasm/interpreter -I/home/dreamer/epcc/wasm-micro-runtime/core/iwasm/libraries/libc-builtin -I/home/dreamer/epcc/wasm-micro-runtime/core/iwasm/libraries/libc-wasi/sandboxed-system-primitives/include -I/home/dreamer/epcc/wasm-micro-runtime/core/iwasm/libraries/libc-wasi/sandboxed-system-primitives/src -I/home/dreamer/epcc/wasm-micro-runtime/core/iwasm/include -I/home/dreamer/epcc/wasm-micro-runtime/core/shared/platform/linux -I/home/dreamer/epcc/wasm-micro-runtime/core/shared/platform/linux/../include -I/home/dreamer/epcc/wasm-micro-runtime/core/shared/mem-alloc -I/home/dreamer/epcc/wasm-micro-runtime/core/iwasm/common -I/home/dreamer/epcc/wasm-micro-runtime/core/shared/utils

C_FLAGS =  -std=gnu99 -ffunction-sections -fdata-sections                                          -Wall -Wno-unused-parameter -Wno-pedantic -fPIC -pthread

