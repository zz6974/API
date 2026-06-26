#include "CCSystem.h"
#include "CCString.h"
#include "CCFile.h"
#include <malloc.h>

#ifdef _WIN32
#include "Windows.h"
#include <shobjidl.h>
#include <propvarutil.h>
#include <shlguid.h>
#include <pdh.h>
#include <propkey.h>
#include <iphlpapi.h>
#include <VersionHelpers.h>
#elif __linux__
#include <linux/limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <fstream>
#include <sstream>
#include <linux/if_arp.h>
#if _SYS_STATVFS_H
#else
#include <sys/statvfs.h>
#endif
#ifdef __OHOS__
#include "hidebug/hidebug.h"
#include <sys/statvfs.h>
#else
#include <sys/sysinfo.h>
#endif
#endif

#ifdef _WIN32
#include <fileapi.h>
#include <winbase.h>
#endif

#ifdef _WIN32
void CTL::System::CreateShortO() {
    CoInitialize(nullptr);
    IShellLink* psl;
    HRESULT hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hres)) {
        CTL::String str = ApplicationDirPath() + "/" + GetAppName();
        CTL::String str1 = ApplicationDirPath() + "/" + GetAppName() + ".lnk";
        CTL::String workingDir = ApplicationDirPath();
        std::wstring utf16Str;
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
        utf16Str.resize(size_needed - 1);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &utf16Str[0], size_needed);
        psl->SetPath(utf16Str.c_str());
        std::wstring workingDirUtf16;
        size_needed = MultiByteToWideChar(CP_UTF8, 0, workingDir.c_str(), -1, NULL, 0);
        workingDirUtf16.resize(size_needed - 1);
        MultiByteToWideChar(CP_UTF8, 0, workingDir.c_str(), -1, &workingDirUtf16[0], size_needed);
        psl->SetWorkingDirectory(workingDirUtf16.c_str());
        psl->SetDescription(reinterpret_cast<LPCWSTR>(reinterpret_cast<LPCSTR>(L"CC_SDK_API")));
        IPersistFile* ppf;
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
        if (SUCCEEDED(hres)) {
            WCHAR wszPath[MAX_PATH];
            MultiByteToWideChar(CP_UTF8, 0, str1.c_str(), -1, wszPath, MAX_PATH);
            IPropertyStore* pps;
            hres = psl->QueryInterface(IID_IPropertyStore, (LPVOID*)&pps);
            if (SUCCEEDED(hres)) {
                PROPVARIANT pv;
                InitPropVariantFromString(L"CC_SDK_API", &pv);
                pps->SetValue(PKEY_AppUserModel_ID, pv);
                PropVariantClear(&pv);
                pps->Commit();
                pps->Release();
            }
            hres = ppf->Save(wszPath, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    CoUninitialize();
}
bool CTL::System::DeleteShortO() {
    bool F = false;
    CTL::String str = OpenKit_Dir + GetAppName() + ".lnk";
    CCFile file(str);
    F = file.Delete();
    return F;
}
CTL::String CTL::System::OpenKit_Dir = "C:/ProgramData/Microsoft/Windows/Start Menu/Programs/StartUp/";
#elif __linux__
CTL::String CTL::System::GetUserName() {
    const char* userName = getenv("USER");
    return userName ? userName : "default_user";
}

bool CTL::System::SetFileReadPermission(const std::string &filePath) {
    struct stat fileStat{};
    if (stat(filePath.c_str(), &fileStat) != 0) {
        std::cerr << "Failed to get file status: " << filePath << std::endl;
        return false;
    }
    mode_t newPermissions = fileStat.st_mode | S_IRUSR | S_IXUSR;
    if (chmod(filePath.c_str(), newPermissions) != 0) {
        std::cerr << "Failed to set read and execute permission for file: " << filePath << std::endl;
        return false;
    }
    return true;
}
#endif

bool CTL::System::SetAppAutoStart(bool F) {
    bool A = false;
#ifdef _WIN32
    if(F){
        CreateShortO();
        CTL::String str = ApplicationDirPath() + "/" + GetAppName() + ".lnk";
        CTL::String AB = OpenKit_Dir + GetAppName() + ".lnk";
        try{
            A = CCFile::Copy(str,AB);
        }
        catch (const CCException& e){
            CC::Println(CTL::String::format("SetAppAutoStart Error -> %s",e.what()),RED);
        }
        CCFile f(str);
        f.Delete();
    }
    else {
        A = DeleteShortO();
    }
#elif __linux__
    if (F) {
        String name = GetAppName();
        const auto appPath = ApplicationDirPath();
        const std::string userName = GetUserName();

        // 检测是否为桌面环境（X11/Wayland）
        const char* displayEnv = getenv("DISPLAY");
        const char* xdgSessionType = getenv("XDG_SESSION_TYPE");
        const bool isDesktopEnv = (displayEnv && strlen(displayEnv) > 0) ||
                                  (xdgSessionType && strlen(xdgSessionType) > 0);
        // 使用用户级systemd服务
        const std::string servicePath = "/etc/systemd/system/" + name + ".service";
        std::ofstream serviceFile(servicePath);
        if (serviceFile.is_open()) {
            System::Println("systemd OK");
            serviceFile << "[Unit]\n";
            serviceFile << "Description=" << name << " Application\n";
            if (isDesktopEnv) {
                serviceFile << "After=display-manager.service graphical-session.target "
                            << "network.target network-online.target\n";
                serviceFile << "Requires=display-manager.service\n";
            } else {
                serviceFile << "After=network.target network-online.target\n";
            }
            serviceFile << "\n";
            serviceFile << "[Service]\n";
            serviceFile << "Type=simple\n";
            serviceFile << "User=" << userName << "\n";
            serviceFile << "TimeoutStartSec=infinity\n";
            serviceFile << "ExecStartPre=/bin/sleep 20\n";
            serviceFile << "ExecStart=" << appPath + "/" + name << "\n";
            serviceFile << "Environment=" << "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:"
                        << "/usr/bin:/sbin:/bin:/snap/bin:" << appPath << "\n";
            serviceFile << "WorkingDirectory=" << appPath << "\n";

            if (isDesktopEnv) {
                // X11 桌面环境：注入图形会话所需的环境变量
                serviceFile << "Environment=QT_QPA_PLATFORM=xcb\n";
                serviceFile << "Environment=DISPLAY="
                            << (displayEnv ? displayEnv : ":0") << "\n";

                const char* xauthority = getenv("XAUTHORITY");
                if (xauthority) {
                    serviceFile << "Environment=XAUTHORITY=" << xauthority << "\n";
                } else {
                    serviceFile << "Environment=XAUTHORITY=/home/" << userName
                                << "/.Xauthority\n";
                }

                const char* xdgRuntimeDir = getenv("XDG_RUNTIME_DIR");
                if (xdgRuntimeDir) {
                    serviceFile << "Environment=XDG_RUNTIME_DIR=" << xdgRuntimeDir
                                << "\n";
                }

                const char* dbusSession = getenv("DBUS_SESSION_BUS_ADDRESS");
                if (dbusSession) {
                    serviceFile << "Environment=DBUS_SESSION_BUS_ADDRESS="
                                << dbusSession << "\n";
                }
            }

            serviceFile << "\n";
            serviceFile << "[Install]\n";
            serviceFile << "WantedBy=default.target\n";
            serviceFile.close();

            // 设置文件权限
            SetFileReadPermission(servicePath);

            // 启用服务
            std::string enableCmd = "systemctl enable " + name + ".service";
            system(enableCmd.c_str());

            A = true;
        }
        else {
            const std::string initScriptPath = "/etc/init.d/S99" + name;
            std::ofstream initScript(initScriptPath);
            if (initScript.is_open()) {
                System::Println("/etc/init.d OK");
                initScript << "#!/bin/sh\n";
                initScript << "# Auto-start script for " << name << "\n";
                initScript << "\n";
                initScript << "APP_NAME=\"" << name << "\"\n";
                initScript << "APP_PATH=\"" << appPath + "/" + name << "\"\n";
                initScript << "WORK_DIR=\"" << appPath << "\"\n";
                initScript << "\n";
                initScript << "# 等待系统完全启动\n";
                initScript << "sleep 3\n";
                initScript << "\n";
                initScript << "# 切换到工作目录\n";
                initScript << "cd $WORK_DIR\n";
                initScript << "\n";
                initScript << "# 启动应用（后台运行）\n";
                initScript << "if [ -x \"$APP_PATH\" ]; then\n";
                initScript << "    echo \"Starting $APP_NAME...\"\n";
                initScript << "    $APP_PATH &\n";
                initScript << "    echo \"$APP_NAME started with PID $!\"\n";
                initScript << "else\n";
                initScript << "    echo \"Error: $APP_PATH not found or not executable\"\n";
                initScript << "fi\n";
                initScript.close();

                // 设置执行权限
                std::string chmodCmd = "chmod +x " + initScriptPath;
                system(chmodCmd.c_str());

                A = true;
                System::Println("Created init script: " + initScriptPath);
            }
        }
    }
    else {
        const String name = GetAppName();
        const std::string servicePath = "/etc/systemd/system/" + name + ".service";

        // 禁用服务
        std::string disableCmd = "systemctl disable " + name + ".service";
        system(disableCmd.c_str());

        // 删除服务文件
        std::remove(servicePath.c_str());

        const std::string initScriptPath = "/etc/init.d/S99" + name;
        std::remove(initScriptPath.c_str());

        A = true;
    }
#endif
    return A;
}

bool CTL::System::IsAppAutoStart() {
    bool F = false;
#ifdef _WIN32
    CTL::String AB = OpenKit_Dir + GetAppName() + ".lnk";
    CCFile file(AB);
    F = file.isExists();
#elif __linux__
    const String name = GetAppName();

    // 方法1: 检查systemd服务
    const std::string servicePath = "/etc/systemd/system/" + name + ".service";
    if (access(servicePath.c_str(), F_OK) == 0) {
        const std::string checkCmd = "systemctl is-enabled " + name + ".service > /dev/null 2>&1";
        const int result = system(checkCmd.c_str());
        if (result == 0) {
            F = true;
        }
    }

    // 方法2: 检查init.d脚本
    if (!F) {
        const std::string initScriptPath = "/etc/init.d/S99" + name;
        if (access(initScriptPath.c_str(), F_OK) == 0) {
            F = true;
        }
    }
#endif
    return F;
}

CTL::String CTL::System::GetAppName() {
    CTL::String A;
#ifdef _WIN32
    char buffer[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    A = std::string(buffer).substr(pos + 1);
#elif __linux__
    char buffer[PATH_MAX];
    const ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        std::string::size_type pos = std::string(buffer).find_last_of("\\/");
        A = std::string(buffer).substr(pos + 1);
    }
#endif
    return A;
}

CTL::String CTL::System::ApplicationDirPath() {
    return CCFile::NormalizePath("./");
}

void CTL::System::Println(const CTL::String &str) {
#ifdef CC_OHOS_
    OH_LOG_Print(LOG_APP, LOG_INFO, 0x0728, "CC_SDK", "%{public}s",str.c_str());
#else
    CC::Println(str);
#endif
}

void CTL::System::ReleaseProcessHeap() {
#if CC_OHOS_

#elif __linux__

#elif _WIN32
    const HANDLE heap = GetProcessHeap();
    HeapCompact(heap, 0);
    HeapDestroy(heap);
    SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
#endif
}

unsigned int CTL::System::GetPhysicalCores() {
#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwNumberOfProcessors;
#elif __linux__
    return sysconf(_SC_NPROCESSORS_CONF);
#endif
}

unsigned int CTL::System::GetLogicalCores() {
    return std::thread::hardware_concurrency();
}

float CTL::System::GetCPUUsage() {
#ifdef _WIN32
    static PDH_HQUERY query = nullptr;
    static PDH_HCOUNTER counter = nullptr;
    static bool initialized = false;

    if (!initialized) {
        // 1. 初始化查询
        if (PdhOpenQuery(nullptr, 0, &query) != ERROR_SUCCESS) {
            return -1.0f;
        }

        // 2. 使用宽字符版本并直接指定英文计数器路径
        const wchar_t* counterPath = L"\\Processor(_Total)\\% Processor Time";
        if (PdhAddCounterW(query, counterPath, 0, &counter) != ERROR_SUCCESS) {
            PdhCloseQuery(query);
            return -1.0f;
        }

        // 3. 首次收集数据
        PdhCollectQueryData(query);
        initialized = true;
        return 0.0f; // 首次返回0
    }

    // 4. 收集新数据
    if (PdhCollectQueryData(query) != ERROR_SUCCESS) {
        return -1.0f;
    }

    // 5. 获取格式化值
    PDH_FMT_COUNTERVALUE value;
    if (PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, nullptr, &value) != ERROR_SUCCESS) {
        return -1.0f;
    }

    return static_cast<float>(value.doubleValue);
#elif __OHOS__
    return OH_HiDebug_GetSystemCpuUsage() * 100;
#elif __linux__
    static unsigned long prevIdle = 0, prevTotal = 0;

    std::ifstream file("/proc/stat");
    std::string line;
    std::getline(file, line);

    unsigned long user, nice, system, idle;
    sscanf(line.c_str() + 5, "%lu %lu %lu %lu", &user, &nice, &system, &idle);

    unsigned long total = user + nice + system + idle;
    float usage = 100.0f * (1.0f - float(idle - prevIdle) / (total - prevTotal));

    prevIdle = idle;
    prevTotal = total;
    return usage;
#endif
}

