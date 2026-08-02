



#include <viper/runtime/thread_pool.h>
#include <algorithm>

namespace viper {

ThreadPool::ThreadPool(u32 numThreads) {
    if (numThreads == 0) {
        numThreads = std::max(2u, std::thread::hardware_concurrency() / 2);
    }
    m_numThreads = numThreads;

    m_workers.reserve(numThreads);
    for (u32 i = 0; i < numThreads; ++i) {
        m_workers.emplace_back(&ThreadPool::workerLoop, this);
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

std::future<void> ThreadPool::submit(std::function<void()> task) {
    std::packaged_task<void()> pt(std::move(task));
    auto future = pt.get_future();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_shutdown.load()) {
            return future;  
        }
        m_tasks.push(std::move(pt));
    }
    m_condition.notify_one();

    return future;
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_shutdown.load()) return;
        m_shutdown.store(true);
    }
    m_condition.notify_all();

    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    m_workers.clear();
}

u32 ThreadPool::size() const noexcept {
    return m_numThreads;
}

u32 ThreadPool::pendingTasks() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<u32>(m_tasks.size());
}

void ThreadPool::workerLoop() {
    while (true) {
        std::packaged_task<void()> task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [this]() {
                return m_shutdown.load() || !m_tasks.empty();
            });

            if (m_shutdown.load() && m_tasks.empty()) {
                return;
            }

            task = std::move(m_tasks.front());
            m_tasks.pop();
        }

        task();
    }
}

} 
