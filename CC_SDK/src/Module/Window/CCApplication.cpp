#include "CCApplication.h"
#include "CCFileOutStream.h"
#include "CCFile.h"

#ifdef _WIN32
#include "windows.h"
#elif __linux__
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <net/if.h>
#endif


#ifdef _WIN32
bool HasChineseCharacters(const std::string& path) {
    // 将多字节字符串转换为宽字符字符串
    int len = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, NULL, 0);
    if (len <= 0) return false;

    wchar_t* wpath = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wpath, len);

    bool hasChinese = false;
    for (int i = 0; wpath[i] != L'\0'; ++i) {
        // 判断是否为中文字符（Unicode范围）
        if (wpath[i] >= 0x4E00 && wpath[i] <= 0x9FFF) {
            hasChinese = true;
            break;
        }
    }

    delete[] wpath;
    return hasChinese;
}
#endif

int CTL::Application::DaemonTime = 1;
bool CTL::Application::DaemonFlag = false;
CTL::Process CTL::Application::process;
CCQueue<CTL::Function<void()>> CTL::Application::M_SignalQueues;
CTL::Function<void()> CTL::Application::func_Close;
CCMutex CTL::Application::M_Mutex;
bool CTL::Application::SingleInstance_t = false;

#ifdef _WIN32
HANDLE CTL::Application::mutex_handle = { nullptr };
CTL::String CTL::Application::mutex_name = "Global\\" + CTL::System::GetAppName();
#else
int CTL::Application::lock_fd = -1;
const CTL::String CTL::Application::lock_file_path = "/tmp/" + CTL::System::GetAppName() + ".lock";
#endif


int CTL::Application::Running(const Application* app) {
    while (CC::IsRun()){
        Thread::SleepMS(16);
        {
            CCUniqueLock locks(M_Mutex);
            try {
                if (M_SignalQueues.empty()) {
                    continue;
                }
                auto signal = M_SignalQueues.front();
                M_SignalQueues.pop();
                signal();
            }
            catch (CCException& e) {
                CC::Println("Application::Running -> " + CTL::String(e.what()),RED);
            }
        }
    }
    if (func_Close){
        func_Close();
    }
    if(SingleInstance_t){
        ReleaseLock();
    }
    return 0;
}

CTL::Application::Application(const int argc, char **argv) {
    this->Parameter.argc = argc;
    for (int i = 0; i < argc; i++){
        this->Parameter.argv.push_back(argv[i]);
    }
#ifdef _WIN32

#elif __linux__

#endif
}

CTL::Application::~Application() {
    CC::Exit();
}

void CTL::Application::SetApp(void *app) {
    App = app;
}

void *CTL::Application::GetApp() {
    return App;
}

CTL::AppParameter_t CTL::Application::GetParameter(){
    return this->Parameter;
}

void CTL::Application::Signal(Function<void()> &&func) {
    CCUniqueLock lock(M_Mutex);
    M_SignalQueues.push(std::move(func));
}

void CTL::Application::SetCloseFun(Function<void()>&& func){
    func_Close = func;
}

bool CTL::Application::IsSingleInstance() {
    SingleInstance_t = true;
#ifdef _WIN32
    mutex_handle = CreateMutex(NULL, TRUE, mutex_name.to_CCWString().c_str());
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (mutex_handle) {
            CloseHandle(mutex_handle);
        }
        return false;
    }
    return true;
#else
    lock_fd = open(lock_file_path.c_str(), O_CREAT | O_RDWR, 0666);
    if (lock_fd < 0) {
        return false;
    }
    if (flock(lock_fd, LOCK_EX | LOCK_NB) != 0) {
        close(lock_fd);
        return false;
    }
    char pid_str[16];
    sprintf(pid_str, "%d\n", getpid());
    write(lock_fd, pid_str, strlen(pid_str));

    return true;
#endif
}

void CTL::Application::ReleaseLock() {
#ifdef _WIN32
    if (mutex_handle) {
        ReleaseMutex(mutex_handle);
        CloseHandle(mutex_handle);
    }
#else
    if (lock_fd >= 0) {
        flock(lock_fd, LOCK_UN);
        close(lock_fd);
        unlink(lock_file_path.c_str());  // 删除锁文件
    }
#endif
}

