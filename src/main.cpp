#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <random>
#include <ranges>
#include <semaphore>
#include <thread>

class Wearhouse
{
  public:
    void notify(std::string msg) { std::cout << "Notified wearhouse: " << msg << std::endl; }
};

struct Order
{
    std::string what;
};

// shared for cashiers and simons
std::queue<Order> orders;
constexpr int numberOfCashiers = 5;
constexpr int numberOfSimons = 2;
std::counting_semaphore<numberOfSimons> simonsSemaphore(numberOfSimons);
std::mutex queueMtx;
std::mutex coutMtx;

class Consumer
{
  public:
    void operator()(int id)
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(queueMtx);
            coutMtx.lock();
            std::cout << "INFO:  QSIZE " << orders.size() << std::endl;
            coutMtx.unlock();
            if (orders.size() > 0 and simonsSemaphore.try_acquire())
            {
                auto order = orders.front();
                orders.pop();
                lock.unlock();
                std::thread simon(
                    [](const Order order)
                    {
                        coutMtx.lock();
                        std::cout << "Simon calls for order: " + order.what << std::endl;
                        coutMtx.unlock();
                        simonsSemaphore.release();
                    },
                    order);
                simon.detach();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            else
            {
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
};

class Cashier
{
  public:
    void operator()(int id)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(500, 1500);
        coutMtx.lock();
        std::cout << "Cashier start: " << id << std::endl;
        coutMtx.unlock();
        int order_num = 0;
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(distrib(gen)));
            queueMtx.lock();
            auto order = Order{"Bananas by cashier: " + std::to_string(id) + " Ord num: " + std::to_string(order_num)};
            coutMtx.lock();
            std::cout << "Make order: " << order.what << std::endl;
            coutMtx.unlock();
            orders.push(order);
            queueMtx.unlock();
            order_num++;
        }
    }
};

int main()
{
    std::cout << "Hello World!" << std::endl;
    std::vector<std::thread> cashiers;
    for (const auto& i : std::views::iota(0, numberOfCashiers))
    {
        cashiers.push_back(std::thread(Cashier(), i));
    }
    std::thread consumer(Consumer(), 1);
    consumer.join();
    for (auto& cashier : cashiers)
    {
        cashier.join();
    }
    return 0;
}
