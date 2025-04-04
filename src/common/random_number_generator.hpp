#pragma once

#include <random>

class RandomGenerator
{
  public:
    template <int MIN, int MAX>
    static int generate()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(MIN, MAX);
        return distrib(gen);
    }
};
