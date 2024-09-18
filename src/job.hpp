#pragma once

#include <string>
#include <thread>
#include <iostream>

class Job
{
public:
    template<typename... Args>
    Job(std::string job_name, Args&&... args) : job_name_(std::move(job_name)), thread_(std::forward<Args>(args)...) {}

    ~Job()
    {
        thread_.join();
        std::cout << "Job \"" << job_name_ << "\" has been finished\n";
    }
private:
    std::string job_name_;
    std::thread thread_;
};

