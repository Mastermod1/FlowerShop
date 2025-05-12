#pragma once

#include <ctime>
#include <iostream>
#include <mutex>
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
    time_t timestamp = time(NULL);
    struct tm datetime = *localtime(&timestamp);
    char output[50];
    strftime(output, 50, "%H:%M:%S", &datetime);
    ss << "[" << output << "][" << who << "] ";
    (ss << ... << args);
    log(ss.str());
}
