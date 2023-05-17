cmake_minimum_required(VERSION 3.14)
project(proxy VERSION 0.1)  

add_compile_options(-pthread)
add_compile_options(-fno-rtti)

add_executable(proxy proxy.cpp ../include/utils.cpp ../include/base64.cpp) 

target_link_libraries (proxy PUBLIC -lpthread alibabacloud-oss-cpp-sdk curl crypto)