CTL::MemoryInfo CTL::System::GetMemoryInfo() {
    CTL::MemoryInfo A = {};
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    A.total = status.ullTotalPhys;
    A.available = status.ullAvailPhys;
    A.used = A.total - A.available;
#elif __OHOS__
    HiDebug_SystemMemInfo memInfo;
    OH_HiDebug_GetSystemMemInfo(&memInfo);
    A.total = memInfo.totalMem;
    A.available = memInfo.availableMem;
    A.used = A.total - A.available;
#elif __linux__
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        std::string line;
        unsigned long memTotal = 0;
        unsigned long memFree = 0;
        unsigned long memAvailable = 0;
        unsigned long buffers = 0;
        unsigned long cached = 0;
        unsigned long sreclaimable = 0;

        while (std::getline(meminfo, line)) {
            std::istringstream iss(line);
            std::string key;
            unsigned long value;
            std::string unit;

            if (iss >> key >> value >> unit) {
                if (key == "MemTotal:") {
                    memTotal = value;
                } else if (key == "MemFree:") {
                    memFree = value;
                } else if (key == "MemAvailable:") {
                    memAvailable = value;
                } else if (key == "Buffers:") {
                    buffers = value;
                } else if (key == "Cached:") {
                    cached = value;
                } else if (key == "SReclaimable:") {
                    sreclaimable = value;
                }
            }
        }
        meminfo.close();

        // /proc/meminfo 中的值单位是 kB，转换为字节
        A.total = static_cast<unsigned long long>(memTotal) * 1024ULL;

        // 如果有 MemAvailable，优先使用（更准确）
        // 否则使用 Free + Buffers + Cached + SReclaimable
        if (memAvailable > 0) {
            A.available = static_cast<unsigned long long>(memAvailable) * 1024ULL;
        } else {
            A.available = static_cast<unsigned long long>(memFree + buffers + cached + sreclaimable) * 1024ULL;
        }

        A.used = A.total - A.available;
    }
