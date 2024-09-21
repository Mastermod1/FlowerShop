#include <bits/stdc++.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

void uber_shop_order_maker(int id)
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        std::cout << "Client err " << socket_fd << "\n";
        exit(socket_fd);
    }

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(2137);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    connect(socket_fd, (struct sockaddr*)&server_address, sizeof(server_address));
    
    char c;
    for (int i = 0; i < 10; i++)
    {
        c = 'a' + id;
        send(socket_fd, &c, 1, 0);
        if (c == 'S')
            break;
    }
    close(socket_fd);
}

int main()
{
    std::vector<std::thread> thr;
    for (int i = 0; i < 10; i++)
        thr.emplace_back(uber_shop_order_maker, i);

    for (int i = 0; i < 10; i++)
        thr[i].join();
}
