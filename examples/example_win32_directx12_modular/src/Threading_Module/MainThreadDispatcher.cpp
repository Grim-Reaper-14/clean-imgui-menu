#include "MainThreadDispatcher.hpp"

namespace Threading
{
void MainThreadDispatcher::Post(Task task)
{
    std::scoped_lock lock(m_mutex);
    m_tasks.push(std::move(task));
}

void MainThreadDispatcher::Drain()
{
    std::queue<Task> pending;
    {
        std::scoped_lock lock(m_mutex);
        std::swap(pending, m_tasks);
    }
    while (!pending.empty())
    {
        auto task = std::move(pending.front());
        pending.pop();
        if (task) task();
    }
}
}
