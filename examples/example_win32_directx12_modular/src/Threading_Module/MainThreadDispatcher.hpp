#pragma once

#include <functional>
#include <mutex>
#include <queue>

namespace Threading
{
class MainThreadDispatcher
{
public:
    using Task = std::function<void()>;
    void Post(Task task);
    void Drain();

private:
    std::mutex m_mutex;
    std::queue<Task> m_tasks;
};
}
