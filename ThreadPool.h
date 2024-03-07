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
    std::vector<std::thread> m_workThreads;   // 所有工作线程

    std::mutex m_mutex;              // 整个线程池的锁
    std::condition_variable m_condition;      // 条件变量，用于阻塞工作者线程
    bool m_shutdown;                // 是否关闭线程池
};

template<class F, class... Args>
auto ThreadPool::addTask(F &&f, Args &&...args)
-> std::future<typename std::result_of<F(Args...)>::type>
{
    // 获取添加的任务函数的返回类型
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    // 绑定任务函数的返回值
    std::future<return_type> res = task->get_future();

    {
        std::unique_lock<std::mutex> lock(m_mutex);

        // 如果添加任务时线程池已经关闭，则抛出异常
        if (m_shutdown) {
            throw std::runtime_error("Add task on stopped ThreadPool");
        }

        // 将任务添加到任务队列中
        m_taskQueue.emplace([task]() { (*task)(); });
    }

    // 通知一个空闲中的工作线程
    m_condition.notify_one();
    // 返回任务函数的返回值，调用方可以通过绑定future获取返回值
    return res;
}



#endif //WEBSERVER_THREADPOOL_H
