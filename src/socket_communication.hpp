#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

#include "icommunication.hpp"

class SocketCommunication : public ICommunication {
public:
  void init() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
      std::cout << "Socket " << server_fd_ << "\n";
      exit(server_fd_);
    }

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(2137);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    auto bind_status = bind(server_fd_, (struct sockaddr *)&server_address,
                            sizeof(server_address));
    if (bind_status == -1) {
      std::cout << "Bind err " << bind_status << "\n";
      exit(bind_status);
    }
    listen(server_fd_, 5);
  }

  void waitForClient() {
    client_fd_ = accept(server_fd_, nullptr, nullptr);
    std::cout << "Accepted client:  " <<  client_fd_ << "\n";
  }

  char read() {
    std::uint8_t buffer[1] = {0};
    int len = recv(client_fd_, buffer, sizeof(buffer), 0);
    if (len == 0 or buffer[0] == g_stop_msg) {
      close(client_fd_);
    }
    return buffer[0];
  }

  ~SocketCommunication() { close(server_fd_); }

private:
  int server_fd_;
  int client_fd_;
};
