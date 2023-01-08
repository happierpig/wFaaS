cmake_minimum_required(VERSION 3.14)
project(proxy VERSION 0.1)  

add_compile_options(-pthread)

add_executable(proxy proxy.cpp ../include/utils.cpp) 

target_link_libraries (proxy PUBLIC -lpthread)
