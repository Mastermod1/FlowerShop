#pragma once

#include <format>
#include <iostream>
#include <mutex>

static std::mutex coutMtx;

template <typename... T>
void sprint(std::string_view fmt, const T&... args)
{
    coutMtx.lock();
    std::cout << std::vformat(fmt, std::make_format_args(args...)) << std::endl;
    coutMtx.unlock();
}
