#include <bits/stdc++.h>

#include "job.hpp"
#include "icommunication.hpp"
#include "socket_communication.hpp"

#ifdef DBG
constexpr bool g_debug = true;
#else
constexpr bool g_debug = true;
#endif

constexpr std::size_t g_storage_size = 100;

using storage_t = std::queue<int>;

std::condition_variable g_cv;
std::mutex g_mtx;
bool stop_consumers = false;

auto get_delivery_time() { return (rand() % 15) + 5; }

void producer(storage_t &storage, int id,
              std::unique_ptr<ICommunication> communication) {
  for (;;) {
    auto msg = communication->read();
    if (msg == g_stop_msg) {
      break;
    }
    std::unique_lock lck(g_mtx);
    if (storage.size() >= g_storage_size)
      g_cv.wait(lck, [&storage] { return storage.size() < g_storage_size; });
    if constexpr (g_debug)
      std::cout << "Produced[" << id << "]: " << msg << "\n";
    storage.push(msg);
    g_cv.notify_all();
  }
  stop_consumers = true;
  g_cv.notify_all();
}

void consumer(storage_t &storage, int id) {
  for (;;) {
    std::unique_lock lck(g_mtx);
    if (storage.size() == 0)
      g_cv.wait(lck,
                [&storage] { return storage.size() != 0 or stop_consumers; });
    if (storage.empty() and stop_consumers)
      break;
    int consumed = storage.front();
    storage.pop();
    g_cv.notify_all();
    if constexpr (g_debug)
      std::cout << "Consumed[" << id << "]: " << (char)consumed
                << " Left In Queue: " << storage.size() << "\n";
    lck.unlock();
    std::this_thread::sleep_for(std::chrono::seconds(get_delivery_time()));
  }
}

int main() {
  srand(time(0));
  storage_t storage;
  auto socket = std::make_unique<SocketCommunication>();
  socket->init();
  socket->waitForClient();
  Job producer_job_1("producer", producer, std::ref(storage), 1,
                     std::move(socket));
  Job consumer_job_1("consumer1", consumer, std::ref(storage), 1);
  Job consumer_job_2("consumer2", consumer, std::ref(storage), 2);
  Job consumer_job_3("consumer3", consumer, std::ref(storage), 3);
}
