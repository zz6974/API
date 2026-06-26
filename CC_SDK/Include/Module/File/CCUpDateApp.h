#ifndef PROCESS_CC_UPDATE_APP_H
#define PROCESS_CC_UPDATE_APP_H

#ifdef _WIN32
#include <windows.h>
#include <cwchar>
#include <string>
#include <strsafe.h>
#elif __linux__
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#endif

namespace CTL{
    class UpDateApp{
    public:
        static int Update(const char * new_exe_path){
#ifdef _WIN32
            size_t newsize = strlen(new_exe_path) + 1;
            auto* wcstring = new wchar_t[newsize];
            size_t convertedChars = 0;
            mbstowcs_s(&convertedChars, wcstring, newsize, new_exe_path, _TRUNCATE);

            // 1. 获取当前执行文件路径
            wchar_t current_exe_path[MAX_PATH];
            if (0 == GetModuleFileNameW(NULL, current_exe_path, MAX_PATH)) {
                fwprintf(stderr, L"获取当前路径失败 (Error %lu)\n", GetLastError());
                delete[] wcstring;
                return 1;
            }

            // 2. 创建备份路径
            wchar_t backup_path[MAX_PATH];
            if (FAILED(StringCchPrintfW(backup_path, MAX_PATH,
                                        L"%s.old", current_exe_path))) {
                fwprintf(stderr, L"创建备份路径失败\n");
                delete[] wcstring;
                return 2;
            }

            // 3. 原子替换操作
            if (!MoveFileExW(current_exe_path, backup_path,
                             MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
                fwprintf(stderr, L"移动旧文件失败 (Error %lu)\n", GetLastError());
                delete[] wcstring;
                return 3;
            }
            if (!MoveFileExW(wcstring, current_exe_path,
                             MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
                fwprintf(stderr, L"替换新文件失败 (Error %lu)\n", GetLastError());
                return 4;
            }
            delete[] wcstring;
            // 4. 启动新进程
            STARTUPINFOW si = { sizeof(STARTUPINFOW) };
            PROCESS_INFORMATION pi;
            if (!CreateProcessW(
                    current_exe_path,   // 新程序路径
                    GetCommandLineW(),  // 继承命令行参数
                    NULL, NULL, FALSE,
                    CREATE_NEW_CONSOLE,
                    NULL, NULL, &si, &pi)) {
                fwprintf(stderr, L"启动新进程失败 (Error %lu)\n", GetLastError());
                return 5;
            }

            // 5. 清理句柄
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            return 0; // 原进程可安全退出
#elif __linux__
    // 1. 获取当前可执行文件路径
    char current_exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", current_exe_path, sizeof(current_exe_path)-1);
    if (len == -1) {
        perror("获取当前路径失败");
        return 1;
    }
    current_exe_path[len] = '\0';

    // 2. 创建备份路径
    char backup_path[PATH_MAX];
    if (snprintf(backup_path, PATH_MAX, "%s.old", current_exe_path) >= PATH_MAX) {
        fprintf(stderr, "备份路径过长\n");
        return 2;
    }

    // 3. 删除旧备份（如果有）
    if (unlink(backup_path) == -1 && errno != ENOENT) {
        perror("删除旧备份失败");
        return 3;
    }

    // 4. 原子替换操作
    if (rename(current_exe_path, backup_path) == -1) {
        perror("重命名旧文件失败");
        return 4;
    }

    if (rename(new_exe_path, current_exe_path) == -1) {
        perror("替换新文件失败");
        // 尝试恢复备份
        rename(backup_path, current_exe_path);
        return 5;
    }

    // 5. 设置可执行权限
    struct stat st;
    if (stat(current_exe_path, &st) == -1) {
        perror("获取文件状态失败");
        return 6;
    }

    if (chmod(current_exe_path, st.st_mode | S_IXUSR) == -1) {
        perror("设置可执行权限失败");
        return 7;
    }
    return 8;
#endif
        }
    };
}

#endif
