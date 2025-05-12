#pragma once

#include <future>

#include "warehouse/warehouse_storage.hpp"

class Truck
{
  public:
    Truck(WarehouseStorage& storage)
        : storage_(storage)
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

            const double delay_factor = RandomGenerator::generate<0, 500>() / 1000.0;
            const int delivery_time = static_cast<int>(static_cast<double>(order->time) * (1 + delay_factor));
            std::this_thread::sleep_for(std::chrono::milliseconds(delivery_time));
            order->promise_.set_value(true);
        }
    }

  private:
    WarehouseStorage& storage_;
    std::thread thread_;
};
