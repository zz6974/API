#ifndef __CC_ThreadPool_H__
#define __CC_ThreadPool_H__
#pragma once

#include <CCThread.h>
#include "CC.h"
#include "CCAutoLock.h"



/**
 * 命名空间CTL，包含线程池的实现
 */
namespace CTL {
    /**
     * ThreadPool类，用于管理和调度线程
     */
    class ThreadPool {
    private:
        size_t m_corePoolSize = {}; // 核心线程池大小
        size_t m_maximumPoolSize = {}; // 最大线程池大小
        int m_keepAliveTime = {}; // 线程存活时间
        int MAX_QUEUE_SIZE = 4096;
        bool m_isStop = {}; // 使用 atomic 确保线程安全，表示线程池是否停止
        int m_C_threadPool = {}; // 使用 atomic 确保线程安全，表示当前线程池中的核心线程数
        int T_PoolSize = {}; // 使用 atomic 确保线程安全，表示当前线程池中的任务数
        int E_PoolSize = {}; // 使用 atomic 确保线程安全，表示当前额外线程数
        int T_CorePoolSize = {}; // 使用 atomic 确保线程安全，表示当前核心任务数
        std::mutex m_mutex = {}; // 互斥锁，用于保护共享资源
        std::condition_variable m_condition = {}; // 条件变量，用于线程间通信
        std::queue<Function<void()>> m_taskQueue = {}; // 任务队列，存储待执行的任务
        std::vector<std::thread*> m_thread = {}; // 使用标准库的 std::thread，存储线程池中的线程
    public:
        ThreadPool(); // 默认构造函数
        ~ThreadPool(); // 析构函数
        /**
         * 构造函数，初始化线程池
         * @param corePoolSize 核心线程池大小
         * @param maximumPoolSize 最大线程池大小，默认为1024
         * @param keepAliveTime 线程存活时间，默认为1000毫秒
         */
        explicit ThreadPool(const size_t corePoolSize, size_t maximumPoolSize = 1024, int keepAliveTime = 1000)
           : m_corePoolSize(corePoolSize), m_maximumPoolSize(maximumPoolSize), m_keepAliveTime(keepAliveTime),
             m_isStop(false), m_C_threadPool(0), T_PoolSize(0), E_PoolSize(0), T_CorePoolSize(0) {
            InitStart(corePoolSize, maximumPoolSize, keepAliveTime);
        }
        /**
         * 初始化线程池
         * @param corePoolSize 核心线程池大小
         * @param maximumPoolSize 最大线程池大小
         * @param keepAliveTime 线程存活时间
         */
        void InitStart(size_t corePoolSize, size_t maximumPoolSize, int keepAliveTime);
        /**
         * 停止线程池
         */
        void Stop();
        /**
         * 添加任务到线程池
         * @param func 任务函数
         * @param args 函数参数
         */
        template<typename Fun,typename ...Args>
        void AddTask(Fun&& func, Args&&... args);
        /**
         * 获取当前活动线程数
         * @return 当前活动线程数
         */
        int GetActiveThread() const;
        /**
         * 获取最大线程池大小
         * @return 最大线程池大小
         */
        int GetMaxPoolSize() const;
        /**
         * 获取核心线程池大小
         * @return 核心线程池大小
         */
        int GetCorePoolSize() const;
        /**
         * 获取线程存活时间
         * @return 线程存活时间
         */
        int GetTimeout() const;
        /**
         * 获取当前活动的额外线程数
         * @return 当前活动的额外线程数
         */
        int GetActiveExtraThread() const;

    private:
        /**
         * 工作线程函数
         * @param index 线程索引
         */
        void Worker(int index);
        /**
         * 打印错误信息
         * @param str 错误信息
         */
        static void CC_PRINT_ERROR(const CTL::String& str);
        /**
         * 创建额外线程并执行任务
         * @param Task 任务函数
         */
        void ExtraThread(Function<void()> Task);
    };


    /**
     * 添加任务到线程池
     * @param func 任务函数
     * @param args 函数参数
     */
    template <typename Fun, typename ... Args>
    void ThreadPool::AddTask(Fun&& func, Args&&... args) {
        if (m_isStop) {
            return;
        }
        Function<void()> task = std::bind(std::forward<Fun>(func), std::forward<Args>(args)...);
        if (!task) {
            return;
        }
        CCUniqueLock lock(m_mutex);
        if (task) {
            // 更合理的任务分配策略
            if (T_CorePoolSize < m_corePoolSize) {
                m_taskQueue.push(task);
                m_condition.notify_one();
            }
            else if (E_PoolSize < (m_maximumPoolSize - m_corePoolSize)) {
                ExtraThread(task);
                CC_PRINT_ERROR("ExtraThread");
            }
            else if (m_taskQueue.size() < MAX_QUEUE_SIZE) { // 添加队列大小限制
                m_taskQueue.push(task);
                m_condition.notify_one();
            }
            else {
                // 队列满时的处理策略：拒绝、等待或扩容
                CC_PRINT_ERROR("Task queue is full, task rejected");
            }
        }
    }
}

#endif
