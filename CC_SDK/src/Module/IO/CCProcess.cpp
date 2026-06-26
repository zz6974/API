#include "IO/CCProcess.h"
#include <CCThread.h>

#include "CCSystem.h"

#ifdef _WIN32
#include "windows.h"
#elif __linux__
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#endif

CTL::Process::Process(const Process &p) {
    ReadBuffer = p.ReadBuffer;
}

void CTL::Process::Command(const CTL::String& CommandExe) {
    ExeName = CommandExe;
}

void CTL::Process::SetWorking(const String &WorkPath) {
    this->Working = WorkPath;
}

void CTL::Process::SetHide() {
    this->IsHide = true;
}

bool CTL::Process::Start(const bool isolation) {
#ifdef _WIN32
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle = TRUE;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        std::cerr << "CreatePipe failed" << std::endl;
        return false;
    }
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    if (IsHide) {
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
    }
    ZeroMemory(&pi, sizeof(pi));
    const auto cmdLine = const_cast<char*>(this->ExeName.c_str());
    const auto Group = isolation ? CREATE_NEW_CONSOLE : 0;
    const char* workingDir = Working.empty() ? nullptr : Working.c_str();
    if (!CreateProcessA(nullptr, cmdLine, nullptr,
        nullptr, TRUE, Group, nullptr,
        workingDir, reinterpret_cast<LPSTARTUPINFOA>(&si), &pi)) {
        std::cerr << "CreateProcess failed" << std::endl;
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return false;
        }
    CloseHandle(hWritePipe);
    Flag = true;
    return true;
#elif __linux__
    if (pipe(pipefd) == -1) {
        std::cerr << "Create pipe failed" << std::endl;
        return false;
    }
    pid = fork();
    if (pid == -1) {
        std::cerr << "Fork failed" << std::endl;
        close(pipefd[0]);
        close(pipefd[1]);
        return false;
    }
    if (pid == 0) {
        if (isolation) {
            if (setsid() == -1) {
                exit(1);
            }
            pid_t pid2 = fork();
            if (pid2 == -1) {
                exit(1);
            }
            if (pid2 != 0) {
                exit(0);
            }
            if (!Working.empty()) {
                chdir(Working.c_str());
            }
            close(pipefd[0]);
            close(pipefd[1]);
            const int fd = open("/dev/null", O_RDWR);
            dup2(fd, STDIN_FILENO);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
            execl("/bin/sh", "sh", "-c", ExeName.c_str(), nullptr);
            exit(1);
        } else {
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
            if (!Working.empty()) {
                chdir(Working.c_str());
            }
            execl("/bin/sh", "sh", "-c", ExeName.c_str(), nullptr);
            exit(1);
        }
    }
    else {
        if (isolation) {
            waitpid(pid, nullptr, 0);
            close(pipefd[0]);
            close(pipefd[1]);
        }
        else {
            close(pipefd[1]);
        }
        Flag = true;
        return true;
    }
#endif
}

void CTL::Process::Stop() {
#ifdef _WIN32
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    Flag = false;
#elif __linux__
    if (Flag) {
        int status;
        waitpid(pid, &status, 0);
        close(pipefd[0]);
        Flag = false;
    }
#endif
}

CTL::ByteArray CTL::Process::ReadLineBuffer() const {
#ifdef _WIN32
    DWORD bytesRead;
    ByteArray ReadBuffer_t;
    char buffer[CC_PROCESS_MAX_BUFFER_SIZE]{};
    if (ReadFile(hReadPipe, buffer, sizeof buffer - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
    }
    ReadBuffer_t.assign(buffer, bytesRead);
    return ReadBuffer_t;
#elif __linux__
    char buffer[CC_PROCESS_MAX_BUFFER_SIZE];
    const ssize_t bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1);
    ByteArray ReadBuffer_t;
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        ReadBuffer_t.assign(buffer, bytesRead);
    }
    return ReadBuffer_t;
#endif
}

bool CTL::Process::IsRunning() const {
#ifdef _WIN32
    if (!Flag) {
        return false;
    }
    DWORD exitCode;
    if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
        return (exitCode == STILL_ACTIVE);
    }
    return false;
#elif __linux__
    if (!Flag) {
        return false;
    }
    return (waitpid(pid, nullptr, WNOHANG) == 0);
#endif
}

void CTL::Process::Execute(const String &Command) {
    Process process;
    process.Command(Command);
    process.Start();
    const auto line = process.ReadLineBuffer();
    if (!line.IsEmpty()) {
        System::Println("{}",line.toString().c_str());
    }
    process.Stop();
}

void CTL::Process::StopProcess(String &Name) {
#ifdef _WIN32
    Name += ".exe";
    const String cmd = String::format("taskkill /im {} /t /f",Name.c_str());
    Execute(cmd);
#elif __linux__
    const String cmd = String::format("killall -9 {}",Name.c_str());
    Execute(cmd);
#endif
}
