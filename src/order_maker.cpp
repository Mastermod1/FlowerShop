#include <bits/stdc++.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main()
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        std::cout << "Client err " << socket_fd << "\n";
        exit(socket_fd);
    }

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    server_address.sin_addr.s_addr = INADDR_ANY;
    connect(socket_fd, (struct sockaddr*)&server_address, sizeof(server_address));
    
    char c;
    for (;;)
    {
        std::cin >> c;
        send(socket_fd, &c, 1, 0);
        if (c == 'S')
            break;
    }
    close(socket_fd);
}
