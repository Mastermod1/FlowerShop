cmake_minimum_required(VERSION 3.10)
project(FlowerShop)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Shop src/shop_main.cpp)
add_executable(Wearhouse src/wearhouse_main.cpp)

option(USE_SANITIZERS "Enable sanitizers for debugging" OFF)
message("USE_SANITIZERS is: '${USE_SANITIZERS}'")

if(USE_SANITIZERS)
    target_compile_options(Shop PRIVATE 
        -g
        -fsanitize=thread
        -fsanitize=undefined
    )
    target_link_options(Shop PRIVATE 
        -g
        -fsanitize=thread
        -fsanitize=undefined
    )

    target_compile_options(Wearhouse PRIVATE 
        -g
        -fsanitize=thread
        -fsanitize=undefined
    )
    target_link_options(Wearhouse PRIVATE 
        -g
        -fsanitize=thread
        -fsanitize=undefined
    )
endif()

target_include_directories(Shop PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(Wearhouse PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_options(Shop PRIVATE -pthread)
target_link_options(Wearhouse PRIVATE -pthread)
