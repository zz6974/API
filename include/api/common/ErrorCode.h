#pragma once
#include <string>

namespace api::common {

// 统一业务错误码，Interface 层负责映射为 HTTP 状态码
enum class ErrorCode {
    kOk = 0,
    kInvalidArgument,   // 参数错误
    kUnauthorized,      // 未授权
    kNotFound,          // 资源不存在
    kDeviceOffline,     // 设备不存在或离线
    kTimeout,           // 操作超时
    kIoError,           // IO 错误
    kNoIdAvailable,     // ID 池耗尽
    kInternal,          // 内部错误
};

inline std::string ToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::kOk:              return "OK";
        case ErrorCode::kInvalidArgument: return "InvalidArgument";
        case ErrorCode::kUnauthorized:    return "Unauthorized";
        case ErrorCode::kNotFound:        return "NotFound";
        case ErrorCode::kDeviceOffline:   return "DeviceOffline";
        case ErrorCode::kTimeout:         return "Timeout";
        case ErrorCode::kIoError:         return "IoError";
        case ErrorCode::kNoIdAvailable:   return "NoIdAvailable";
        case ErrorCode::kInternal:        return "Internal";
    }
    return "Unknown";
}

} // namespace api::common
