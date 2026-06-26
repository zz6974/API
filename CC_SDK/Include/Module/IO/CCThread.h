#ifndef CC_Thread_
#define CC_Thread_

#include "CC.h"
#include "CCSystem.h"
#include "TL/Map.h"
#include "TL/Queue.h"

// 禁用Visual Studio的警告信息
#pragma warning(disable : 4996)

// 定义引用封装宏
#define C_ARG(A) std::ref(A)
// 定义线程相关的类型别名
#define CC_Thread void

namespace CTL {
    class IntSleep {
    public:
        IntSleep() : interrupt_flag(false) {}
        void SleepHour(const std::chrono::hours duration) {
            interrupt_flag = false;
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait_for(lock, duration, [this] { return interrupt_flag; });
        }
        void SleepMinutes(const std::chrono::minutes duration) {
            interrupt_flag = false;
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait_for(lock, duration, [this] { return interrupt_flag; });
        }
        void SleepSecond(const std::chrono::seconds duration) {
            interrupt_flag = false;
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait_for(lock, duration, [this] { return interrupt_flag; });
        }
        void SleepMillisecond(const std::chrono::milliseconds duration) {
            interrupt_flag = false;
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait_for(lock, duration, [this] { return interrupt_flag; });
        }
        void SleepMicrosecond(const std::chrono::microseconds duration) {
            interrupt_flag = false;
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait_for(lock, duration, [this] { return interrupt_flag; });
        }
        void interrupt() {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                interrupt_flag = true;
            }
            condition_.notify_all();
        }
    private:
        std::condition_variable condition_;
        std::mutex mutex_;
        bool interrupt_flag;
    };
    class Thread_Integration {
    public:
        virtual ~Thread_Integration() = default;
        void Start();
        void Stop();
        void Wait();
        [[nodiscard]] bool Sign() const;
        virtual void Run() = 0;
    private:
        void running();
        bool Flag = false, Flag_m = false;
        std::thread Thread_m;
    };
    /**
     * @class Thread
     * @brief 线程管理类，用于创建和控制线程
     */
    class Thread{
    public:
        Thread();
        Thread(Thread &&other) noexcept;
        /**
         * @brief 构造函数，用于创建线程
         *
         * @tparam Func 要在线程中执行的函数类型
         * @tparam Args 函数的参数类型
         * @param fun 要在线程中执行的函数
         * @param args 函数的参数
         */
        template <typename Func, typename... Args>
        explicit Thread(Func&& fun, Args&&... args);
        ~Thread();
        /**
         * @brief 设置线程要执行的任务
         *
         * @tparam Func 要在线程中执行的函数类型
         * @tparam Args 函数的参数类型
         * @param fun 要在线程中执行的函数
         * @param args 函数的参数
         * @return true 如果设置成功
         * @return false 如果线程已经开始执行
         */
        template <typename Func, typename... Args>
        bool SetThread(Func&& fun, Args&&... args);
        /**
         * @brief 启动线程并执行任务
         *
         * @tparam Func 要在线程中执行的函数类型
         * @tparam Args 函数的参数类型
         * @param fun 要在线程中执行的函数
         * @param args 函数的参数
         * @return true 如果线程成功启动
         * @return false 如果线程已经开始执行
         */
        template <typename Func, typename... Args>
        static void addTask(Func &&fun, Args &&...args);
        /**
         * @brief 启动线程并执行任务，任务完成后线程
         */
        void Start(){
            if (!Flag) {
                Flag = true;
                Thread_m = std::make_unique<std::thread>(&Thread::run,this);
            }
        }
        /**
         * @brief 启动线程并执行任务，任务完成后线程自动分离
        */
        void StartDetach() {
            if (!Flag) {
                Flag = true;
                Thread_m = std::make_unique<std::thread>(&Thread::run,this);
                Thread_m->detach();
            }
        }
        /**
         * @brief 启动线程并执行任务，等待任务完成
         */
        void RunWait() {
            if (!Flag) {
                Flag = true;
                Thread_m = std::make_unique<std::thread>(&Thread::run,this);
                Thread_m->join();
            }
        }
        /**
         * @brief 停止线程，并等待任务完成
         */
        void Stop();
        /**
         * @brief 检查线程是否正在执行任务
         * @return true 如果线程正在执行任务
         * @return false 如果线程没有执行任务
         */
        [[nodiscard]] bool Sign() const;
        /**
         * @brief 线程休眠
         * @param us 休眠时间，单位为微秒
         * @return 实际休眠时间，单位为微秒
         */
        static long Sleep(unsigned int us);
        /**
         * @brief 线程休眠
         * @param MS 休眠时间，单位为毫秒
         * @return 实际休眠时间，单位为毫秒
        */
        static long SleepMS(unsigned int MS);
        /**
         * @brief 获取线程对象的引用
         * @return std::thread& 线程对象的引用
         */
        [[nodiscard]] std::thread* GetThread() const;
        /**
         * @brief 获取线程的标识符
         * @return std::thread::id 线程的标识符
         */
        [[nodiscard]] std::thread::id GetThreadId() const;
        /**
         * @brief 设置线程是否安全退出标志
         * @param Flag 安全退出标志
         */
        static void SetSafeExit(bool Flag);
        /**
         * @brief 获取硬件信息
         * @return int 硬件信息
         */
        static int getHardWare();
        /**
         * @brief 线程休眠
         */
        static void YieLd();
        /**
         * @brief 获取当前时间戳
         * @return size_t 当前时间戳
         */
        static size_t getNowMS();
        /**
         * @brief 设置线程信息
         * @param info 线程信息
         */
        void setInfo(const std::string& info);
    protected:
        static std::mutex thread_mutex_t;
        static Queue<std::function<void()>> CC_thread_Queue_t;
        static std::unordered_map<uint64_t,Thread> CC_thread_Map_t;
        static bool SafeExit;
        static int AssignID();
        int threadID = -1;
        std::unique_ptr<std::thread> Thread_m = nullptr; // 线程对象
        std::string Info;
        bool Flag = false; // 线程状态标志
    private:
        void run();
    };
}

