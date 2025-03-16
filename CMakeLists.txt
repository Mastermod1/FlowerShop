cmake_minimum_required(VERSION 3.10)
project(FlowerShop)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER g++-13)

add_executable(FlowerShop src/main.cpp)

target_include_directories(FlowerShop PRIVATE ${CMAKE_SOURCE_DIR}/src)
