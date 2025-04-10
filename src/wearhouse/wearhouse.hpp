#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <memory>
#include <string>
#include <vector>

#include "../common/sprint.hpp"
#include "delivery_verificator.hpp"
#include "truck.hpp"
#include "wearhouse_storage.hpp"

void set_nonblocking(int sock)
{
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

const int MAX_EVENTS = 100;
const int PORT = 8080;
const std::string IP_ADDR = "127.0.0.1";

class Wearhouse
{
  public:
    Wearhouse()
    {
        for (int i = 0; i < 3; i++)
        {
            trucks_.push_back(std::make_unique<Truck>(storage_, future_orders_));
        }
    }

    ~Wearhouse()
    {
        storage_.finish();
        trucks_.clear();
        verificator_.release();
    }

    void runServer()
    {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(PORT);
        inet_pton(AF_INET, IP_ADDR.c_str(), &address.sin_addr);
        bind(server_fd, (sockaddr*)&address, sizeof(address));
        listen(server_fd, SOMAXCONN);
        set_nonblocking(server_fd);

        int epoll_fd = epoll_create1(0);
        epoll_event event{}, events[MAX_EVENTS];
        event.data.fd = server_fd;
        event.events = EPOLLIN;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

        int closed_connections = 0;
        while (true)
        {
            int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
            for (int i = 0; i < n; ++i)
            {
                if (events[i].data.fd == server_fd)
                {
                    sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
                    set_nonblocking(client_fd);
                    event.data.fd = client_fd;
                    event.events = EPOLLIN | EPOLLET;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
                }
                else
                {
                    char buffer[1024] = {0};
                    int bytes_read = read(events[i].data.fd, buffer, sizeof(buffer));
                    if (bytes_read <= 0)
                    {
                        sprint("Wearhouse", "Connection with Simon closed");
                        ::close(events[i].data.fd);
                        closed_connections++;
                    }
                    else
                    {
                        storage_.insertOrder(Order{buffer});
                    }
                }
            }
        }
        ::close(server_fd);
        ::close(epoll_fd);
    }

  private:
    StartedFutureOrders future_orders_;
    std::vector<std::unique_ptr<Truck>> trucks_;
    WearhouseStorage storage_;
    DeliveryVerificator verificator_{future_orders_};
};
