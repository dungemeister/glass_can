#include "periodic_pool.h"
#include "iostream"

PeriodicTasks::PeriodicPool::PeriodicPool()
:m_stop(false)

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
        auto heap_size = m_heap.size();
        if(closest_task->next_run > now){
            m_cv.wait_until(lock, closest_task->next_run, [this, heap_size]{ return m_stop.load() | heap_size != m_heap.size(); });
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
            try{

                t->action();
            }
            catch(std::exception& e){
                std::cout << "ERROR (" << t->name << "): " << e.what() << std::endl;
                t->stopped = true;
            }
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

PeriodicTasks::task_id_hash PeriodicTasks::PeriodicPool::addTask(PeriodicTaskDescriptor&& task){
    std::unique_lock lock(m_mutex);
    task_id_hash id = task.id;
    m_tasks[id] = std::make_shared<PeriodicTaskDescriptor>(task);
    m_heap.push(m_tasks[id]);
    m_cv.notify_one();
    return id;
}

void PeriodicTasks::PeriodicPool::pauseTask(task_id_hash id){
    std::unique_lock lock (m_mutex);

    auto it = m_tasks.find(id);
    if(it != m_tasks.end()){
        it->second->paused = true;
    }
    m_cv.notify_one();
}

void PeriodicTasks::PeriodicPool::resumeTask(task_id_hash id){
    std::unique_lock lock(m_mutex);
    auto it = m_tasks.find(id);
    if(it != m_tasks.end()){
        it->second->paused = false;
    }
    m_cv.notify_one();
}

void PeriodicTasks::PeriodicPool::stopTask(task_id_hash id){
    std::unique_lock lock(m_mutex);
    auto it = m_tasks.find(id);
    if(it != m_tasks.end()){
        it->second->stopped = true;
    }
    m_cv.notify_one();
}

void PeriodicTasks::PeriodicPool::deleteTask(task_id_hash id){
    std::unique_lock lock(m_mutex);
    auto it = m_tasks.find(id);
    if(it != m_tasks.end()){
        m_tasks.erase(it);
    }
    m_cv.notify_one();
}