#endif
    return A;
}

CTL::DiskInfo CTL::System::GetDiskInfo() {
DiskInfo diskInfo;
    diskInfo.totalDiskSize = 0;
    diskInfo.totalUsedSize = 0;
    diskInfo.totalAvailableSize = 0;
    diskInfo.overallUsagePercent = 0.0f;

#ifdef _WIN32
    // Windows 平台 - 遍历所有驱动器
    DWORD drives = GetLogicalDrives();
    char driveLetter = 'A';

    while (drives > 0) {
        if (drives & 1) {
            CTL::String mountPoint = CTL::String::format("%c:\\", driveLetter);
            ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;

            if (GetDiskFreeSpaceExA(mountPoint.c_str(), &freeBytesAvailable,
                                     &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
                DiskPartitionInfo partition;
                partition.mountPoint = mountPoint;
                partition.deviceName = mountPoint;
                partition.totalSize = totalNumberOfBytes.QuadPart;
                partition.usedSize = totalNumberOfBytes.QuadPart - totalNumberOfFreeBytes.QuadPart;
                partition.availableSize = totalNumberOfFreeBytes.QuadPart;
                partition.usagePercent = (partition.totalSize > 0) ?
                    (float(partition.usedSize) / partition.totalSize * 100.0f) : 0.0f;

                // 获取文件系统类型
                char fileSystem[MAX_PATH] = {0};
                GetVolumeInformationA(mountPoint.c_str(), nullptr, 0, nullptr,
                                     nullptr, nullptr, fileSystem, MAX_PATH);
                partition.fileSystemType = fileSystem;

                diskInfo.partitions.push_back(partition);
                diskInfo.totalDiskSize += partition.totalSize;
                diskInfo.totalUsedSize += partition.usedSize;
                diskInfo.totalAvailableSize += partition.availableSize;
            }
        }
        drives >>= 1;
        driveLetter++;
    }

#elif __linux__
    // Linux/OpenHarmony 平台 - 读取 /proc/mounts
    std::ifstream mountsFile("/proc/mounts");
    std::string line;

    while (std::getline(mountsFile, line)) {
        std::istringstream iss(line);
        std::string device, mountPoint, fsType;

        if (iss >> device >> mountPoint >> fsType) {
            // 跳过虚拟文件系统、临时文件系统等
            if (device.find("tmpfs") != std::string::npos ||
                device.find("devtmpfs") != std::string::npos ||
                device.find("overlay") != std::string::npos ||
                device.find("squashfs") != std::string::npos ||
                device.find("proc") != std::string::npos ||
                device.find("sysfs") != std::string::npos ||
                device.find("debugfs") != std::string::npos ||
                device.find("cgroup") != std::string::npos ||
                device.find("pstore") != std::string::npos ||
                device.find("configfs") != std::string::npos ||
                device.find("fusectl") != std::string::npos ||
                device.find("mqueue") != std::string::npos ||
                device.find("hugetlbfs") != std::string::npos ||
                device.find("securityfs") != std::string::npos) {
                continue;
            }

            // 只统计物理设备和网络文件系统
            bool isPhysicalDevice = (device.find("/dev/") == 0) ||
                                   (device.find("/dev/block/") == 0) ||
                                   (fsType == "nfs") || (fsType == "cifs") ||
                                   (fsType == "vfat") || (fsType == "exfat") ||
                                   (fsType == "ext4") || (fsType == "ext3") ||
                                   (fsType == "xfs") || (fsType == "btrfs") ||
                                   (fsType == "f2fs");

            if (!isPhysicalDevice && mountPoint != "/" && mountPoint != "/system" &&
                mountPoint != "/vendor" && mountPoint != "/product" &&
                mountPoint != "/odm" && mountPoint != "/data") {
                continue;
            }

            struct statvfs stat;
            if (statvfs(mountPoint.c_str(), &stat) == 0) {
                DiskPartitionInfo partition;
                partition.mountPoint = mountPoint.c_str();
                partition.deviceName = device.c_str();
                partition.fileSystemType = fsType.c_str();

                // 计算大小（字节）
                partition.totalSize = stat.f_blocks * stat.f_frsize;
                partition.availableSize = stat.f_bavail * stat.f_frsize;
                partition.usedSize = partition.totalSize - (stat.f_bfree * stat.f_frsize);
                partition.usagePercent = (partition.totalSize > 0) ?
                    (float(partition.usedSize) / partition.totalSize * 100.0f) : 0.0f;

                diskInfo.partitions.push_back(partition);
                diskInfo.totalDiskSize += partition.totalSize;
                diskInfo.totalUsedSize += partition.usedSize;
                diskInfo.totalAvailableSize += partition.availableSize;
            }
        }
    }

    mountsFile.close();
#endif

    // 计算总体使用率
    if (diskInfo.totalDiskSize > 0) {
        diskInfo.overallUsagePercent = float(diskInfo.totalUsedSize) /
                                       diskInfo.totalDiskSize * 100.0f;
    }

    return diskInfo;
}

CTL::DiskPartitionInfo CTL::System::GetCurrentDriveInfo() {
    // 获取程序执行文件的路径
    const CTL::String appPath = CTL::System::ApplicationDirPath();

#ifdef _WIN32
    // Windows: 提取盘符 (例如 "C:\\")
    if (appPath.length() >= 2) {
        CTL::String driveLetter = appPath.substr(0, 2);  // "C:"
        if (driveLetter[1] != ':') {
            driveLetter = appPath.substr(0, 1) + ":";
        }
        return CTL::System::GetDiskInfo(driveLetter);
    }
#elif __OHOS__
    return CTL::System::GetDiskInfo("/data");
#elif __linux__
    // Linux: 直接使用程序目录路径
    return CTL::System::GetDiskInfo(appPath);
#endif

    return CTL::DiskPartitionInfo();
}

CTL::DiskPartitionInfo CTL::System::GetDiskInfo(const CTL::String& mountPoint) {
    DiskPartitionInfo info;
    info.mountPoint = mountPoint;
    info.totalSize = 0;
    info.usedSize = 0;
    info.availableSize = 0;
    info.usagePercent = 0.0f;
    info.fileSystemType = "";
    info.deviceName = "";

#ifdef _WIN32
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;

    if (GetDiskFreeSpaceExA(mountPoint.c_str(), &freeBytesAvailable,
                           &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        info.totalSize = totalNumberOfBytes.QuadPart;
        info.usedSize = totalNumberOfBytes.QuadPart - totalNumberOfFreeBytes.QuadPart;
        info.availableSize = totalNumberOfFreeBytes.QuadPart;
        info.usagePercent = (info.totalSize > 0) ?
            (float(info.usedSize) / info.totalSize * 100.0f) : 0.0f;

        // 获取文件系统类型
        char fileSystem[MAX_PATH] = {0};
        GetVolumeInformationA(mountPoint.c_str(), nullptr, 0, nullptr,
                             nullptr, nullptr, fileSystem, MAX_PATH);
        info.fileSystemType = fileSystem;
        info.deviceName = mountPoint;
    }

#elif __linux__
    struct statvfs stat;
    if (statvfs(mountPoint.c_str(), &stat) == 0) {
        info.mountPoint = mountPoint;

        // 计算大小（字节）
        info.totalSize = stat.f_blocks * stat.f_frsize;
        info.availableSize = stat.f_bavail * stat.f_frsize;
        info.usedSize = info.totalSize - (stat.f_bfree * stat.f_frsize);
        info.usagePercent = (info.totalSize > 0) ?
            (float(info.usedSize) / info.totalSize * 100.0f) : 0.0f;

        // 尝试从 /proc/mounts 获取设备名称和文件系统类型
        std::ifstream mountsFile("/proc/mounts");
        std::string line;
        while (std::getline(mountsFile, line)) {
            std::istringstream iss(line);
            std::string dev, mp, fsType;
            if (iss >> dev >> mp >> fsType) {
                if (mp == mountPoint.c_str()) {
                    info.deviceName = dev.c_str();
                    info.fileSystemType = fsType.c_str();
                    break;
                }
            }
        }
        mountsFile.close();
    }
#endif

    return info;
}

CTL::String CTL::System::GetMac() {
#ifdef _WIN32
    PIP_ADAPTER_INFO pAdapterInfo = nullptr;
    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    CTL::String macAddress;

    // 第一次调用获取缓冲区大小
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulOutBufLen);
    }

    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
        while (pAdapter) {
            // 跳过虚拟适配器（如VPN、虚拟机等）
            if (pAdapter->Type != MIB_IF_TYPE_ETHERNET &&
                pAdapter->Type != IF_TYPE_IEEE80211) {
                pAdapter = pAdapter->Next;
                continue;
                }

            // 获取第一个物理适配器的MAC地址
            char macStr[18] = {0};
            sprintf_s(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                     pAdapter->Address[0], pAdapter->Address[1],
                     pAdapter->Address[2], pAdapter->Address[3],
                     pAdapter->Address[4], pAdapter->Address[5]);
            macAddress = macStr;
            break;
        }
    }

    if (pAdapterInfo) {
        free(pAdapterInfo);
    }

    return macAddress;
