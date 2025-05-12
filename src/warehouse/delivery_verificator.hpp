#pragma once

#include <future>

#include "common/sprint.hpp"

class DeliveryVerificator
{
  public:
    DeliveryVerificator(std::future<bool>&& future, int estimated_time)
    {
        thread_ = std::thread(&DeliveryVerificator::task, this, std::move(future), estimated_time);
        thread_.detach();
    }

    void task(std::future<bool> future, int time)
    {
        if (future.wait_for(std::chrono::milliseconds(static_cast<int>((double)time * 1.2))) ==
            std::future_status::ready)
        {
            sprint("DeliveryVerificator", "Happy Dan");
        }
        else
        {
            sprint("DeliveryVerificator", "Angry Dan");
        }
    }

  private:
    std::thread thread_;
};
