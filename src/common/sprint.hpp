#pragma once

#include <fstream>
#include <mutex>

extern std::string LOG_FILE;

static std::mutex coutMtx;

static std::ostream& getFile()
{
    static std::ofstream logfile(LOG_FILE, std::ios::app);
    return logfile;
}

template <typename... T>
void sprint(std::string who, T... args)
{
    std::lock_guard<std::mutex> lock(coutMtx);
    auto& logfile = getFile();
    logfile << "[" << who << "] ";
    (logfile << ... << args);
    logfile << std::endl;
    logfile.flush();
}
