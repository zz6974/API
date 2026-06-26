#ifndef CC_PROCESS_H
#define CC_PROCESS_H

#include "CC.h"

#ifdef _WIN32
#include "windows.h"
#elif __linux__

#endif

#define CC_PROCESS_MAX_BUFFER_SIZE 1024

#ifdef _WIN32

#elif __linux__

#endif

namespace CTL{
#ifdef _WIN32

#elif __linux__

#endif
    class Process{
        std::queue<String> ReadBuffer;
        String ExeName;
        String Working;
        bool Flag = false;
        bool IsHide = false;
#ifdef _WIN32
        HANDLE hReadPipe{}, hWritePipe{};
        SECURITY_ATTRIBUTES sa{};
        PROCESS_INFORMATION pi{};
#elif __linux__
        int pipefd[2]{};
        pid_t pid{};
#endif
    public:
        Process() = default;
        Process(const Process& p);
        void Command(const String& CommandExe);
        void SetWorking(const String& WorkPath);
        void SetHide();
        bool Start(bool isolation = false);
        void Stop();
        [[nodiscard]] ByteArray ReadLineBuffer() const;
        [[nodiscard]] bool IsRunning() const;
        static void Execute(const String& Command);
        static void StopProcess(String& Name);
    private:

    public:

    };
}




#endif
