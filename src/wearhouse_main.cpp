#include <chrono>
#include <csignal>
#include <cstdlib>

#include "wearhouse/wearhouse.hpp"

bool is_finished = false;

void signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        std::cout << "\nCaught SIGINT (Ctrl+C). Cleaning up and exiting..." << std::endl;
    }
    else if (signal == SIGTERM)
    {
        std::cout << "\nCaught SIGTERM. Cleaning up and exiting..." << std::endl;
    }

    is_finished = true;
}

int main()
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    Wearhouse wearhouse;
    while (not is_finished)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
