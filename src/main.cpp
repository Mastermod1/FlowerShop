#include <bits/stdc++.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#ifdef DBG
constexpr bool g_debug = true;
#else
constexpr bool g_debug = true;
#endif

constexpr std::size_t g_data_size = 20'000;
constexpr std::size_t g_storage_size = 100;

using storage_t = std::queue<int>;

std::condition_variable g_cv;
std::mutex g_mtx;
bool stop_consumers = false;

auto get_delivery_time()
{
    return (rand()%15)+5;
}

void producer(storage_t& storage, int id)
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
        exit(socket_fd);

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    server_address.sin_addr.s_addr = INADDR_ANY;

    auto bind_status = bind(socket_fd, (struct sockaddr*)&server_address, sizeof(server_address));
    if (bind_status == -1)
    {
        std::cout << "Bind err " << bind_status << "\n";
        exit(bind_status);
    }
    listen(socket_fd, 5);

    std::list<int> clients;
    int client_fd = accept(socket_fd, nullptr, nullptr);
    clients.push_back(client_fd);
    std::cout << "accepted " << client_fd << "\n";
    for(;;)
    {
        std::queue<char> orders;
        for (auto it = clients.begin(); it != clients.end(); it++)
        {
            std::uint8_t buffer[1] = {0};
            int len = recv(*it, buffer, sizeof(buffer), 0);

            if (len != 0)
            {
                if (buffer[0] == 'S')
                {
                    close(*it);
                    it = clients.erase(it);
                    break;
                }
                else
                {
                    orders.push(buffer[0]);
                }
            }
        }
        if (clients.empty())
            break;
        std::unique_lock lck(g_mtx);
        if (storage.size() >= g_storage_size)
            g_cv.wait(lck, [&storage]{ return storage.size() < g_storage_size; });
        while (not orders.empty())
        {
            storage.push(orders.front());
            orders.pop();
        }
        if constexpr (g_debug)
            std::cout << "Produced[" << id << "]: " << storage.front() << "\n";
        g_cv.notify_all();
        lck.unlock();
    }
    std::cout << "Production Closed\n";
    stop_consumers = true;
    g_cv.notify_all();
    close(socket_fd);
}

void consumer(storage_t& storage, int id)
{
    for (;;)
    {
        std::unique_lock lck(g_mtx);
        if (storage.size() == 0)
            g_cv.wait(lck, [&storage]{ return storage.size() != 0 or stop_consumers; });
        if (storage.empty() and stop_consumers)
            break;
        int consumed = storage.front();
        storage.pop();
        g_cv.notify_all();
        if constexpr (g_debug)
            std::cout << "Consumed[" << id << "]: " << consumed << " Left In Queue: " << storage.size()  << "\n";
        lck.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(get_delivery_time()));
    }
    g_mtx.lock();
    std::cout << "Finished deliveries from: " << id << "\n";
    g_mtx.unlock();
}

int main()
{
    srand(time(0)); 
    storage_t storage;
    std::thread producer_thread_1(producer, std::ref(storage), 1);
    std::thread consumer_thread_1(consumer, std::ref(storage), 1);
    std::thread consumer_thread_2(consumer, std::ref(storage), 2);
    std::thread consumer_thread_3(consumer, std::ref(storage), 3);
    producer_thread_1.join();
    consumer_thread_1.join();
    consumer_thread_2.join();
    consumer_thread_3.join();
    std::cout << "Finished\n";
}
