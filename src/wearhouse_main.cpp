#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <future>
#include <list>
#include <mutex>
#include <queue>
#include <thread>

#include "common/random_number_generator.hpp"
#include "common/sprint.hpp"

#define MAX_EVENTS 100
#define PORT 8080

std::string LOG_FILE = "log_wearhouse.txt";
std::string LOG_TITLE = "wearhouse_main.cpp";
int NUMBER_OF_SIMONS = 2;

void set_nonblocking(int sock)
{
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

struct Order
{
    std::string what;
};

struct LocalOrder
{
    Order order;
    int estimated_time;
    int actual_delivery;
};

struct StartedFutureOrders
{
    std::mutex mtx_;
    std::list<std::future<LocalOrder>> future_orders_;
};

struct ProducerConsumerOrders
{
    std::queue<Order> orders_;
    std::mutex mtx_;
    std::condition_variable cv_;
};

class DeliveryVerificator
{
  public:
    DeliveryVerificator(StartedFutureOrders& future_orders) : future_orders_(future_orders)
    {
        thread_ = std::thread(&DeliveryVerificator::task, this);
        thread_.detach();
    }

    ~DeliveryVerificator() { /* thread.join(); */ }

    void task()
    {
        int order_cntr = 0;
        while (true)
        {
            {
                std::lock_guard<std::mutex> lock(future_orders_.mtx_);
                if (is_finished_ and future_orders_.future_orders_.empty()) break;
            }

            std::list<std::future<LocalOrder>> copy;
            std::list<LocalOrder> orders;
            {
                std::lock_guard<std::mutex> lock(future_orders_.mtx_);
                copy = std::move(future_orders_.future_orders_);
            }

            // Polling
            for (auto it = copy.begin(); it != copy.end();)
            {
                if (it->wait_for(std::chrono::milliseconds(100)) != std::future_status::ready)
                {
                    it++;
                    continue;
                }
                orders.push_back(it->get());
                it = copy.erase(it);
            }

            {
                std::lock_guard<std::mutex> lock(future_orders_.mtx_);
                for (auto& x : copy)
                {
                    future_orders_.future_orders_.emplace_back(std::move(x));
                }
            }

            if (orders.empty())
            {
                // Could add also cv on the future_orders_. Cashiers might be just very slow so it would busy loop in
                // the end too
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }

            for (const auto& order : orders)
            {
                order_cntr++;
                if (order.actual_delivery > order.estimated_time * 1.2)
                    sprint(LOG_TITLE, "Angry Dan: estimated: ", order.estimated_time,
                           " actual: ", order.actual_delivery);
                else
                    sprint(LOG_TITLE, "Happy Dan: estimated: ", order.estimated_time,
                           " actual: ", order.actual_delivery);
            }
        }
        sprint(LOG_TITLE, "Received in total: ", order_cntr, " orders");

        std::unique_lock<std::mutex> lock(mtx_);
        done_ = true;
        cv_.notify_one();
    }

    void release()
    {
        is_finished_.store(true);
        {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [this] { return done_ == true; });
        }
    }

  private:
    StartedFutureOrders& future_orders_;
    // Used to handle detached thread
    std::condition_variable cv_;
    std::mutex mtx_;
    bool done_ = false;  // could replaced with dan_futures.future_orders_.empty()
    //
    std::thread thread_;
    std::atomic<bool> is_finished_ = false;
};

class Truck
{
  public:
    Truck(ProducerConsumerOrders& orders, StartedFutureOrders& future_orders)
        : orders_(orders), future_orders_(future_orders)
    {
        thread_ = std::thread(&Truck::work, this);
    }

    void work()
    {
        while (true)
        {
            Order order;
            {
                std::unique_lock<std::mutex> lock(orders_.mtx_);
                orders_.cv_.wait(lock, [this] { return not orders_.orders_.empty() or is_finished_; });
                if (orders_.orders_.empty() and is_finished_)
                {
                    break;
                }
                order = orders_.orders_.front();
                orders_.orders_.pop();
            }

            std::promise<LocalOrder> order_promise;
            {
                std::lock_guard<std::mutex> lock(future_orders_.mtx_);
                future_orders_.future_orders_.push_back(order_promise.get_future());
            }

            const int estimated_time = RandomGenerator::generate<1000, 3000>();
            const double delay_factor = RandomGenerator::generate<0, 500>() / 1000.0;
            const int delivery_time = static_cast<int>(static_cast<double>(estimated_time) * (1 + delay_factor));
            std::this_thread::sleep_for(std::chrono::milliseconds(delivery_time));

            order_promise.set_value(LocalOrder{order, estimated_time, delivery_time});
        }
    }

    void release() { is_finished_.store(true); }

    ~Truck() { thread_.join(); }

  private:
    ProducerConsumerOrders& orders_;
    StartedFutureOrders& future_orders_;
    std::thread thread_;
    std::atomic<bool> is_finished_ = false;
};

class Wearhouse
{
  public:
    Wearhouse()
    {
        for (int i = 0; i < 3; i++)
        {
            trucks_.push_back(std::make_unique<Truck>(orders_, future_orders_));
        }
    }

    ~Wearhouse()
    {
        for (auto& x : trucks_) (*x).release();
        orders_.cv_.notify_all();
        trucks_.clear();
        verificator_.release();
    }

    void runServer()
    {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(PORT);
        inet_pton(AF_INET, "172.18.0.2", &address.sin_addr);
        bind(server_fd, (sockaddr*)&address, sizeof(address));
        listen(server_fd, SOMAXCONN);
        set_nonblocking(server_fd);

        int epoll_fd = epoll_create1(0);
        epoll_event event{}, events[MAX_EVENTS];
        event.data.fd = server_fd;
        event.events = EPOLLIN;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

        int closed_connections = 0;
        while (closed_connections < NUMBER_OF_SIMONS)
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
                        sprint(LOG_TITLE, "Connection with Simon closed");
                        ::close(events[i].data.fd);
                        closed_connections++;
                    }
                    else
                    {
                        {
                            std::lock_guard<std::mutex> lock(orders_.mtx_);
                            orders_.orders_.push(Order{buffer});
                            orders_.cv_.notify_one();
                        }
                        sprint(LOG_TITLE, "Dan received order: ", buffer);
                    }
                }
            }
        }
        ::close(server_fd);
        ::close(epoll_fd);
    }

  private:
    ProducerConsumerOrders orders_;
    StartedFutureOrders future_orders_;
    std::vector<std::unique_ptr<Truck>> trucks_;
    DeliveryVerificator verificator_{future_orders_};
};

int main()
{
    Wearhouse wearhouse;
    wearhouse.runServer();
}
