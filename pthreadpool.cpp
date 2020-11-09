
#include"pthreadpool.h"

ThreadPool::ThreadPool(int num):stop(false){
    work_threads.reserve(num);
    for(int i=0;i<num;i++){
        work_threads.emplace_back(ThreadPool::worker, this);
    }
}

ThreadPool::~ThreadPool(){
    stop = 1;
    cv.notify_all();
    for(auto &thread : work_threads){
        thread.join();
    }
}

void *ThreadPool::worker(void *arg){
    ThreadPool *pool = (ThreadPool*)arg;
    pool->run();
    return pool;
}

int ThreadPool::append(Task t){
    queue_mutex.lock();
    task_queue.push(t);
    queue_mutex.unlock();
    cv.notify_one();
    return 1;
}

void ThreadPool::run(){
    while(1){
        unique_lock<mutex> lk(queue_mutex);           //加锁时新建一个独占锁
        while(task_queue.empty()){
            cv.wait(lk);
            if(stop) break;
        }
        if(stop){
            break;
        }
        Task task = task_queue.front();
        task_queue.pop();
        lk.unlock();
        task();
    }
}