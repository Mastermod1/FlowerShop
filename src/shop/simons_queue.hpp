#pragma once

#include <functional>
#include <memory>
#include <queue>
#include <semaphore>

#include "shop/simon.hpp"

template <std::size_t SIZE>
class SimonsQueue
{
  public:
    SimonsQueue()
    {
        for (auto i = 0; i < SIZE; i++)
        {
            simons_.push(std::make_unique<Simon>());
        }
    }

    void callSimon(std::function<void(std::unique_ptr<Simon>&)> callback)
    {
        simons_semaphore_.acquire();
        std::unique_ptr<Simon> simon;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            simon = std::move(simons_.front());
            simons_.pop();
        }

        callback(simon);

        {
            std::lock_guard<std::mutex> lock(mtx_);
            simons_.push(std::move(simon));
        }
        simons_semaphore_.release();
    }

  private:
    std::queue<std::unique_ptr<Simon>> simons_;
    std::mutex mtx_;
    std::counting_semaphore<SIZE> simons_semaphore_{SIZE};
};
