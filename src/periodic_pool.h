#pragma once
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <string>
#include <memory>

namespace PeriodicTasks{

    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    struct PeriodicTaskDescriptor{
        using PeriodicTask = std::function<void()>;

        uint64_t        id;
        std::string     name;
        uint64_t        period;
        TimePoint       next_run;
        bool            paused;
        bool            stopped;
        PeriodicTask    action;

        int             chat_id;

        bool operator>(const PeriodicTaskDescriptor& other){
            return next_run > other.next_run;
        }

    };

    struct CompareNextRun {
        bool operator()(const std::shared_ptr<PeriodicTaskDescriptor>& a,
                        const std::shared_ptr<PeriodicTaskDescriptor>& b) const {
            return a->next_run > b->next_run;
        }
    };

    class PeriodicPool{
    public:
        PeriodicPool();
        ~PeriodicPool();

        uint64_t addTask(PeriodicTaskDescriptor&& task);
        void pauseTask(uint64_t id);
        void resumeTask(uint64_t id);
        void stopTask(uint64_t id);

        void runPool();
        void stopPool();

        size_t getTaskSize() { return m_tasks.size(); }
        bool   emptyTasks()  { return m_tasks.empty(); }

    private:
        std::thread m_worker;
        std::atomic<bool> m_stop;
        uint64_t m_next_task_id;

        std::unordered_map<uint64_t, std::shared_ptr<PeriodicTaskDescriptor>>m_tasks;
        std::priority_queue<std::shared_ptr<PeriodicTaskDescriptor>, std::vector<std::shared_ptr<PeriodicTaskDescriptor>>, CompareNextRun> m_heap;
        std::mutex m_mutex;
        std::condition_variable m_cv;
    };

};
