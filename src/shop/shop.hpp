#pragma once

#include <vector>

#include "shop/cashier.hpp"
#include "shop/simons_queue.hpp"

constexpr int numberOfCashiers = 5;
constexpr int numberOfSimons = 2;

class Shop
{
  public:
    Shop()
    {
        for (int i = 0; i < numberOfCashiers; i++)
        {
            cashiers_.push_back(std::make_unique<Cashier>(i, simons_queue_, is_open_));
        }
    }

    ~Shop() { is_open_.store(false); }

  private:
    SimonsQueue<numberOfSimons> simons_queue_;
    std::vector<std::unique_ptr<Cashier>> cashiers_;
    std::atomic<bool> is_open_ = true;
};