/**
 * @brief 构造函数模板，用于创建线程并执行给定的任务
 *
 * @tparam Func 要在线程中执行的函数类型
 * @tparam Args 函数的参数类型
 * @param fun 要在线程中执行的函数
 * @param args 函数的参数
 */
template<typename Func, typename ... Args>
CTL::Thread::Thread(Func &&fun, Args &&...args) {
    this->threadID = AssignID();
    SetThread(fun, args...);
}

/**
 * @brief 设置线程要执行的任务
 *
 * @tparam Func 要在线程中执行的函数类型
 * @tparam Args 函数的参数类型
 * @param fun 要在线程中执行的函数
 * @param args 函数的参数
 * @return true 如果设置成功
 * @return false 如果线程已经开始执行
 */
template<typename Func, typename ... Args>
bool CTL::Thread::SetThread(Func &&fun, Args &&...args) {
    if (Thread_m && Thread_m->joinable()) {
        return false;
    }
    const auto task = std::bind(std::forward<Func>(fun), std::forward<Args>(args)...);
    CC_thread_Queue_t.Add(task);
    return true;
}

/**
 * @brief 启动线程并执行任务
 *
 * @tparam Func 要在线程中执行的函数类型
 * @tparam Args 函数的参数类型
 * @param fun 要在线程中执行的函数
 * @param args 函数的参数
 */
template<typename Func, typename ... Args>
void CTL::Thread::addTask(Func &&fun, Args &&...args) {
    const std::function<void()> func = std::bind(std::forward<Func>(fun), std::forward<Args>(args)...);
    const auto thread_t = new CTL::Thread;
    thread_t->SetThread([](CTL::Thread* thread_t, const std::function<void()>& func_t) {
        try {
            if (func_t) {
                func_t();
            }
        }
        catch (const std::exception& e) {
            CTL::System::Println("CTL::Thread::addTask Error: {}", e.what());
        }
        catch (...) {
            CTL::System::Println("CTL::Thread::addTask Unknown Error");
        }
        delete thread_t;
    }, thread_t, std::move(func));
    thread_t->StartDetach();
}


#endif
