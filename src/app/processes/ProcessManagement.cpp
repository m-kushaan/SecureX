#include <iostream>
#include <windows.h>
#include "ProcessManagement.hpp"
#include "../encryptDecrypt/Cryption.hpp"

ProcessManagement::ProcessManagement() {}

bool ProcessManagement::submitToQueue(std::unique_ptr<Task> task) {
    taskQueue.push(std::move(task));
    return true;
}

void ProcessManagement::executeTasks() {
    while (!taskQueue.empty()) {
        std::unique_ptr<Task> taskToExecute = std::move(taskQueue.front());
        taskQueue.pop();

        std::cout << "Executing task: " << taskToExecute->toString() << std::endl;
        executeCryption(taskToExecute->toString());

        std::string taskStr = taskToExecute->toString();

        // Command to execute
        std::string command = "cryption.exe \"" + taskStr + "\"";

        STARTUPINFOA si{};
        PROCESS_INFORMATION pi{};

        si.cb = sizeof(si);

        // Create child process
        BOOL success = CreateProcessA(
            nullptr,
            command.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &si,
            &pi
        );

        if (success) {
            // std::cout << "Child process created. PID = "
            //         << pi.dwProcessId << std::endl;

            WaitForSingleObject(pi.hProcess, INFINITE);

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else {
            std::cerr << "CreateProcess failed. Error = "
                    << GetLastError() << std::endl;
        }
    }
}