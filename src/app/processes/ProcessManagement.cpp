#include <iostream>
#include <windows.h>

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "ProcessManagement.hpp"
#include "../encryptDecrypt/Cryption.hpp"


#define SHM_NAME "SecureXSharedMemory"
#define ITEMS_SEMAPHORE_NAME "SecureXItemsSemaphore"
#define EMPTY_SLOTS_SEMAPHORE_NAME "SecureXEmptySlotsSemaphore"
#define QUEUE_MUTEX_NAME "SecureXQueueMutex"


// =========================
// GET CURRENT TIME
// =========================

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


// =========================
// CONSTRUCTOR
// =========================

ProcessManagement::ProcessManagement(bool worker)
{
    workerMode = worker;

    sharedMem = nullptr;
    sharedMemoryHandle = nullptr;
    itemsSemaphore = nullptr;
    emptySlotsSemaphore = nullptr;
    queueMutex = nullptr;


    // =========================
    // WORKER PROCESS
    // =========================

    if (workerMode)
    {
        sharedMemoryHandle = OpenFileMappingA(
            FILE_MAP_ALL_ACCESS,
            FALSE,
            SHM_NAME
        );

        if (sharedMemoryHandle == nullptr)
        {
            std::cerr
                << "Worker failed to open shared memory. Error: "
                << GetLastError()
                << std::endl;

            return;
        }


        sharedMem =
            static_cast<SharedMemory*>(
                MapViewOfFile(
                    sharedMemoryHandle,
                    FILE_MAP_ALL_ACCESS,
                    0,
                    0,
                    sizeof(SharedMemory)
                )
            );


        if (sharedMem == nullptr)
        {
            std::cerr
                << "Worker failed to map shared memory. Error: "
                << GetLastError()
                << std::endl;

            CloseHandle(sharedMemoryHandle);
            sharedMemoryHandle = nullptr;

            return;
        }


        itemsSemaphore = OpenSemaphoreA(
            SEMAPHORE_ALL_ACCESS,
            FALSE,
            ITEMS_SEMAPHORE_NAME
        );


        emptySlotsSemaphore = OpenSemaphoreA(
            SEMAPHORE_ALL_ACCESS,
            FALSE,
            EMPTY_SLOTS_SEMAPHORE_NAME
        );


        queueMutex = OpenMutexA(
            MUTEX_ALL_ACCESS,
            FALSE,
            QUEUE_MUTEX_NAME
        );


        if (itemsSemaphore == nullptr ||
            emptySlotsSemaphore == nullptr ||
            queueMutex == nullptr)
        {
            std::cerr
                << "Worker failed to open synchronization objects."
                << std::endl;
        }

        return;
    }


    // =========================
    // PARENT PROCESS
    // =========================

    sharedMemoryHandle = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        sizeof(SharedMemory),
        SHM_NAME
    );


    if (sharedMemoryHandle == nullptr)
    {
        std::cerr
            << "Failed to create shared memory. Error: "
            << GetLastError()
            << std::endl;

        return;
    }


    sharedMem =
        static_cast<SharedMemory*>(
            MapViewOfFile(
                sharedMemoryHandle,
                FILE_MAP_ALL_ACCESS,
                0,
                0,
                sizeof(SharedMemory)
            )
        );


    if (sharedMem == nullptr)
    {
        std::cerr
            << "Failed to map shared memory. Error: "
            << GetLastError()
            << std::endl;

        CloseHandle(sharedMemoryHandle);
        sharedMemoryHandle = nullptr;

        return;
    }


    // Initialize shared queue

    sharedMem->front = 0;
    sharedMem->rear = 0;
    sharedMem->size = 0;


    // =========================
    // SEMAPHORES / MUTEX
    // =========================

    itemsSemaphore = CreateSemaphoreA(
        nullptr,
        0,
        1000,
        ITEMS_SEMAPHORE_NAME
    );


    emptySlotsSemaphore = CreateSemaphoreA(
        nullptr,
        1000,
        1000,
        EMPTY_SLOTS_SEMAPHORE_NAME
    );


    queueMutex = CreateMutexA(
        nullptr,
        FALSE,
        QUEUE_MUTEX_NAME
    );


    if (itemsSemaphore == nullptr ||
        emptySlotsSemaphore == nullptr ||
        queueMutex == nullptr)
    {
        std::cerr
            << "Failed to create synchronization objects."
            << std::endl;
    }
}


// =========================
// DESTRUCTOR
// =========================