#elif __linux__
    struct ifconf ifc;
    struct ifreq *ifr;
    char buffer[4096];
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    CTL::String macAddress;

    if (sock >= 0) {
        // 设置ifconf结构
        ifc.ifc_len = sizeof(buffer);
        ifc.ifc_buf = buffer;

        // 获取所有网络接口
        if (ioctl(sock, SIOCGIFCONF, &ifc) >= 0) {
            // 遍历所有网络接口
            int numInterfaces = ifc.ifc_len / sizeof(struct ifreq);
            ifr = ifc.ifc_req;

            for (int i = 0; i < numInterfaces; i++) {
                // 跳过回环接口
                if (strcmp(ifr[i].ifr_name, "lo") == 0) {
                    continue;
                }

                // 获取该接口的MAC地址
                // strcpy(ifr[i].ifr_name, ifr[i].ifr_name);
                // strncpy(ifr[i].ifr_name, ifr[i].ifr_name, sizeof(ifr[i].ifr_name) - 1);  // 限制写入长度
                if (ioctl(sock, SIOCGIFHWADDR, &ifr[i]) >= 0) {
                    // 检查是否是以太网接口
                    if (ifr[i].ifr_hwaddr.sa_family == ARPHRD_ETHER) {
                        unsigned char* mac = (unsigned char*)ifr[i].ifr_hwaddr.sa_data;
                        // 检查MAC地址是否全为0
                        if (mac[0] != 0 || mac[1] != 0 || mac[2] != 0 ||
                            mac[3] != 0 || mac[4] != 0 || mac[5] != 0) {

                            char macStr[18] = {0};
                            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                            macAddress = macStr;
                            break;
                            }
                    }
                }
            }
        }
        close(sock);
    }

    // 如果仍然没有获取到MAC地址，则返回空字符串
    return macAddress;
