set(BINARY ${CMAKE_PROJECT_NAME})

add_executable(${BINARY}_APP main.cpp)
add_executable(SENDER_APP order_maker.cpp)
