#include "shop/shop.hpp"

int main()
{
    {
        Shop shop;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return 0;
}
