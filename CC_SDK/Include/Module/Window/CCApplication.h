#ifndef CC_APPLICATION_H
#define CC_APPLICATION_H

#include "CCThread.h"
#include "CCSystem.h"
#include "CCProcess.h"

namespace CTL {
    struct AppParameter_t{
        int argc = 0;
        CCVector<String> argv;
    };
    class Application{
    public:
        Application(int argc, char** argv);
        ~Application();
        static int Running(const Application* app = nullptr);
        //--------------------------------------------------------------------------------------------------------------
        void SetApp(void* app);
        void* GetApp();
        //--------------------------------------------------------------------------------------------------------------
        AppParameter_t GetParameter();
        //--------------------------------------------------------------------------------------------------------------
        static void Signal(Function<void()>&& func);
        static void SetCloseFun(Function<void()>&& func);
        static bool IsSingleInstance();
        static void ReleaseLock();
        static void StartDaemon(int argc, char **argv);
        static void StopDaemon();
        static bool IsDaemon();
        //--------------------------------------------------------------------------------------------------------------
    private:
        AppParameter_t Parameter{};
        void* App = nullptr;
        static int DaemonTime; // Seconds
        static bool DaemonFlag;
        static Process process;
        static CCQueue<Function<void()>> M_SignalQueues;
        static Function<void()> func_Close;
        static CCMutex M_Mutex;
        static bool SingleInstance_t;
#ifdef _WIN32
        static HANDLE mutex_handle;
        static String mutex_name;
#else
        static int lock_fd;
        static const String lock_file_path;
        static bool SetFileReadPermission(const std::string &filePath);
#endif
    };
}

#endif
