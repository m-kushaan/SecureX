# SecureX

SecureX is a C++ application for **file encryption and decryption**, developed to demonstrate sequential processing, multiprocessing, and multithreading approaches.

## Features

* File encryption and decryption
* Recursive directory processing
* Task-based execution using a queue
* Sequential processing
* Multiprocessing using Windows child processes
* Multithreading using `std::thread`
* Windows-compatible implementation

## Project Structure

```text
SecureX/
├── main.cpp
├── src/
│   └── app/
│       ├── processes/
│       │   ├── ProcessManagement.hpp
│       │   ├── ProcessManagement.cpp
│       │   └── Task.hpp
│       │
│       └── encryptDecrypt/
│           └── Cryption.hpp
│
├── test/
│   ├── test1.txt
│   └── test2.txt
│
├── Makefile
├── README.md
└── .gitignore
```

## Processing Implementations

The project was developed in three stages, represented by separate commits.

### 1. `Initial Commit` — Sequential Processing

* Files are converted into tasks.
* Tasks are stored in a queue.
* Tasks are processed one at a time.
* No parallel execution is used.

```text
Task 1 → Task 2 → Task 3 → Task 4
```

### 2. `Implement Windows multiprocessing with shared memory` — Multiprocessing

* The parent process creates child processes using Windows `CreateProcess()`.
* Tasks are stored in shared memory.
* Each child process retrieves and processes a task independently.
* The parent waits for all child processes to finish.

```text
                 Parent Process
                /      |      \
           Child 1  Child 2  Child 3
           Task 1   Task 2   Task 3
```

### 3. `Implement multithreading for parallel encryption` — Multithreading

* Uses C++ `std::thread`.
* Multiple worker threads access the shared task queue.
* A mutex protects queue operations.
* Encryption/decryption tasks execute concurrently within the same process.

```text
                  SecureX
                     |
            ProcessManagement
             /       |       \
        Thread 1  Thread 2  Thread 3
         Task 1    Task 2    Task 3
```

## How It Works

1. The user provides the directory path.
2. The user selects `encrypt` or `decrypt`.
3. SecureX recursively scans the directory.
4. Each file is converted into a `Task`.
5. Tasks are added to the task queue.
6. `ProcessManagement` executes the tasks using the selected processing model.
7. Encryption/decryption is performed on each file.

## How to Run

### Clone the Repository

```bash
git clone <your-repository-url>
cd SecureX
```

### Build

```bash
make
```

### Run

```bash
./SecureX
```

Enter the directory and action:

```text
Enter the directory path: test
Enter the action (encrypt/decrypt): encrypt
```

Use `decrypt` to decrypt the files.

## Requirements

* C++17
* Windows
* MinGW / g++
* GNU Make

## Parallel Processing Comparison

| Implementation  | Execution                | Synchronization                 |
| --------------- | ------------------------ | ------------------------------- |
| Sequential      | One task at a time       | Not required                    |
| Multiprocessing | Multiple child processes | Shared memory + synchronization |
| Multithreading  | Multiple threads         | Mutex-protected task queue      |
