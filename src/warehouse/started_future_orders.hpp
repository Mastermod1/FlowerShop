#pragma once

#include <future>
#include <list>
#include <mutex>

#include "warehouse/order.hpp"

struct LocalOrder
{
    Order order;
    int estimated_time;
    int actual_delivery;
};

struct StartedFutureOrders
{
    std::mutex mtx_;
    std::list<std::future<LocalOrder>> future_orders_;
};
