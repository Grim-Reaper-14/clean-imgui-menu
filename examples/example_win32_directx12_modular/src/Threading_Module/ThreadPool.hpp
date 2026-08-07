#pragma once

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace Threading
{
class ThreadPool
{
public:
    using Task = std::function<void()>;

    explicit ThreadPool(std::size_t workerCount = DefaultWorkerCount());
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename Fn>
    auto Enqueue(Fn&& fn) -> std::future<std::invoke_result_t<Fn>>
    {
        using Result = std::invoke_result_t<Fn>;
        auto work = std::make_shared<std::packaged_task<Result()>>(std::forward<Fn>(fn));
        auto future = work->get_future();
        {
            std::scoped_lock lock(m_mutex);
            if (m_stopping) throw std::runtime_error("ThreadPool is stopping");
            m_tasks.emplace([work] { (*work)(); });
        }
        m_cv.notify_one();
        return future;
    }

    void Stop();
    static std::size_t DefaultWorkerCount();

private:
    void WorkerLoop(std::stop_token stopToken);

    std::mutex m_mutex;
    std::condition_variable_any m_cv;
    std::queue<Task> m_tasks;
    std::vector<std::jthread> m_workers;
    bool m_stopping = false;
};
}
