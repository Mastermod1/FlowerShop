#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <ranges>
#include <semaphore>
#include <thread>
#include "random_number_generator.hpp"
#include "sprint.hpp"

constexpr int numberOfCashiers = 5;
constexpr int numberOfSimons = 2;

struct Order
{
    std::string what;
};

class Wearhouse  // Dan
{
  public:
    Wearhouse()
    {
        for (int i = 0; i < 5; i++)
        {
            cars.push_back(std::thread([]() { sprint("Carrr"); }));
        }
    }

    ~Wearhouse()
    {
        for (int i = 0; i < 5; i++)
        {
            cars[i].join();
        }
    }

    void notify(Order order)
    {
        {
            std::lock_guard<std::mutex> lock(wearhouseMtx);
            orders_.push(order);
        }
        sprint("Notified wearhouse with order: {}", order.what);
    }

    void check()
    {
        // If the random estimated time is 3 seconds, Dan gets mad if it's delivered in 1.2 times
        // Estimated 3.6seconds
    }

  private:
    std::queue<Order> orders_;
    std::vector<std::thread> cars;
    std::mutex wearhouseMtx;
};

class OrderManager
{
  public:
    OrderManager(const std::shared_ptr<Wearhouse>& wearhouse) : wearhouse_(wearhouse) {}
    void start()
    {
        if (wearhouse_ == nullptr)
        {
            std::cerr << "Nullptr on wearhouse" << std::endl;
            exit(0);
        }
        std::vector<std::thread> cashiers;
        std::thread consumer(&OrderManager::orderHandler, this);
        for (const auto& i : std::views::iota(0, numberOfCashiers))
        {
            cashiers.push_back(std::thread(&OrderManager::registerHandler, this, i));
        }
        for (auto& cashier : cashiers)
        {
            cashier.join();
        }
        consumer.join();
    }

    void registerHandler(int id)
    {
        sprint("Cashier start: {}", id);
        int order_num = 0;
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(RandomGenerator::generate<500, 1500>()));
            auto order = Order{std::format("Bananas by cashier: {} Ord num: {}", id, order_num)};
            sprint("Make order: {}", order.what);
            queue_push(order);
            order_num++;
        }
    }

    void orderHandler()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(queueMtx_);
            queueCondVar_.wait(lock, [this] { return !orders_.empty() and simonsSemaphore_.try_acquire(); });
            sprint("INFO: order count {}", orders_.size());
            auto order = orders_.front();
            orders_.pop();
            lock.unlock();
            std::thread simon(
                [this](const Order order)
                {
                    sprint("Simon calls wearhouse with order: {}", order.what);
                    wearhouse_->notify(order);
                    std::this_thread::sleep_for(std::chrono::milliseconds(RandomGenerator::generate<100, 300>()));
                    simonsSemaphore_.release();
                    queueCondVar_.notify_one();
                },
                order);
            simon.detach();
        }
    }

    void queue_push(const Order& order)
    {
        std::unique_lock<std::mutex> lock(queueMtx_);
        orders_.push(order);
        queueCondVar_.notify_one();
    }

  private:
    std::counting_semaphore<2> simonsSemaphore_{2};
    std::mutex queueMtx_;
    std::condition_variable queueCondVar_;
    std::queue<Order> orders_;  // shared for cashiers and simons
    std::shared_ptr<Wearhouse> wearhouse_;
};

int main()
{
    auto wearhouse = std::make_shared<Wearhouse>();
    OrderManager queue_manager_(wearhouse);
    queue_manager_.start();
    return 0;
}
