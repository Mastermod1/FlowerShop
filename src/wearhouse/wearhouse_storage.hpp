#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

#include "common/sprint.hpp"
#include "wearhouse/order.hpp"

class WearhouseStorage
{
  public:
    std::optional<Order> getNext()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return not orders_.empty() or finished_; });
        if (orders_.empty()) return std::nullopt;
        auto order = orders_.front();
        orders_.pop();
        return order;
    }

    void insertOrder(Order order)
    {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            orders_.push(order);
            cv_.notify_one();
        }
        sprint("Wearhouse", "Dan received order: ", order.what);
    }

    void finish()
    {
        finished_.store(true);
        cv_.notify_all();
    }

    ~WearhouseStorage()
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
