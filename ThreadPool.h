//
// Created by hrkkk on 2024/3/6.
//

#ifndef WEBSERVER_THREADPOOL_H
#define WEBSERVER_THREADPOOL_H

#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <condition_variable>

using namespace std;
using namespace std::literals::chrono_literals;
using callback = void(*)(void*);

class Task {
public:
    callback m_function;
    void* m_arg;
    Task(callback func, void* arg) {
        m_function = func;
        m_arg = arg;
    }
};

class ThreadPool {
public:
    ThreadPool(int min, int max);
    ~ThreadPool();

    void addTask(callback func, void* arg);
    void addTask(Task task);
    int busyNum();
    int aliveNum();
private:
    queue<Task> m_taskQueue;        // 任务队列
    thread m_managerID;             // 管理者线程ID
    vector<thread> m_threadIDs;     // 所有线程ID
    int m_minNum;                   // 最小线程数
    int m_maxNum;                   // 最大线程数
    int m_busyNum;                  // 忙的线程数
    int m_liveNum;                  // 存活的线程数
    int m_exitNum;                  // 要销毁的线程数

    mutex m_mutexPool;              // 整个线程池的锁
    condition_variable m_cond;      // 任务队列是否为空，阻塞工作者线程
    bool m_shutdown;                // 是否销毁线程池
    static void manager(void* arg);     // 管理者线程
    static void worker(void* arg);      // 工作线程
};


#endif //WEBSERVER_THREADPOOL_H
