# SecureX — Parallel File Encryption and Decryption

## Overview

**SecureX** is a C++ application for file encryption and decryption using **process-based parallelism**.

The application recursively scans a given directory, creates a task for each file, and uses a task queue to manage the encryption/decryption operations. On Windows, separate processes are created using the Windows Process API.

## Features

* File encryption and decryption
* Recursive directory traversal
* Task queue-based processing
* Process-based parallelism
* Windows child process creation using `CreateProcessA()`
* Process synchronization using `WaitForSingleObject()`

## Project Structure

```text
SecureX/
│
├── src/
│   └── app/
│       ├── encryptDecrypt/
│       ├── fileHandling/
│       └── processes/
│           ├── ProcessManagement.cpp
│           ├── ProcessManagement.hpp
│           ├── Task.cpp
│           └── Task.hpp
│
├── test/
├── .env
├── .gitignore
├── Makefile
├── README.md
└── main.cpp
```

## How It Works

```text
Directory
    ↓
Recursive File Scan
    ↓
Create Tasks
    ↓
Task Queue
    ↓
ProcessManagement
    ↓
CreateProcessA()
    ↓
cryption.exe
    ↓
Encrypt / Decrypt
```

For every regular file in the selected directory, a `Task` is created and added to the queue. `ProcessManagement` then executes the tasks using a separate `cryption.exe` process.

## Windows Process Management

The Windows implementation uses:

```cpp
CreateProcessA()
WaitForSingleObject()
CloseHandle()
```

`CreateProcessA()` creates the child process running `cryption.exe`, while `WaitForSingleObject()` allows the parent process to wait for its completion.

## Build & Run

### Build

Make sure `g++` and `make` are installed.

```bash
make
```

This builds:

```text
encrypt_decrypt.exe
cryption.exe
```

### Run

```bash
encrypt_decrypt.exe
```

Enter the directory path and operation:

```text
Enter the directory path: test
Enter the action (encrypt/decrypt): encrypt
```

Use `decrypt` to perform decryption.

## Technologies

* C++17
* C++ Filesystem
* Windows API
* Makefile
* Process-based parallelism
