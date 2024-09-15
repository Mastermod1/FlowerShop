#include <bits/stdc++.h>

#ifdef DBG
constexpr bool g_debug = true;
#else
constexpr bool g_debug = false;
#endif

constexpr std::size_t g_data_size = 20'000;
constexpr std::size_t g_storage_size = 100;

using storage_t = std::queue<int>;

std::condition_variable g_cv;
std::mutex g_mtx;

std::atomic<int> g_consumed_count = 0;
std::atomic<int> g_produced_count = 0;

auto random_production_time()
{
    return (rand()%6)+1;
}

auto random_consumption_time()
{
    return (rand()%30)+10;
}

void producer(storage_t& storage, int id)
{
    while(g_produced_count < g_data_size)
    {
        std::unique_lock lck(g_mtx);
        if (storage.size() >= g_storage_size)
            g_cv.wait(lck, [&storage]{ return storage.size() < g_storage_size; });
        storage.push(10);
        g_produced_count++;
        if constexpr (g_debug)
            std::cout << "Produced[" << id << "]: " << storage.front() << "\n";
        g_cv.notify_all();
        lck.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(random_production_time()));
    }
}

void consumer(storage_t& storage, int id)
{
    while(g_consumed_count < g_data_size)
    {
        std::unique_lock lck(g_mtx);
        if (storage.size() == 0)
            g_cv.wait(lck, [&storage]{ return storage.size() != 0; });
        int consumed = storage.front();
        g_consumed_count++;
        assert(consumed != -1);
        storage.pop();
        g_cv.notify_all();
        if constexpr (g_debug)
            std::cout << "Consumed[" << id << "]: " << consumed << "\n";
        lck.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(random_consumption_time()));
    }
}

int main()
{
    srand(time(0)); 
    storage_t storage;
    std::thread producer_thread_1(producer, std::ref(storage), 1);
    std::thread consumer_thread_1(consumer, std::ref(storage), 1);
    std::thread consumer_thread_2(consumer, std::ref(storage), 2);
    std::thread consumer_thread_3(consumer, std::ref(storage), 3);
    producer_thread_1.join();
    consumer_thread_1.join();
    consumer_thread_2.join();
    consumer_thread_3.join();
    std::cout << "Finished\n";
}
