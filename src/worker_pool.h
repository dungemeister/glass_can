#pragma once

#include <functional>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>

class WorkerPool{
public:
    using Task = std::function<void()>;

    WorkerPool(size_t workers);
    ~WorkerPool();
    
    void enqueue(Task task);
    Task dequeue();

    void stop();
    void setPoolState(bool state);

    size_t size() { return m_tasks.size(); }
    bool empty()  { return m_tasks.empty(); }
    
private:
    std::vector<std::thread> m_workers;
    std::queue<Task> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stop;
};