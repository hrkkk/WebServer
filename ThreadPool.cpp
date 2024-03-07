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

