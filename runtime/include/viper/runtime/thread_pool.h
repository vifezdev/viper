#pragma once





#include <viper/common/types.h>

#include <functional>
#include <future>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace viper {

class ThreadPool {
public:
    
    explicit ThreadPool(u32 numThreads = 0);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    
    std::future<void> submit(std::function<void()> task);

    
    void shutdown();

    
    u32 size() const noexcept;

    
    u32 pendingTasks() const;

private:
    void workerLoop();

    std::vector<std::thread> m_workers;
    std::queue<std::packaged_task<void()>> m_tasks;

    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_shutdown{false};
    u32 m_numThreads;
};

} 
