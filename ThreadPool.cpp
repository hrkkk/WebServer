//
// Created by hrkkk on 2024/3/6.
//

#include "ThreadPool.h"

ThreadPool::ThreadPool(int num) : m_shutdown(false)
{
    for (size_t i = 0; i < num; ++i) {
        // emplace_back会创建thread对象，相当于创建线程，传递工作函数
        m_workThreads.emplace_back([this] {
            while (1) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    // 任务队列为空时（且线程池未关闭），当前工作线程阻塞，等待唤醒
                    m_condition.wait(lock, [this] {
                        return m_shutdown || !m_taskQueue.empty();
                    });
                    // 如果线程池关闭且任务队列为空，销毁当前线程
                    if (m_shutdown && m_taskQueue.empty()) {
                        return;
                    }

                    // 从任务队列中取出第一个任务
                    task = std::move(m_taskQueue.front());
                    m_taskQueue.pop();
                }

                // 执行任务
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_shutdown = true;
    }
    // 通知所有空闲线程自行销毁
    m_condition.notify_all();
    // 等待所有线程中的任务都执行完成后线程池才能真正的关闭
    for (std::thread& worker : m_workThreads) {
        worker.join();
    }
}

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

