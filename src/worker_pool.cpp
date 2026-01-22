#include "worker_pool.h"
#include <iostream>

WorkerPool::WorkerPool(size_t workers)
:m_stop(false)
{
    for(auto i = 0; i < workers; ++i){
        m_workers.emplace_back([this](){
            while(true){
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(m_mutex);

                    m_cv.wait(lock);
                    
                    if(m_stop) return;

                    task = dequeue();
                }

                std::cout << "Running task from " << std::this_thread::get_id() << std::endl;
                task();
            }
        });
    }

}

WorkerPool::~WorkerPool(){

}

void WorkerPool::enqueue(Task task){

    {
        std::unique_lock<std::mutex>lock(m_mutex);
        m_tasks.emplace(std::move(task));
    }
    m_cv.notify_one();

}

inline WorkerPool::Task WorkerPool::dequeue(){
    Task task = std::move(m_tasks.front());
    m_tasks.pop();

    return task;
}

void WorkerPool::setPoolState(bool state){

}