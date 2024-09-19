set(BINARY ${CMAKE_PROJECT_NAME})

add_executable(${BINARY}_APP main.cpp job.hpp icommunication.hpp socket_communication.hpp)
add_executable(SENDER_APP order_maker.cpp)
