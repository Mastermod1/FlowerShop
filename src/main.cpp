#include <bits/stdc++.h>

#include "socket_communication.hpp"
#include "order_manager.hpp"

int main() {
  srand(time(0));
  auto socket = std::make_unique<SocketCommunication>();
  socket->setupListeningSocket();
  // socket->waitForClient();
  OrderManager manager(std::move(socket));
  manager.run();
}
