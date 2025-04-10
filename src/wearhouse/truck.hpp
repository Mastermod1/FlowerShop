#pragma once

#include "wearhouse_storage.hpp"
#include "../common/random_number_generator.hpp"
#include <future>
#include "started_future_orders.hpp"

class Truck
{
  public:
    Truck(WearhouseStorage& storage, StartedFutureOrders& future_orders)
        : storage_(storage), future_orders_(future_orders)
    {
        thread_ = std::thread(&Truck::work, this);
    }

    ~Truck()
    {
        if (thread_.joinable()) thread_.join();
    }

    void work()
    {
        while (true)
        {
            std::optional<Order> order = storage_.getNext();
            if (not order.has_value())
            {
                return;
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

            order_promise.set_value(LocalOrder{order.value(), estimated_time, delivery_time});
        }
    }

  private:
    WearhouseStorage& storage_;
    StartedFutureOrders& future_orders_;
    std::thread thread_;
};
