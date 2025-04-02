#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <vector>

#define MAX_EVENTS 10
#define PORT 8080

void set_nonblocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (sockaddr*)&address, sizeof(address));
    listen(server_fd, SOMAXCONN);
    set_nonblocking(server_fd);
    
    int epoll_fd = epoll_create1(0);
    epoll_event event{}, events[MAX_EVENTS];
    event.data.fd = server_fd;
    event.events = EPOLLIN;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    while (true) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == server_fd) {
                sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
                set_nonblocking(client_fd);
                event.data.fd = client_fd;
                event.events = EPOLLIN | EPOLLET;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
            } else {
                char buffer[1024] = {0};
                int bytes_read = read(events[i].data.fd, buffer, sizeof(buffer));
                if (bytes_read <= 0) {
                    close(events[i].data.fd);
                } else {
                    std::cout << "Received: " << buffer << std::endl;
                    send(events[i].data.fd, buffer, bytes_read, 0);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}