void CTL::Application::StartDaemon(int argc, char **argv) {
#ifdef _WIN32
    const auto AppDir = System::ApplicationDirPath();
    if (HasChineseCharacters(AppDir)) {
        System::Println("Daemon process cannot run the Chinese path program Path: {}",AppDir.c_str());
        return;
    }
    DaemonFlag = true;
    if (argc >= 2) {
        const String str = argv[1];
        if (str == "Daemon_Start") {
            return;
        }
    }
    const auto Name = "Daemon_" + System::GetAppName();
    const auto AppName = System::GetAppName();
    const auto AppNamePath = AppDir + "/" + AppName;
    const auto Pos = AppName.find_first_of(".");
    const String Pro = AppName.substr(0,Pos);
    String AppNamePathBat = AppDir + "/" + Pro + ".bat";
    String str = CCFile::NormalizePath("./") + String::format("/{}_Stop.flag",Pro.c_str());
    str = str.Encoder("GBK");
    const CCFile file(str);
    if (file.isExists()) {
        bool F = file.Delete();
    }
    String Bat;
    Bat += "@echo off\n";
    Bat += String::format("set _interval={}\n",DaemonTime);
    Bat += String::format("set _processName={}\n",Pro.c_str());
    Bat += ":loop\n";
    Bat += String::format("tasklist | find \"{}\" > nul\n",AppName.c_str());
    Bat += "if %errorlevel% neq 0 (\n";
    Bat += "    echo Start %_processName%-%time%\n";
    Bat += String::format("    start \"\" \"{}\" Daemon_Start\n",AppNamePath.c_str());
    Bat += ")\n";
    Bat += "timeout /t %_interval% /nobreak > nul\n";
    Bat += String::format("if exist \"{}_Stop.flag\" (\n",Pro.c_str());
    Bat += String::format("    del \"{}_Stop.flag\"\n",Pro.c_str());
    Bat += String::format("    del \"{}_Start.flag\"\n",Pro.c_str());
    Bat += "    exit\n";
    Bat += ")\n";
    Bat += "goto loop\n";
    Bat += String::format("    del \"{}_Stop.flag\" 2>nul\n", Pro.c_str());
    Bat += String::format("    del \"{}_Start.flag\" 2>nul\n", Pro.c_str());
    process.Command(AppNamePathBat);
    AppNamePathBat = AppNamePathBat.Encoder("GBK");
    FileOutStream Out(AppNamePathBat);
    Out << Bat;
    Out.close();
    process.SetHide();
    DaemonFlag = process.Start(true);
#elif __OHOS__
#elif __linux__
    DaemonFlag = true;
    if (argc >= 2) {
        const String str = argv[1];
        if (str == "Daemon_Start") {
            // System::Println("守护进程启动");
            return;
        }
    }
    const auto AppName = System::GetAppName();
    const auto AppDir = System::ApplicationDirPath();
    const auto AppNamePathSh = CCFile::NormalizePath(AppDir + "/" + AppName);
    const auto AppNamePathBat = AppNamePathSh + "_Daemon.sh";
    String Sh;
    Sh += "#!/bin/sh\n";
    Sh += "while true\n";
    Sh += "do\n";
    Sh += String::format("if [ ! -f {}.pid ] || ! kill -0 $(cat {}.pid) 2>/dev/null; then\n", AppName.c_str(), AppName.c_str());
    Sh += String::format("    {} Daemon_Start &\n", AppNamePathSh.c_str());
    Sh += String::format("    echo $! > {}.pid\n", AppName.c_str());
    Sh += String::format("    echo \"Start {}\"\n", AppNamePathSh.c_str());
    Sh += "    sleep 3\n";
    Sh += "fi\n";
    Sh += String::format("sleep {}\n", DaemonTime);
    Sh += "done\n";
    FileOutStream Out(AppNamePathBat);
    Out << Sh;
    Out.close();
    SetFileReadPermission(AppNamePathBat);
    process.Command(AppNamePathBat);
    DaemonFlag = process.Start(true);
#endif
}

void CTL::Application::StopDaemon() {
#ifdef _WIN32
    if (!IsDaemon()) {
        return;
    }
    const auto AppName = System::GetAppName();
    const auto Pos = AppName.find_first_of(".");
    const String Pro = AppName.substr(0,Pos);
    String str = CCFile::NormalizePath("./") + String::format("/{}_Stop.flag",Pro.c_str());
    str = str.Encoder("GBK");
    const bool F = CCFile::Create(str);
    if (F) {
        const CCFile file(str);
        int Time = 0;
        while (true) {
            if (!file.isExists() || Time >= DaemonTime * 2) {
                break;
            }
            Time++;
            Thread::SleepMS(1000);
        }
    }
#elif __OHOS__
    #elif __linux__
    const auto AppName = System::GetAppName();
    const auto AppNamePathBat = AppName + "_Daemon.sh";
    const auto cmd = String::format("killall -9 {}",AppNamePathBat.c_str());
    System::Execute(cmd);
#endif
}

bool CTL::Application::IsDaemon() {
    return DaemonFlag;
}

#ifdef __linux__
bool CTL::Application::SetFileReadPermission(const std::string &filePath) {
    // 获取文件的当前权限
    struct stat fileStat{};
    if (stat(filePath.c_str(), &fileStat) != 0) {
        std::cerr << "Failed to get file status: " << filePath << std::endl;
        return false;
    }

    // 更改文件权限，添加读和执行权限
    mode_t newPermissions = fileStat.st_mode | S_IRUSR | S_IXUSR;
    if (chmod(filePath.c_str(), newPermissions) != 0) {
        std::cerr << "Failed to set read and execute permission for file: " << filePath << std::endl;
        return false;
    }

    return true;
}
#endif