ProcessManagement::~ProcessManagement()
{
    if (sharedMem != nullptr)
    {
        UnmapViewOfFile(sharedMem);
        sharedMem = nullptr;
    }


    if (sharedMemoryHandle != nullptr)
    {
        CloseHandle(sharedMemoryHandle);
        sharedMemoryHandle = nullptr;
    }


    if (itemsSemaphore != nullptr)
    {
        CloseHandle(itemsSemaphore);
        itemsSemaphore = nullptr;
    }


    if (emptySlotsSemaphore != nullptr)
    {
        CloseHandle(emptySlotsSemaphore);
        emptySlotsSemaphore = nullptr;
    }


    if (queueMutex != nullptr)
    {
        CloseHandle(queueMutex);
        queueMutex = nullptr;
    }
}


// =========================
// SUBMIT TASK
// =========================

bool ProcessManagement::submitToQueue(
    std::unique_ptr<Task> task
)
{
    // Wait for an empty slot

    WaitForSingleObject(
        emptySlotsSemaphore,
        INFINITE
    );


    // Lock queue

    WaitForSingleObject(
        queueMutex,
        INFINITE
    );


    // Put task into shared memory

    strcpy_s(
        sharedMem->tasks[sharedMem->rear],
        256,
        task->toString().c_str()
    );


    sharedMem->rear =
        (sharedMem->rear + 1) % 1000;


    InterlockedIncrement(
        &sharedMem->size
    );


    // Unlock queue

    ReleaseMutex(queueMutex);


    // Tell worker that a task is available

    ReleaseSemaphore(
        itemsSemaphore,
        1,
        nullptr
    );


    // =========================
    // CREATE WORKER PROCESS
    // =========================

    char exePath[MAX_PATH];

    DWORD pathLength =
        GetModuleFileNameA(
            nullptr,
            exePath,
            MAX_PATH
        );


    if (pathLength == 0)
    {
        std::cerr
            << "Failed to get executable path. Error: "
            << GetLastError()
            << std::endl;

        return false;
    }


    std::string command =
        "\"" +
        std::string(exePath) +
        "\" --worker";


    STARTUPINFOA si{};

    PROCESS_INFORMATION pi{};

    si.cb = sizeof(si);


    // CreateProcessA may modify the command buffer,
    // so we provide a writable character buffer.

    std::vector<char> commandBuffer(
        command.begin(),
        command.end()
    );

    commandBuffer.push_back('\0');


    BOOL success = CreateProcessA(
        exePath,
        commandBuffer.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    );


    if (!success)
    {
        std::cerr
            << "Failed to create child process. Error: "
            << GetLastError()
            << std::endl;

        return false;
    }


    std::cout
        << "["
        << getCurrentTime()
        << "] Child process created | PID: "
        << pi.dwProcessId
        << std::endl;


    // Store process handle.
    // Parent waits for it later.

    childProcesses.push_back(
        pi.hProcess
    );


    // Parent doesn't need the thread handle.

    CloseHandle(pi.hThread);


    return true;
}


// =========================
// WORKER EXECUTION
// =========================

void ProcessManagement::executeTask()
{
    if (!workerMode)
    {
        return;
    }


    // =========================
    // WAIT FOR TASK
    // =========================

    DWORD result =
        WaitForSingleObject(
            itemsSemaphore,
            INFINITE
        );


    if (result != WAIT_OBJECT_0)
    {
        std::cerr
            << "Worker failed while waiting for task."
            << std::endl;

        return;
    }


    // =========================
    // LOCK QUEUE
    // =========================

    WaitForSingleObject(
        queueMutex,
        INFINITE
    );


    char taskStr[256];


    // Take task from shared queue

    strcpy_s(
        taskStr,
        256,
        sharedMem->tasks[sharedMem->front]
    );


    sharedMem->front =
        (sharedMem->front + 1) % 1000;


    InterlockedDecrement(
        &sharedMem->size
    );


    // Unlock queue

    ReleaseMutex(queueMutex);


    // One more empty slot is available

    ReleaseSemaphore(
        emptySlotsSemaphore,
        1,
        nullptr
    );


    // =========================
    // EXECUTE TASK
    // =========================

    std::cout
        << "["
        << getCurrentTime()
        << "] Child process | PID: "
        << GetCurrentProcessId()
        << " | Executing: "
        << taskStr
        << std::endl;


    executeCryption(taskStr);


    std::cout
        << "["
        << getCurrentTime()
        << "] Child process | PID: "
        << GetCurrentProcessId()
        << " | Finished"
        << std::endl;
}


// =========================
// WAIT FOR CHILDREN
// =========================

void ProcessManagement::executeTasks()
{
    if (workerMode)
    {
        return;
    }


    // Parent waits for every child

    for (HANDLE child : childProcesses)
    {
        WaitForSingleObject(
            child,
            INFINITE
        );

        CloseHandle(child);
    }


    childProcesses.clear();


    std::cout
        << "["
        << getCurrentTime()
        << "] All child processes finished"
        << std::endl;
}