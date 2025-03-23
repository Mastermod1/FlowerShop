cmake_minimum_required(VERSION 3.10)
project(FlowerShop)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER g++-13)

add_executable(FlowerShop src/main.cpp)

option(USE_SANITIZERS "Enable sanitizers for debugging" ON)

if(USE_SANITIZERS)
    target_compile_options(FlowerShop PRIVATE 
        -fsanitize=thread
        -fsanitize=undefined
    )
    target_link_options(FlowerShop PRIVATE 
        -fsanitize=thread
        -fsanitize=undefined
    )
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -fno-omit-frame-pointer")

set(CMAKE_BUILD_TYPE Debug)

target_include_directories(FlowerShop PRIVATE ${CMAKE_SOURCE_DIR}/src)
