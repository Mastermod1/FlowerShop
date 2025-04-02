#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <condition_variable>
#include <cstring>
#include <future>
#include <list>
#include <mutex>
#include <queue>
#include <thread>

#include "random_number_generator.hpp"
#include "sprint.hpp"

#define MAX_EVENTS 100
#define PORT 8080

void set_nonblocking(int sock)
{
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

struct Order
{
    std::string what;
};

class Wearhouse
{
  public:
    Wearhouse() { buildTrucks(); }

    void buildTrucks()
    {
        for (int i = 0; i < 3; i++)
        {
            trucks_.push_back(std::make_unique<Truck>(dan_orders, dan_futures));
        }
    }

    ~Wearhouse()
    {
        // release trucks
        for (auto& x : trucks_) (*x).release();
        dan_orders.queue_cv_.notify_all();
        trucks_.clear();
        verificator_.release();
    }

    void close() { is_working_wearhouse_.store(false); }

    void runServer()
    {
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

        int clients = 0;
        while (clients < 2)
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
                        sprint("close");
                        ::close(events[i].data.fd);
                        clients++;
                    }
                    else
                    {
                        std::lock_guard<std::mutex> lock(dan_orders.queue_mtx_);
                        dan_orders.orders_.push(Order{buffer});
                        dan_orders.queue_cv_.notify_one();
                        sprint("Dan received order: {}", buffer);
                    }
                }
            }
        }
        ::close(server_fd);
    }

  private:
    struct LocalOrder
    {
        Order order;
        int estimated_time;
        int actual_delivery;
    };

    struct DanFutures
    {
        std::mutex fututres_mtx_;
        std::list<std::future<LocalOrder>> future_orders_;
    };

    struct DanOrders
    {
        // Those protect eachother
        std::queue<Order> orders_;
        std::mutex queue_mtx_;
        std::condition_variable queue_cv_;
    };

    class DeliveryVerificator
    {
      public:
        DeliveryVerificator(DanFutures& dan_futures) : dan_futures(dan_futures)
        {
            thread = std::thread(&DeliveryVerificator::task, this);
            // thread.detach();
        }

        ~DeliveryVerificator() { thread.join(); }

        void task()
        {
            int order_cntr = 0;
            while (true)
            {
                {
                    std::lock_guard<std::mutex> lock(dan_futures.fututres_mtx_);
                    if (is_finished and dan_futures.future_orders_.empty()) return;
                }

                std::list<LocalOrder> tmp;
                // polling
                {
                    std::lock_guard<std::mutex> lock(dan_futures.fututres_mtx_);
                    for (auto it = dan_futures.future_orders_.begin(); it != dan_futures.future_orders_.end();)
                    {
                        if (it->wait_for(std::chrono::milliseconds(100)) != std::future_status::ready)
                        {
                            it++;
                            continue;
                        }
                        tmp.push_back(it->get());
                        it = dan_futures.future_orders_.erase(it);
                    }
                }

                if (tmp.empty())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    continue;
                }

                for (const auto& order : tmp)
                {
                    order_cntr++;
                    if (order.actual_delivery > order.estimated_time * 1.2)
                        sprint("Angry Dan: estimated: {} actual: {}", order.estimated_time, order.actual_delivery);
                    else
                        sprint("Happy Dan: estimated: {} actual: {}", order.estimated_time, order.actual_delivery);
                }
                sprint("Dan received {} orders", order_cntr);
            }
        }

        void release() { is_finished.store(true); }

      private:
        DanFutures& dan_futures;
        std::thread thread;
        std::atomic<bool> is_finished = false;
    };

    class Truck
    {
      public:
        Truck(DanOrders& data, DanFutures& dan_futures) : dan_orders(data), dan_futures(dan_futures)
        {
            thread = std::thread(&Truck::work, this);
        }

        void work()
        {
            while (true)
            {
                Order order;
                std::promise<LocalOrder> orderPromise;
                {
                    std::unique_lock<std::mutex> lock(dan_orders.queue_mtx_);
                    dan_orders.queue_cv_.wait(lock,
                                              [this] { return not dan_orders.orders_.empty() or not is_working; });
                    if (dan_orders.orders_.empty() and not is_working)
                    {
                        return;
                    }
                    order = dan_orders.orders_.front();
                    dan_orders.orders_.pop();
                }

                {
                    std::lock_guard<std::mutex> lock(dan_futures.fututres_mtx_);
                    dan_futures.future_orders_.push_back(orderPromise.get_future());
                }

                const int estimated_time = RandomGenerator::generate<1000, 3000>();
                const double delay_factor = RandomGenerator::generate<0, 500>() / 1000.0;
                const int delivery_time = static_cast<int>(static_cast<double>(estimated_time) * (1 + delay_factor));
                std::this_thread::sleep_for(std::chrono::milliseconds(delivery_time));

                orderPromise.set_value(LocalOrder{order, estimated_time, delivery_time});
            }
        }

        void release() { is_working.store(false); }

        ~Truck() { thread.join(); }

      private:
        DanOrders& dan_orders;
        DanFutures& dan_futures;
        std::thread thread;
        std::atomic<bool> is_working = true;
    };

    DanOrders dan_orders;
    DanFutures dan_futures;
    std::vector<std::unique_ptr<Truck>> trucks_;
    DeliveryVerificator verificator_{dan_futures};
    std::atomic<bool> is_working_wearhouse_ = true;
};

int main()
{
    Wearhouse wearhouse;
    wearhouse.runServer();
}
