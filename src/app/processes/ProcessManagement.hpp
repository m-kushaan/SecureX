#ifndef PROCESS_MANAGEMENT_HPP
#define PROCESS_MANAGEMENT_HPP

#include "Task.hpp"

#include <windows.h>
#include <memory>
#include <vector>

class ProcessManagement
{
public:
    ProcessManagement(bool worker = false);
    ~ProcessManagement();

    bool submitToQueue(std::unique_ptr<Task> task);

    // Used by worker process
    void executeTask();

    // Used by parent process
    void executeTasks();

private:

    struct SharedMemory
    {
        LONG size;
        LONG front;
        LONG rear;

        char tasks[1000][256];
    };

    SharedMemory* sharedMem;

    HANDLE sharedMemoryHandle;
    HANDLE itemsSemaphore;
    HANDLE emptySlotsSemaphore;
    HANDLE queueMutex;

    bool workerMode;

    std::vector<HANDLE> childProcesses;
};

#endif