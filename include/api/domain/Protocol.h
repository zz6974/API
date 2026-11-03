#pragma once

namespace api::domain::protocol {

// 摄像头终端 -> 服务端
inline constexpr const char* kPong        = "PONG";        // 心跳回复
inline constexpr const char* kRequestId   = "REQUEST_ID";  // 请求分配 ID
inline constexpr const char* kLogin       = "LOGIN";       // 终端登录
inline constexpr const char* kStreamData  = "STREAM_DATA"; // 推流帧头（后跟长度 + 二进制）
inline constexpr const char* kPhotoData   = "PHOTO_DATA";  // 照片头（后跟长度 + 二进制）

// 服务端 -> 摄像头终端
inline constexpr const char* kAssignId    = "ASSIGN_ID";    // 下发分配的 ID
inline constexpr const char* kLoginOk     = "LOGIN_OK";     // 登录成功
inline constexpr const char* kLoginFail   = "LOGIN_FAIL";   // 登录失败
inline constexpr const char* kStreamStart = "STREAM_START"; // 开始推流
inline constexpr const char* kStreamStop  = "STREAM_STOP";  // 停止推流
inline constexpr const char* kTakePhoto   = "TAKE_PHOTO";   // 拍照

} // namespace api::domain::protocol
