#ifndef API_URL_H
#define API_URL_H

#include <functional>
#include <map>
#include <mutex>
#include <string>

#include "CameraSession.h"
#include "CCRequest.h"
#include "CCWebSocket.h"

using RoutHandler = std::function<std::string(CTL::Request&, int&)>;

class URL {
public:

    URL();
    ~URL();
    // ===== 初始化 & 路由注册 =====
    void Init();
    void add(const std::string& path, RoutHandler handle);

    // ===== 路由分发 =====
    bool Dispatch(const std::string& path, CTL::Request& request,
                  std::string& result, int& status);

    // ===== 路由查询 =====
    size_t Size() const;
    void PrintRoutes() const;
    std::map<std::string, RoutHandler> Getroutes() { return routes; }

    // ===== 路由处理函数 =====
    // 用户认证
    static std::string HandleRegister(CTL::Request& request, int& status);
    // ID 分配
    static std::string HandleID(CTL::Request& request, int& status);
    // 摄像头列表
    static std::string HandleCameraList(CTL::Request& request, int& status);
    // 摄像头实时帧
    static std::string HandleCameraFrame(CTL::Request& request, int& status);
    // 拍照
    static std::string HandleTakePhoto(CTL::Request& request, int& status);
    // 推流控制
    std::string HandleStreamControl(CTL::Request& request, int& status);

    void Onopen(CTL::WebSocketInfo& info);

private:
    std::map<std::string, RoutHandler> routes;
    static CCVector<int> all_id;
    static std::mutex containerMutex;
    CTL::WebSocket *websock;
    std::thread m_thread;
};

#endif
