#include <chrono>
#include <climits>
#include <condition_variable>
#include <future>
#include <list>
#include <mutex>
#include <queue>
#include <semaphore>
#include <thread>

#include "random_number_generator.hpp"
#include "sprint.hpp"

std::atomic<int> num_of_sent_orders = 0;
std::atomic<int> num_of_received_orders = 0;

struct Order
{
    std::string what;
};

class Wearhouse
{
  public:
    Wearhouse()
    {
        // buildTrucks();
    }

    void buildTrucks()
    {
        for (int i = 0; i < 3; i++)
        {
            trucks_.push_back(std::make_unique<Truck>(dan_orders, dan_futures));
        }
    }

    ~Wearhouse()
    {
        // release trucks
        for (auto& x : trucks_) (*x).release();
        dan_orders.queue_cv_.notify_all();
        trucks_.clear();
        verificator_.release();
    }

    void close() { is_working_wearhouse_.store(false); }

    void notify(Order order)
    {
        {
            // loop read socket
            std::lock_guard<std::mutex> lock(dan_orders.queue_mtx_);
            dan_orders.orders_.push(order);
        }
        dan_orders.queue_cv_.notify_one();
    }

  private:
    struct LocalOrder
    {
        Order order;
        int estimated_time;
        int actual_delivery;
    };

    struct DanFutures
    {
        std::mutex fututres_mtx_;
        std::list<std::future<LocalOrder>> future_orders_;
    };

    struct DanOrders
    {
        // Those protect eachother
        std::queue<Order> orders_;
        std::mutex queue_mtx_;
        std::condition_variable queue_cv_;
    };

    class DeliveryVerificator
    {
      public:
        DeliveryVerificator(DanFutures& dan_futures) : dan_futures(dan_futures)
        {
            thread = std::thread(&DeliveryVerificator::task, this);
            // thread.detach();
        }

        ~DeliveryVerificator() { thread.join(); }

        void task()
        {
            while (true)
            {
                {
                    std::lock_guard<std::mutex> lock(dan_futures.fututres_mtx_);
                    if (is_finished and dan_futures.future_orders_.empty()) return;
                }

                std::list<LocalOrder> tmp;
                // polling
                {
                    std::lock_guard<std::mutex> lock(dan_futures.fututres_mtx_);
                    for (auto it = dan_futures.future_orders_.begin(); it != dan_futures.future_orders_.end();)
                    {
                        if (it->wait_for(std::chrono::milliseconds(100)) != std::future_status::ready)
                        {
                            it++;
                            continue;
                        }
                        tmp.push_back(it->get());
                        it = dan_futures.future_orders_.erase(it);
                    }
                }

                if (tmp.empty())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    continue;
                }

                for (const auto& order : tmp)
                {
                    num_of_received_orders++;
                    if (order.actual_delivery > order.estimated_time * 1.2)
                        sprint("Angry Dan: estimated: {} actual: {}", order.estimated_time, order.actual_delivery);
                    else
                        sprint("Happy Dan: estimated: {} actual: {}", order.estimated_time, order.actual_delivery);
                }
            }
        }

        void release() { is_finished.store(true); }

      private:
        DanFutures& dan_futures;
        std::thread thread;
        std::atomic<bool> is_finished = false;
    };

    class Truck
    {
      public:
        Truck(DanOrders& data, DanFutures& dan_futures) : dan_orders(data), dan_futures(dan_futures)
        {
            thread = std::thread(&Truck::work, this);
        }

        void work()
        {
            while (true)
            {
                Order order;
                std::promise<LocalOrder> orderPromise;
                {
                    std::unique_lock<std::mutex> lock(dan_orders.queue_mtx_);
                    dan_orders.queue_cv_.wait(lock,
                                              [this] { return not dan_orders.orders_.empty() or not is_working; });
                    if (dan_orders.orders_.empty() and not is_working)
                    {
                        return;
                    }
                    order = dan_orders.orders_.front();
                    dan_orders.orders_.pop();
                }

                {
                    std::lock_guard<std::mutex> lock(dan_futures.fututres_mtx_);
                    dan_futures.future_orders_.push_back(orderPromise.get_future());
                }

                const int estimated_time = RandomGenerator::generate<1000, 3000>();
                const double delay_factor = RandomGenerator::generate<0, 500>() / 1000.0;
                const int delivery_time = static_cast<int>(static_cast<double>(estimated_time) * (1 + delay_factor));
                std::this_thread::sleep_for(std::chrono::milliseconds(delivery_time));

                orderPromise.set_value(LocalOrder{order, estimated_time, delivery_time});
            }
        }

        void release() { is_working.store(false); }

        ~Truck() { thread.join(); }

      private:
        DanOrders& dan_orders;
        DanFutures& dan_futures;
        std::thread thread;
        std::atomic<bool> is_working = true;
    };

    DanOrders dan_orders;
    DanFutures dan_futures;
    std::vector<std::unique_ptr<Truck>> trucks_;
    DeliveryVerificator verificator_{dan_futures};
    std::atomic<bool> is_working_wearhouse_ = true;
};

constexpr int numberOfCashiers = 5;
constexpr int numberOfSimons = 2;

std::mutex simons_queue;
std::counting_semaphore<numberOfSimons> simons_semaphore(numberOfSimons);

class Simon
{
  public:
    Simon(std::shared_ptr<Wearhouse> wearhouse) : wearhouse_(wearhouse) {}

    void sendOrder(Order order) { wearhouse_->notify(order); }

  private:
    std::shared_ptr<Wearhouse> wearhouse_;
};

class Cashier
{
  public:
    Cashier(int id, std::queue<std::unique_ptr<Simon>>& simons) : id_(id), simons_(simons)
    {
        thread = std::thread(&Cashier::task, this);
    }

    void task()
    {
        while (is_open_)
        {
            Order order{std::format("Order from cashier: {}", id_)};
            std::this_thread::sleep_for(std::chrono::milliseconds(RandomGenerator::generate<1000, 1500>()));

            simons_semaphore.acquire();
            std::unique_ptr<Simon> simon;

            {
                std::lock_guard<std::mutex> lock(simons_queue);
                simon = std::move(simons_.front());
                simons_.pop();
            }

            sprint("Make order: {}", order.what);
            num_of_sent_orders++;
            simon->sendOrder(order);

            {
                std::lock_guard<std::mutex> lock(simons_queue);
                simons_.push(std::move(simon));
            }

            simons_semaphore.release();
        }
    }
    void end() { is_open_.store(false); }

    ~Cashier() { thread.join(); }

  private:
    int id_;
    std::queue<std::unique_ptr<Simon>>& simons_;
    std::atomic<bool> is_open_ = true;
    std::thread thread;
};

int main()
{
    {
        auto wearhouse = std::make_shared<Wearhouse>();

        std::queue<std::unique_ptr<Simon>> simons;
        for (int i = 0; i < numberOfSimons; i++)
        {
            simons.push(std::make_unique<Simon>(wearhouse));
        }
        wearhouse->buildTrucks();

        std::vector<std::unique_ptr<Cashier>> cashiers;
        for (int i = 0; i < numberOfCashiers; i++)
        {
            cashiers.push_back(std::make_unique<Cashier>(i, simons));
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
        for (int i = 0; i < numberOfCashiers; i++)
        {
            sprint("Close cashier i: {}", i);
            cashiers[i]->end();
        }
    }
    sprint("Received {} Sent {}", num_of_received_orders.load(), num_of_sent_orders.load());

    return 0;
}
