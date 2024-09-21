#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <forward_list>
#include <iostream>

#include "icommunication.hpp"

class SocketCommunication : public ICommunication {
public:
  void setupListeningSocket() {
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
    if (fcntl(server_fd_, F_SETFL, O_NONBLOCK) == -1) {
      std::cout << "Fcntl err\n";
    }
    listen(server_fd_, 5);
    FD_ZERO(&clients_);
    FD_SET(server_fd_, &clients_);
    max_ = server_fd_;
  }

  int setup() override {
    setupListeningSocket();
    return 1;
  }

  std::forward_list<char> read() override {
    fd_set check_activity = clients_;
    int activity = select(max_ + 1, &check_activity, NULL, NULL, NULL);
    if (activity < 0)
      exit(-1);
    std::forward_list<char> list;
    if (FD_ISSET(server_fd_, &check_activity)) {
      int client_fd = accept(server_fd_, nullptr, nullptr);
      if (client_fd == -1) {
        std::cout << "Error on client fd " << client_fd << "\n";
      }
      std::cout << "Accepted " << client_fd << "\n";
      FD_SET(client_fd, &clients_);
      max_ = std::max(max_, client_fd);
    } else {
      for (int i = 0; i < max_; i++) {
        if (FD_ISSET(i, &check_activity)) {
          std::uint8_t buffer[1] = {0};
          int len = recv(i, buffer, sizeof(buffer), 0);
          if ((len == 0 or buffer[0] == g_stop_msg) and i != server_fd_) {
            FD_CLR(i, &clients_);
            close(i);
            continue;
          }
          list.push_front(buffer[0]);
        }
      }
    }
    return list;
  }

  ~SocketCommunication() { close(server_fd_); }

private:
  int server_fd_;
  fd_set clients_;
  int max_ = 0;
};
