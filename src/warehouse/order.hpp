#pragma once

#include <future>
#include <string>

class Order
{
  public:
    Order(std::string s) : what(s) {};
    Order(Order&) = delete;
    Order& operator=(Order&) = delete;

    Order(Order&& mv) noexcept
        : what(std::move(mv.what)),
          promise_(std::move(mv.promise_)),
          time(mv.time),
          moved_(false)  // mark this as valid
    {
        mv.moved_ = true;
    }

    Order& operator=(Order&& mv) noexcept
    {
        if (this != &mv)
        {
            what = std::move(mv.what);
            promise_ = std::move(mv.promise_);
            time = mv.time;
            moved_ = false;  // mark this as valid
            mv.moved_ = true;
        }
        return *this;
    }

    std::string what;
    std::promise<bool> promise_;
    int time = 0;

  private:
    bool moved_ = false;
};
