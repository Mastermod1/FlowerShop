#include <chrono>
#include <csignal>
#include <cstdlib>

#include "warehouse/warehouse.hpp"
#include "common/sprint.hpp"

std::atomic<bool> is_finished = false;

void signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        sprint("WarehouseMain", "Caught SIGINT (Ctrl+C). Cleaning up and exiting...");
    }
    else if (signal == SIGTERM)
    {
        sprint("WarehouseMain", "Caught SIGTERM. Cleaning up and exiting...");
    }

    is_finished = true;
}

int main()
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    Warehouse warehouse;
    while (not is_finished)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
