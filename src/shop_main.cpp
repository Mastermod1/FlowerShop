#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <climits>
#include <iostream>
#include <mutex>
#include <queue>
#include <semaphore>
#include <thread>
#include <vector>

#include "common/random_number_generator.hpp"
#include "common/sprint.hpp"

#define PORT 8080

std::string LOG_FILE = "log_shop.txt";
const std::string LOG_TITLE = "shop_main.cpp";
const std::string IP_ADDR = "127.0.0.1";
constexpr int numberOfCashiers = 5;
constexpr int numberOfSimons = 2;

std::mutex simons_queue;
std::counting_semaphore<numberOfSimons> simons_semaphore(numberOfSimons);

struct Order
{
    std::string what;
};

class Simon
{
  public:
    Simon()
    {
        sock_ = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        inet_pton(AF_INET, IP_ADDR.c_str(), &server_addr.sin_addr);

        if (connect(sock_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        {
            std::cerr << "Connection failed\n";
            exit(-1);
        }
    }

    ~Simon()
    {
        close(sock_);
        sprint(LOG_TITLE, "Simon sent ", sent_msgs_, " messages");
    }

    void sendOrder(Order order)
    {
        send(sock_, order.what.c_str(), order.what.length(), 0);
        sent_msgs_++;
    }

  private:
    int sent_msgs_ = 0;
    int sock_;
};

class Cashier
{
  public:
    Cashier(int id, std::queue<std::unique_ptr<Simon>>& simons) : id_(id), simons_(simons)
    {
        thread_ = std::thread(&Cashier::task, this);
    }

    void task()
    {
        while (is_open_)
        {
            Order order{"Order from cashier: " + std::to_string(id_)};
            std::this_thread::sleep_for(std::chrono::milliseconds(RandomGenerator::generate<1000, 1500>()));

            simons_semaphore.acquire();
            std::unique_ptr<Simon> simon;

            {
                std::lock_guard<std::mutex> lock(simons_queue);
                simon = std::move(simons_.front());
                simons_.pop();
            }

            simon->sendOrder(order);

            {
                std::lock_guard<std::mutex> lock(simons_queue);
                simons_.push(std::move(simon));
            }

            simons_semaphore.release();

            sprint(LOG_TITLE, "Make order: ", order.what);
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
    std::queue<std::unique_ptr<Simon>>& simons_;
    std::atomic<bool> is_open_ = true;
    std::thread thread_;
};

int main()
{
    std::queue<std::unique_ptr<Simon>> simons;
    for (int i = 0; i < numberOfSimons; i++)
    {
        simons.push(std::make_unique<Simon>());
    }

    std::vector<std::unique_ptr<Cashier>> cashiers;
    for (int i = 0; i < numberOfCashiers; i++)
    {
        cashiers.push_back(std::make_unique<Cashier>(i, simons));
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}
