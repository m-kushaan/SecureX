#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "ProcessManagement.hpp"
#include "../encryptDecrypt/Cryption.hpp"


std::string getCurrentTime()
{
    auto now = std::chrono::system_clock::now();

    auto time =
        std::chrono::system_clock::to_time_t(now);

    auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ) % 1000;

    std::tm localTime;

    localtime_s(&localTime, &time);

    std::ostringstream oss;

    oss << std::put_time(
                &localTime,
                "%H:%M:%S"
            )
        << "."
        << std::setfill('0')
        << std::setw(3)
        << milliseconds.count();

    return oss.str();
}


ProcessManagement::ProcessManagement()
{
}


// =========================
// ADD TASK TO QUEUE
// =========================

bool ProcessManagement::submitToQueue(
    std::unique_ptr<Task> task
)
{
    std::lock_guard<std::mutex> lock(queueMutex);

    taskQueue.push(std::move(task));

    return true;
}


// =========================
// WORKER THREAD
// =========================

void ProcessManagement::worker(int threadId)
{
    while (true)
    {
        std::unique_ptr<Task> task;

        {
            std::lock_guard<std::mutex> lock(queueMutex);

            // No more tasks
            if (taskQueue.empty())
            {
                return;
            }

            task = std::move(taskQueue.front());

            taskQueue.pop();
        }


        // =========================
        // EXECUTE TASK
        // =========================

        std::cout
            << "["
            << getCurrentTime()
            << "] Thread "
            << threadId
            << " started | Executing: "
            << task->toString()
            << std::endl;


        executeCryption(
            task->toString()
        );


        std::cout
            << "["
            << getCurrentTime()
            << "] Thread "
            << threadId
            << " finished"
            << std::endl;
    }
}


// =========================
// EXECUTE ALL TASKS
// =========================

void ProcessManagement::executeTasks()
{
    if (taskQueue.empty())
    {
        return;
    }


    // Number of hardware threads
    unsigned int threadCount =
        std::thread::hardware_concurrency();


    // Fallback if hardware_concurrency()
    // cannot determine the number of threads

    if (threadCount == 0)
    {
        threadCount = 2;
    }


    // Don't create more threads than tasks

    if (threadCount > taskQueue.size())
    {
        threadCount =
            static_cast<unsigned int>(
                taskQueue.size()
            );
    }


    std::cout
        << "["
        << getCurrentTime()
        << "] Creating "
        << threadCount
        << " worker threads"
        << std::endl;


    std::vector<std::thread> threads;


    // =========================
    // CREATE THREADS
    // =========================

    for (unsigned int i = 0;
         i < threadCount;
         i++)
    {
        threads.emplace_back(
            &ProcessManagement::worker,
            this,
            i + 1
        );
    }


    // =========================
    // WAIT FOR THREADS
    // =========================

    for (auto& thread : threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }


    std::cout
        << "["
        << getCurrentTime()
        << "] All threads finished"
        << std::endl;
}