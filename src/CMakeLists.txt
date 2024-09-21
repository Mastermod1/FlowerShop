set(BINARY ${CMAKE_PROJECT_NAME})

add_executable(${BINARY}_APP main.cpp job.hpp icommunication.hpp socket_communication.hpp order_manager.hpp)
add_executable(OrderMaker_APP order_maker.cpp)
add_executable(AutoOrder_APP auto_order.cpp)
