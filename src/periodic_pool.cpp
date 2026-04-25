#include "periodic_pool.h"
#include "iostream"

PeriodicTasks::PeriodicPool::PeriodicPool()
:m_stop(false)
,m_next_task_id(0)

{
    m_worker = std::thread(&PeriodicTasks::PeriodicPool::runPool, this);
}

PeriodicTasks::PeriodicPool::~PeriodicPool(){
    stopPool();
    if(m_worker.joinable()) m_worker.join();

}

void PeriodicTasks::PeriodicPool::runPool(){


    while(true){
        std::unique_lock lock(m_mutex);
        if(m_stop) break;
        
        if(m_heap.empty()){
            m_cv.wait(lock, [this]{ return m_stop || !m_heap.empty(); });

            if(m_stop) break;
        }

        auto closest_task = m_heap.top();
        auto now = Clock::now();
        if(closest_task->next_run > now){
            m_cv.wait_until(lock, closest_task->next_run, [this]{ return m_stop.load(); });
            continue;
        }


        std::vector<std::shared_ptr<PeriodicTaskDescriptor>> ready;
        ready.reserve(m_heap.size());
        
        while(!m_heap.empty() && m_heap.top()->next_run <= now){
            auto task = m_heap.top();
            m_heap.pop();

            if(!task->stopped && !task->paused){
                ready.push_back(task);
            }

            if(!task->stopped && !task->paused){
                task->next_run = now + std::chrono::milliseconds(task->period);
                m_heap.push(task);
            }
        }

        lock.unlock();
        for(auto t: ready){
            t->action();
        }

    }
}

void PeriodicTasks::PeriodicPool::stopPool(){
    {
        std::unique_lock lock(m_mutex);
        m_stop.store(true);
    }

    m_cv.notify_one();
}

uint64_t PeriodicTasks::PeriodicPool::addTask(PeriodicTaskDescriptor&& task){
    std::unique_lock lock(m_mutex);
    uint64_t id = m_next_task_id++;
    task.id = id;
    m_tasks[id] = std::make_shared<PeriodicTaskDescriptor>(task);
    m_heap.push(m_tasks[id]);
    m_cv.notify_one();
    return id;
}

void PeriodicTasks::PeriodicPool::pauseTask(uint64_t id){
    std::unique_lock lock (m_mutex);

    auto it = m_tasks.find(id);
    if(it != m_tasks.end()){
        it->second->paused = true;
    }
    m_cv.notify_one();
}