#endif
}

CTL::String CTL::System::GetOSName() {
#ifdef _WIN32
    // 方法2：精确版本号检测（适用于所有情况）
    OSVERSIONINFOEX osvi = { sizeof(OSVERSIONINFOEX) };
    GetVersionEx(reinterpret_cast<OSVERSIONINFO *>(&osvi));

    if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0) {
        if (osvi.dwBuildNumber >= 22000) {
            return "Windows 11";
        }
        else {
            return "Windows 10";
        }
    }
    else if (osvi.dwMajorVersion == 6) {
        switch (osvi.dwMinorVersion) {
            case 3: return "Windows 8.1";
            case 2: return "Windows 8";
            case 1: return "Windows 7";
            case 0: return "Windows Vista";
            default: return "Unknown Windows Version";;
        }
    }
    else if (osvi.dwMajorVersion == 5) {
        switch (osvi.dwMinorVersion) {
            case 2:
                if (GetSystemMetrics(SM_SERVERR2))
                    return "Windows Server 2003 R2";
                else if (osvi.wProductType == VER_NT_WORKSTATION)
                    return "Windows XP Professional x64";
                else return "Windows Server 2003";
            case 1: return "Windows XP";
            case 0: return "Windows 2000";
            default: return "Unknown Windows Version"; ;
        }
    }
