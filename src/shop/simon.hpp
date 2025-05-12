#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common/sprint.hpp"
#include "common/globals.hpp"
#include "shop/order.hpp"

class Simon
{
  public:
    Simon()
    {
        sock_ = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        inet_pton(AF_INET, IP_ADDR.c_str(), &server_addr.sin_addr);

        if (connect(sock_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        {
            sprint("Simon", "Connection failed");
            exit(-1);
        }
    }

    ~Simon()
    {
        close(sock_);
        sprint("Simon", "Simon sent ", sent_msgs_, " messages");
    }

    void sendOrder(Order order)
    {
        send(sock_, order.what.c_str(), order.what.length(), 0);
        sent_msgs_++;
    }

  private:
    int sent_msgs_ = 0;
    int sock_;
};
