#pragma once

#include <mutex>
#include <iostream>
#include <sstream>

static void log(std::string msg)
{
    static std::mutex coutMtx;
    std::lock_guard<std::mutex> lock(coutMtx);
    std::cout << msg << std::endl;
    std::cout.flush();
}

template <typename... T>
void sprint(std::string who, T... args)
{
    std::stringstream ss;
    ss << "[" << who << "] ";
    (ss << ... << args);
    log(ss.str());
}
