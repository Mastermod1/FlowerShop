#pragma once

#include "../common/sprint.hpp"
#include "started_future_orders.hpp"

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
                    sprint("DeliveryVerificator", "Angry Dan: estimated: ", order.estimated_time,
                           " actual: ", order.actual_delivery);
                else
                    sprint("DeliveryVerificator", "Happy Dan: estimated: ", order.estimated_time,
                           " actual: ", order.actual_delivery);
            }
        }
        sprint("DeliveryVerificator", "Received in total: ", order_cntr, " orders");

        std::unique_lock<std::mutex> lock(mtx_);
        done_ = true;
        cv_.notify_one();
    }

    void release()
    {
        is_finished_.store(true);
        {
            // could be also handled with promise future
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
