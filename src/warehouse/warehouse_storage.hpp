#pragma once

#include <condition_variable>
#include <future>
#include <mutex>
#include <optional>
#include <queue>

#include "common/random_number_generator.hpp"
#include "common/sprint.hpp"
#include "warehouse/delivery_verificator.hpp"
#include "warehouse/order.hpp"

class WarehouseStorage
{
  public:
    std::optional<Order> getNext()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return not orders_.empty() or finished_; });
        if (orders_.empty()) return std::nullopt;
        auto order = std::move(orders_.front());
        orders_.pop();

        const int estimated_time = RandomGenerator::generate<1000, 3000>();
        order.time = estimated_time;

        DeliveryVerificator verificator(order.promise_.get_future(), estimated_time);
        return std::move(order);
    }

    void insertOrder(Order&& order)
    {
        std::string what = order.what;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            orders_.push(std::move(order));
            cv_.notify_one();
        }
        sprint("Warehouse", "Dan received order: ", what);
    }

    void finish()
    {
        finished_.store(true);
        cv_.notify_all();
    }

    ~WarehouseStorage()
    {
        finished_.store(true);
        cv_.notify_all();
    }

  private:
    std::atomic<bool> finished_{false};
    std::queue<Order> orders_;
    std::mutex mtx_;
    std::condition_variable cv_;
};
