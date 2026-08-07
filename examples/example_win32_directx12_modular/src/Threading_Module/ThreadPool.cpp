#include "ThreadPool.hpp"

namespace Threading
{
std::size_t ThreadPool::DefaultWorkerCount()
{
    const auto hardware = std::thread::hardware_concurrency();
    return std::max<std::size_t>(1, hardware > 1 ? hardware - 1 : 1);
}

ThreadPool::ThreadPool(std::size_t workerCount)
{
    m_workers.reserve(workerCount);
    for (std::size_t i = 0; i < workerCount; ++i)
        m_workers.emplace_back([this](std::stop_token token) { WorkerLoop(token); });
}

ThreadPool::~ThreadPool() { Stop(); }

void ThreadPool::Stop()
{
    {
        std::scoped_lock lock(m_mutex);
        if (m_stopping) return;
        m_stopping = true;
    }
    for (auto& worker : m_workers) worker.request_stop();
    m_cv.notify_all();
    m_workers.clear();
}

void ThreadPool::WorkerLoop(std::stop_token stopToken)
{
    while (!stopToken.stop_requested())
    {
        Task task;
        {
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, stopToken, [this] { return m_stopping || !m_tasks.empty(); });
            if ((m_stopping || stopToken.stop_requested()) && m_tasks.empty()) return;
            if (m_tasks.empty()) continue;
            task = std::move(m_tasks.front());
            m_tasks.pop();
        }
        if (task) task();
    }
}
}