#elif CC_OHOS_
    return "OpenHarmony";
#elif __linux__
    return "Linux";
#endif
    return "Unknown";
}

CTL::OS CTL::System::GetOS() {
    const auto os = GetOSName();
    return GetOS(os);
}

CTL::OS CTL::System::GetOS(const CTL::String &os) {
    if (os == "Windows 11") {
        return OS::Windows11;
    }
    else if (os == "Windows 10") {
        return OS::Windows10;
    }
    else if (os == "Windows 8.1") {
        return OS::Windows8;
    }
    else if (os == "Windows 8") {
        return OS::Windows8;
    }
    else if (os == "Windows 7") {
        return OS::Windows7;
    }
    else if (os == "Windows Vista") {
        return OS::WindowsXP;
    }
    else if (os == "Windows XP") {
        return OS::WindowsXP;
    }
    else if (os == "Windows 2000") {
        return OS::WindowsXP;
    }
    else if (os == "Windows Server 2003") {
        return OS::WindowsServer;
    }
    else if (os == "OpenHarmony") {
        return OS::OpenHarmony;
    }
    else if (os == "Linux") {
        return OS::Linux;
    }
    return OS::Unknown;
}

void CTL::System::Exit(int code) {
    exit(code);
}

void CTL::System::Execute(const CTL::String &cmd) {
    system(cmd.c_str());
}
