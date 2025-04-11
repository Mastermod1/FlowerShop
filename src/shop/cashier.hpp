#pragma once

#include <memory>
#include <thread>
#include <atomic>

#include "common/random_number_generator.hpp"
#include "common/sprint.hpp"
#include "shop/simons_queue.hpp"

class Cashier
{
  public:
    Cashier(int id, SimonsQueue<2>& simons) : id_(id), simons_(simons) { thread_ = std::thread(&Cashier::task, this); }

    void task()
    {
        while (is_open_)
        {
            Order order{"Order from cashier: " + std::to_string(id_)};
            std::this_thread::sleep_for(std::chrono::milliseconds(RandomGenerator::generate<1000, 1500>()));

            simons_.callSimon([&order](std::unique_ptr<Simon>& simon) { simon->sendOrder(order); });

            sprint("Cashier", "Make order: ", order.what);
        }
    }

    ~Cashier()
    {
        // interestingly it's also faster than looping over all cashiers and calling false
        is_open_.store(false);
        if (thread_.joinable()) thread_.join();
    }

  private:
    int id_;
    SimonsQueue<2>& simons_;
    std::atomic<bool> is_open_ = true;
    std::thread thread_;
};
