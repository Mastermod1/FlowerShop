#pragma once

#include <vector>

#include "simons_queue.hpp"
#include "cashier.hpp"

constexpr int numberOfCashiers = 5;
constexpr int numberOfSimons = 2;

class Shop
{
  public:
    Shop()
    {
        for (int i = 0; i < numberOfCashiers; i++)
        {
            cashiers_.push_back(std::make_unique<Cashier>(i, simons_queue_));
        }
    }

  private:
    SimonsQueue<numberOfSimons> simons_queue_;
    std::vector<std::unique_ptr<Cashier>> cashiers_;
};
