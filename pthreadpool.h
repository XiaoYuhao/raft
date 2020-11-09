#ifndef _PTHREADPOOL_H_
#define _PTHREADPOOL_H_

#include<vector>
#include<queue>
#include<thread>
#include<iostream>
#include<stdexcept>
#include<condition_variable>
#include<memory>
#include<functional>

using std::vector;
using std::queue;
using std::mutex;
using std::thread;
using std::condition_variable;
using std::function;
using std::unique_lock;

typedef function<void(void)> Task;

class ThreadPool{
    vector<thread> work_threads;
    queue<Task> task_queue;
    bool stop;
    mutex queue_mutex;
    condition_variable cv;

public:
    ThreadPool(int num = 20);
    ~ThreadPool();
    static void *worker(void *arg);
    void run();
    int append(Task t);
};

#endif