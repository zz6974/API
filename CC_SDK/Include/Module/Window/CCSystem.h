#ifndef CC_SysTem_H
#define CC_SysTem_H

#include "basic/CC.h"

#ifdef _WIN32

#elif __linux__

#endif

namespace CTL{
    struct MemoryInfo {
        unsigned long long total;
        unsigned long long available;
        unsigned long long used;
    };
    // 磁盘分区信息结构体
    struct DiskPartitionInfo {
        CTL::String mountPoint;      // 挂载点
        CTL::String deviceName;      // 设备名称
        CTL::String fileSystemType;  // 文件系统类型
        size_t totalSize;            // 总大小 (字节)
        size_t usedSize;             // 已使用大小 (字节)
        size_t availableSize;        // 可用大小 (字节)
        float usagePercent;          // 使用百分比
    };
    // 磁盘信息汇总结构体
    struct DiskInfo {
        std::vector<DiskPartitionInfo> partitions;  // 所有分区列表
        size_t totalDiskSize;        // 总磁盘大小
        size_t totalUsedSize;        // 总已使用大小
        size_t totalAvailableSize;   // 总可用大小
        float overallUsagePercent;   // 总体使用百分比
    };
    enum OS {
        Windows11 = 1,
        Windows10 = 2,
        Windows8 = 3,
        Windows7 = 4,
        WindowsXP = 5,
        WindowsServer = 6,
        Linux = 7,
        OpenHarmony = 0,
        Unknown = -1
    };
    class System{
    public:
#ifdef _WIN32
        static void CreateShortO();
        static bool DeleteShortO();
        static CTL::String OpenKit_Dir;
#elif __linux__
        static CTL::String GetUserName();
        static bool SetFileReadPermission(const std::string& filePath);
#endif
        static bool SetAppAutoStart(bool F);
        static bool IsAppAutoStart();
        static CTL::String GetAppName();
        static CTL::String ApplicationDirPath();
        static void Exit(int code = 0);
        static void Execute(const CTL::String& cmd);
        template<typename... Args>
        static void Println(const char* fmt, Args... args);
        static void Println(const CTL::String& str);
        static void ReleaseProcessHeap();
        static unsigned int GetPhysicalCores();
        static unsigned int GetLogicalCores();
        static float GetCPUUsage();
        static MemoryInfo GetMemoryInfo();
        static DiskInfo GetDiskInfo();
        static CTL::DiskPartitionInfo GetCurrentDriveInfo();
        static DiskPartitionInfo GetDiskInfo(const CTL::String& mountPoint);
        static String GetMac();
        static CTL::String GetOSName();
        static OS GetOS();
    private:
        static OS GetOS(const CTL::String& os);
    };

    template<typename ... Args>
    void System::Println(const char *fmt, Args... args) {
        const std::string str = CC::format(fmt,args...);
        #ifdef CC_OHOS_
            OH_LOG_Print(LOG_APP, LOG_INFO, 0x0728, "CC_SDK", "%{public}s",str.c_str());
        #else
            CC::Println(str);
        #endif
    }
}

#endif
