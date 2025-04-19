#include <chrono>
#include <csignal>
#include <cstdlib>

#include "shop/shop.hpp"
#include "common/sprint.hpp"

bool is_finished = false;

void signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        sprint("ShopMain", "Caught SIGINT (Ctrl+C). Cleaning up and exiting...");
    }
    else if (signal == SIGTERM)
    {
        sprint("ShopMain", "Caught SIGTERM. Cleaning up and exiting...");
    }

    is_finished = true;
}

int main()
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    Shop shop;
    while (not is_finished)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
