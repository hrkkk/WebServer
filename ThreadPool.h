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
#include <functional>
#include <future>


class ThreadPool {
public:
    ThreadPool(int num);
    ~ThreadPool();

    template<class F, class... Args>
          auto addTask(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

private:
    std::queue<std::function<void()>> m_taskQueue;        // 任务队列
    std::vector<thread> m_workThreads;   // 所有工作线程

    mutex m_mutex;              // 整个线程池的锁
    condition_variable m_condition;      // 条件变量，用于阻塞工作者线程
    bool m_shutdown;                // 是否关闭线程池
};


#endif //WEBSERVER_THREADPOOL_H
