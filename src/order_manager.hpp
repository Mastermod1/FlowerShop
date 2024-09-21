#pragma once

#include <bits/stdc++.h>

#include "icommunication.hpp"
#include "job.hpp"

static auto get_delivery_time() { return (rand() % 15) + 5; }

class OrderManager {
public:
  using storage_t = std::queue<int>;

  OrderManager(std::unique_ptr<ICommunication> communication)
      : communication_(std::move(communication)) {}
  void run() {
    // communication_->setup();
    Job producer_job_1("producer", [this] { producer(1); });
    Job consumer_job_1("consumer1", [this] { consumer(1); });
    Job consumer_job_2("consumer2", [this] { consumer(2); });
    Job consumer_job_3("consumer3", [this] { consumer(3); });
  }

private:
  void consumer(int id) {
    for (;;) {
      std::unique_lock lck(mtx_);
      if (storage_.size() == 0)
        cv_.wait(lck,
                 [this] { return storage_.size() != 0 or stop_consumers_; });
      if (storage_.empty() and stop_consumers_)
        break;
      int consumed = storage_.front();
      storage_.pop();
      cv_.notify_all();
      std::cout << "Consumed[" << id << "]: " << (char)consumed
                << " Left In Queue: " << storage_.size() << "\n";
      lck.unlock();
      std::this_thread::sleep_for(std::chrono::seconds(get_delivery_time()));
    }
  }

  void producer(int id) {
    for (;;) {
      auto msg = communication_->read();
      std::unique_lock lck(mtx_);
      if (storage_.size() >= g_storage_size)
        cv_.wait(lck, [this] { return storage_.size() < g_storage_size; });
      for (auto &x : msg) {
        std::cout << "Produced[" << id << "]: " << x << "\n";
        storage_.push(x);
      }
      cv_.notify_all();
    }
    stop_consumers_ = true;
    cv_.notify_all();
  }

  std::unique_ptr<ICommunication> communication_;
  storage_t storage_;
  std::condition_variable cv_;
  std::mutex mtx_;
  const std::size_t g_storage_size = 100;
  bool stop_consumers_ = false;
};
