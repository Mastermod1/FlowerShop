#pragma once

#include <fstream>
#include <mutex>

extern std::string LOG_FILE;

static std::ofstream logfile(LOG_FILE, std::ios::app);

static std::mutex coutMtx;

template <typename... T>
void sprint(std::string who, T... args)
{
    std::lock_guard<std::mutex> lock(coutMtx);
    logfile << "[" << who << "] ";
    (logfile << ... << args);
    logfile << std::endl;
    logfile.flush();
}
