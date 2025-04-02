cmake_minimum_required(VERSION 3.10)
project(FlowerShop)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER g++-14)

add_executable(Shop src/shop_main.cpp)
add_executable(Wearhouse src/wearhouse_main.cpp)

add_executable(Client src/main_socket_client.cpp)
add_executable(Server src/main_socket_server.cpp)

option(USE_SANITIZERS "Enable sanitizers for debugging" ON)

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
