//
// Created by hrkkk on 2024/3/6.
//

#include "ThreadPool.h"
#include <cstdlib>
#include <iostream>
#include <cstring>

using namespace std;
const int NUMBER = 2;

ThreadPool::ThreadPool(int min, int max)
{
    m_minNum = min;
    m_maxNum = max;
    m_busyNum = 0;
    m_liveNum = min;
    m_exitNum = 0;
    m_shutdown = false;
    // 创建管理者线程
    m_managerID = thread(manager, this);
    // 创建工作线程
    m_workThreads.resize(max);
    for (int i = 0; i < min; ++i) {
        m_workThreads[i] = thread(worker, this);
    }
}

ThreadPool::~ThreadPool()
{
    m_shutdown = true;
    // 阻塞回收管理者线程
    if (m_managerID.joinable()) {
        m_managerID.join();
    }
    // 唤醒阻塞的消费者线程
    m_condition.notify_all();
    for (int i = 0; i < m_maxNum; ++i) {
        if (m_workThreads[i].joinable()) {
            m_workThreads[i].join();
        }
    }
}

void ThreadPool::addTask(Task task)
{
    unique_lock<mutex> lk(m_mutexPool);
    if (m_shutdown) {
        return;
    }
    // 添加任务
    m_taskQueue.push(task);
    m_condition.notify_all();
}

void ThreadPool::addTask(callback func, void *arg)
{
    addTask(Task(func, arg));
}

int ThreadPool::busyNum()
{
    m_mutexPool.lock();
    int busy = m_busyNum;
    m_mutexPool.unlock();
    return busy;
}

int ThreadPool::aliveNum()
{
    m_mutexPool.lock();
    int alive = m_liveNum;
    m_mutexPool.unlock();
    return alive;
}

void ThreadPool::worker(void *arg)
{
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    // 工作者线程需要不停的获取线程池任务队列，所以使用while
    while (true) {
        // 每一个线程都需要对线程池进行任务队列操作，因此线程池是共享资源，需要加锁
        unique_lock<mutex> lk(pool->m_mutexPool);
        // 当前任务队列是否为空
        while (pool->m_taskQueue.empty() && !pool->m_shutdown) {
            // 如果任务队列为空，并且线程池没有被关闭，则阻塞当前工作线程，等待其他线程通知或有任务加入
            pool->m_condition.wait(lk);

            // 判断是否要销毁线程，管理者让该工作者线程自杀
            if (pool->m_exitNum > 0) {
                pool->m_exitNum--;
                if (pool->m_liveNum > pool->m_minNum) {
                    pool->m_liveNum--;
                    cout << "Thread: " << this_thread::get_id() << " exit..." << endl;
                    // 当前线程拥有互斥锁，需要解锁，否则会死锁
                    lk.unlock();
                    return;
                }
            }
        }
        // 判断线程池是否关闭了
        if (pool->m_shutdown) {
            cout << "Thread: " << this_thread::get_id() << " exit..." << endl;
            return;
        }

        // 以下是工作流程：
        // 从任务队列中取出一个任务
        Task task = pool->m_taskQueue.front();
        pool->m_taskQueue.pop();
        pool->m_busyNum++;
        // 当访问完线程池队列时，线程池解锁
        lk.unlock();

        // 取出Task任务后，就可以在当前线程中执行该任务了
        cout << "Thread: " << this_thread::get_id() << " start work..." << endl;
        task.m_function(task.m_arg);
        free(task.m_arg);
        task.m_arg = nullptr;

        // 任务执行完毕，忙线程解锁，线程回到空闲状态
        cout << "Thread: " << this_thread::get_id() << " end work..." << endl;
        lk.lock();
        pool->m_busyNum--;
        lk.unlock();
    }
}

void ThreadPool::manager(void *arg)
{
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    // 管理者线程需要不停的监视线程池队列和工作线程
    while (!pool->m_shutdown) {
        // 每隔3秒检测一次
        this_thread::sleep_for(chrono::seconds(3));

        // 取出线程池中任务的数量和当前线程的数量，别的线程有可能在写数据，所以需要加锁
        // 目的是添加或销毁线程
        unique_lock<mutex> lk(pool->m_mutexPool);
        int queueSize = pool->m_taskQueue.size();
        int liveNum = pool->m_liveNum;
        int busyNum = pool->m_busyNum;
        lk.unlock();

        // 添加线程 —— 代表当前工作线程不够，且未达到最大线程数
        // 任务的个数 > 存活的线程个数 && 存活的线程数 < 最大线程数
        if (queueSize > liveNum && liveNum < pool->m_maxNum) {
            // 因为在for循环中操作了线程池变量，所以需要加锁
            lk.lock();
            // 用于计数，添加的线程个数
            int count = 0;
            // 添加线程
            for (int i = 0; i < pool->m_maxNum && count < NUMBER && pool->m_liveNum < pool->m_maxNum; ++i) {
                // 判断当前线程ID，用来存储创建的线程ID
                if (pool->m_workThreads[i].get_id() == thread::id()) {
                    cout << "Create a new thread..." << endl;
                    pool->m_workThreads[i] = thread(worker, pool);
                    // 线程创建完毕
                    count++;
                    pool->m_liveNum++;
                }
            }
            lk.unlock();
        }

        // 销毁线程 —— 当前存活的线程太多了，工作的线程太少了
        // 忙的线程*2 < 存活的线程数 && 存活的线程数 > 最小的线程数
        if (busyNum * 2 < liveNum && liveNum > pool->m_minNum) {
            // 访问了线程池，需要加锁
            lk.lock();
            // 一次性销毁两个
            pool->m_exitNum = NUMBER;
            lk.unlock();
            // 让工作线程自杀，无法做到直接杀死空闲线程，只能通知空闲线程让它自杀
            for (int i = 0; i < NUMBER; ++i) {
                pool->m_condition.notify_all();
            }
        }
    }
}