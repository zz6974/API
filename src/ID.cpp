#include "../include/ID.h"
#include "CCSystem.h"
#include "TL/Map.h"
#include "mutex"
#include "../include/CameraManager.h"

inline static CCVector<bool> ID_t(20, false); // 使用bool数组，初始化为false表示未使用
std::mutex mutex_t;
//拿ID
int GetID() {
    CCUniqueLock lock(mutex_t);
    for (int i = 0; i < 20; i++) {
        if (!ID_t[i]) { // 如果第i个位置为false，表示未被使用
            ID_t[i] = true; // 标记为已使用
            return i;
        }
    }
    return -1; // 所有ID都已用完
}
//释放ID
void ReleaseID(int id) {
    CCUniqueLock lock(mutex_t);
    if (id >= 0 && id < 20) {
        ID_t[id] = false; // 标记为未使用
    }
}
//查看ID状态
CCVector<bool>  LookAllID() {
    CCUniqueLock lock(mutex_t);
    CCVector<bool> ID_L = ID_t;
    return ID_L;
}

//查看ID是否已分配
bool IsIDAllocated(int id) {
    CCUniqueLock lock(mutex_t);
    if (id >= 0 && id < 20) {
        return ID_t[id];
    }
    return false;
}

//判断摄像头在线状况
bool CameraOnline(int id) {
    auto devices = CameraManager::Instance().OnlineDevices();
    for (int deviceId : devices) {
        if (deviceId == id) return true;
    }
    return false;
}