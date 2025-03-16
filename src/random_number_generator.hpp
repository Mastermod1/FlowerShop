#pragma once

#include <random>

class RandomGenerator
{
  public:
    template <int MIN, int MAX>
    static int generate()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> distrib(MIN, MAX);
        return distrib(gen);
    }
};
