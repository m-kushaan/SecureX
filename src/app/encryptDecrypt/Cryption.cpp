
#include "Cryption.hpp"
#include "../processes/Task.hpp"
#include "../fileHandling/ReadEnv.cpp"

#include <windows.h>
#include <iostream>

int executeCryption(const std::string& taskData) {
    Task task = Task::fromString(taskData);
    ReadEnv env;
    std::string envKey = env.getenv();
    int key = std::stoi(envKey);
    key = ((key % 256) + 256) % 256;

    if (task.f_stream.is_open()) {
        task.f_stream.close();
    }

    HANDLE fileHandle = CreateFileA(
        task.filePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (fileHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "Unable to open file: "
                  << task.filePath << std::endl;
        return 1;
    }

    LARGE_INTEGER fileSize;

    if (!GetFileSizeEx(fileHandle, &fileSize)) {
        CloseHandle(fileHandle);
        return 1;
    }

    if (fileSize.QuadPart == 0) {
        CloseHandle(fileHandle);
        return 0;
    }

    HANDLE mappingHandle = CreateFileMappingA(
        fileHandle,
        nullptr,
        PAGE_READWRITE,
        0,
        0,
        nullptr
    );

    if (mappingHandle == nullptr) {
        CloseHandle(fileHandle);
        return 1;
    }

    unsigned char* data = static_cast<unsigned char*>(
        MapViewOfFile(
            mappingHandle,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            0
        )
    );

    if (data == nullptr) {
        CloseHandle(mappingHandle);
        CloseHandle(fileHandle);
        return 1;
    }

    size_t size = static_cast<size_t>(fileSize.QuadPart);

    if (task.action == Action::ENCRYPT) {
        for (size_t i = 0; i < size; i++) {
            data[i] = (data[i] + key) % 256;
        }
    } else {
        for (size_t i = 0; i < size; i++) {
            data[i] = (data[i] - key + 256) % 256;
        }
    }

    bool success = FlushViewOfFile(data, size) != 0;

    UnmapViewOfFile(data);
    CloseHandle(mappingHandle);

    if (success) {
        success = FlushFileBuffers(fileHandle) != 0;
    }

    CloseHandle(fileHandle);

    if (!success) {
        std::cerr << "Failed to flush file: "
                  << task.filePath << std::endl;
        return 1;
    }

    return 0;
}
