#include <chrono>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>

#define PORT 8080

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed\n";
        return 1;
    }

    for (int i = 0; i < 5; i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        const char* message = "Hello, server!";
        send(sock, message, strlen(message), 0);

        char buffer[1024] = {0};
        read(sock, buffer, sizeof(buffer));
        std::cout << "Server response: " << buffer << std::endl;
    }

    close(sock);
    return 0;
}
