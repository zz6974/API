#include <iostream>
#include <thread>
#include "time.h"
#include "../include/PhotoManager.h"
#include "../include/URL.h"

#include "httplib.h"
#include "../include/ID.h"
#include "CCWebSocket.h"
#include "../include/CameraSession.h"
#include "../include/CameraManager.h"

const std::map<std::string, std::string> user_message{
    {"Account", "123456"},
    {"Password", "123456"}
};

CCVector<int> URL::all_id;
std::mutex URL::containerMutex;

// ============================================================
// Init / add / Dispatch / 查询
// ============================================================

URL::URL() {
    websock = new CTL::WebSocket();
}

URL::~URL() {
    if (websock) {
        websock->Stop();
    }
    if (m_thread.joinable()) {
        m_thread.join();
    }
    delete websock;
}

void URL::Init() {
    add("/api/login", [](CTL::Request& request, int& status) -> std::string {
        return HandleRegister(request, status);
    });
    // add("/api/GetID", [](CTL::Request& req, int& status) -> std::string {
    //     return HandleID(req, status);
    // });
    add("/api/camera/list", [](CTL::Request& req, int& status) -> std::string {
        return HandleCameraList(req, status);
    });
    websock->Init("192.168.1.175" , 8081);
    if (websock) {
        websock->OnBind("/" ,CC_WSFunClass(Onopen),
            nullptr , nullptr , nullptr
            ,nullptr,nullptr);
    }
    m_thread = std::thread([this]() {
            websock->Running();
    });
}

bool URL::Dispatch(const std::string& path, CTL::Request& request,
                   std::string& result, int& status) {
    auto it = routes.find(path);
    if (it != routes.end()) {
        result = it->second(request, status);
        return true;
    }
    return false;
}

void URL::add(const std::string& path, RoutHandler handle) {
    routes[path] = std::move(handle);
}

size_t URL::Size() const {
    return routes.size();
}

void URL::PrintRoutes() const {
    for (const auto& route : routes) {
        std::cerr << "[Thread " << std::this_thread::get_id()
                  << "] Route: " << route.first.c_str() << std::endl;
    }
}

// ============================================================
// 路由处理函数 — 认证 & ID
// ============================================================

std::string URL::HandleRegister(CTL::Request& request, int& status) {
    std::cerr << "[Thread " << std::this_thread::get_id()
              << "] [Route] POST /api/login" << std::endl;
    if (request.RequestData.Body.IsEmpty()) {
        return R"({"code":400,"msg":"Body 不能为空"})";
    }
    try {
        CTL::JSONObject json = request.GetJson();
        std::string account = json.get("Account");
        std::string password = json.get("Password");

        auto it = user_message.find("Account");
        bool accountValid = it != user_message.end() && it->second == account;

        it = user_message.find("Password");
        bool passwordValid = it != user_message.end() && it->second == password;

        if (accountValid && passwordValid) {
            json.put("status", "200");
        } else {
            json.put("status", "fail");
        }
        status = 200;
        return json.to_String();
    } catch (const std::exception& e) {
        std::cerr << "[Thread " << std::this_thread::get_id()
                  << "] [Login] 异常: " << e.what() << std::endl;
        status = 500;
        CTL::JSONObject err;
        err["code"] = 500;
        err["msg"] = e.what();
        return err.to_String();
    }
}

std::string URL::HandleID(CTL::Request& request, int& status) {
    std::cerr << "[Thread " << std::this_thread::get_id()
              << "] [Route] GET /api/GetID" << std::endl;
    int i = GetID();
    std::cerr << "ID: " << i << std::endl;
    {
        CCUniqueLock lock(URL::containerMutex);
        URL::all_id.push_back(i);
    }
    std::string cam = "cam" + std::to_string(i);
    status = 200;
    return CTL::JSONObject{{cam, i}}.to_String();
}

// ============================================================
// 路由处理函数 — 摄像头列表
// ============================================================

std::string URL::HandleCameraList(CTL::Request& request, int& status) {
    std::cerr << "[Route] GET /api/camera/list" << std::endl;
    try {
        auto devices = CameraManager::Instance().OnlineDevices();

        nlohmann::json json;
        json["devices"] = nlohmann::json::array();
        for (int id : devices) {
            json["devices"].push_back(id);
        }
        json["count"] = static_cast<int>(devices.size());

        status = 200;
        return json.dump();
    } catch (const std::exception& e) {
        std::cerr << "[HandleCameraList] 异常: " << e.what() << std::endl;
        status = 500;
        nlohmann::json err;
        err["code"] = 500;
        err["msg"] = e.what();
        return err.dump();
    }
}

//路由处理函数,frame
std::string URL::HandleCameraFrame(CTL::Request& request, int& status) {
    std::string path = request.RequestData.Path;
    std::string idstr = path.substr(12);
    auto pos = idstr.find("/frame");
    if (pos == std::string::npos) {
        status = 400;
        return R"({"code":400,"msg":"invalid path"})";
    }
    idstr = idstr.substr(0, pos);
    int deviceId = 0;
    try {
        deviceId = std::stoi(idstr);
    } catch (...) {
        status = 400;
        return R"({"code":400,"msg":"invalid device id"})";
    }

    std::cerr << "[Route] GET /api/camera/" << deviceId << "/frame" << std::endl;
    // auto frame = CameraManager::Instance().GetFrame(deviceId);
    // if (frame.empty()) {
    //     status = 400;
    //     return R"({"code":404,"msg":"no frame available"})";
    // }
    //
    // std::string raw(frame.begin(), frame.end());
    // std::string b64 = httplib::detail::base64_encode(raw);

    CTL::JSONObject json;
    // json["frame"]   = b64;
    // json["size"]    = static_cast<int>(raw.size());
    json["deviceId"] = deviceId;
    status = 200;
    return json.to_String();
}

// ============================================================
// 路由处理函数 — 拍照
// ============================================================

std::string URL::HandleTakePhoto(CTL::Request& request, int& status) {
    std::string path = request.RequestData.Path;
    std::string idStr = path.substr(12);              // 跳过 "/api/camera/"
    auto pos = idStr.find("/photo");
    if (pos == std::string::npos) {
        status = 400;
        return R"({"code":400,"msg":"invalid path"})";
    }
    int deviceId = std::stoi(idStr.substr(0, pos));

    std::cerr << "[Route] POST /api/camera/" << deviceId << "/photo" << std::endl;

    auto photo = CameraManager::Instance().TakePhoto(deviceId);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if (photo.IsEmpty()) {
        // 重试一次
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        photo = CameraManager::Instance().TakePhoto(deviceId);
    }
    if (photo.IsEmpty()) {
        status = 500;
        return R"({"code":500,"msg":"take photo failed or timeout"})";
    }

    std::string photoId = PhotoManager::Instance().SavePhoto(deviceId, photo);
    CTL::JSONObject json;
    json["photoId"]  = photoId;
    json["deviceId"] = deviceId;
    json["size"]     = static_cast<int>(photo.size());
    json["photo"]    = httplib::detail::base64_encode(
      std::string(photo.begin(), photo.end())
  );

    status = 200;
    return json.to_String();
}

// ============================================================
// 路由处理函数 — 推流控制
// ============================================================ map

std::string URL::HandleStreamControl(CTL::Request& request, int& status) {
    std::string path = request.RequestData.Path;
    bool start;
    std::string suffix;
    if (path.find("/stream/start") != std::string::npos) {
        start = true;
        suffix = "/stream/start";
    } else if (path.find("/stream/stop") != std::string::npos) {
        start  = false;
        suffix = "/stream/stop";
    } else {
        status = 400;
        start = false;
        return R"({"code":400,"msg":"invalid stream command"})";
    }

    std::string idStr = path.substr(12);              // 跳过 "/api/camera/"
    auto pos = idStr.find(suffix);
    int deviceId = std::stoi(idStr.substr(0, pos));

    std::cerr << "[Route] " << (start ? "START" : "STOP")
              << " stream for device " << deviceId << std::endl;

    if (start) {
        CameraManager::Instance().StartStream(deviceId);
        // 等第一帧到达（最多等 3 秒）
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (std::chrono::steady_clock::now() < deadline) {
            auto frame = CameraManager::Instance().GetFrame(deviceId);
            if (!frame.empty()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } else {
        CameraManager::Instance().StopStream(deviceId);
    }
    status = 200;
    bool start_ = true;
    std::string ip = "192.168.1.175";
    std::string port_ = "8081";
    return ip +"\n"+ port_ + "\r\n";
}

void URL::Onopen(CTL::WebSocketInfo &info) {
    while (info.GetConnectFlag()) {
        auto devices = CameraManager::Instance().OnlineDevices();
        if (devices.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        auto frame = CameraManager::Instance().GetFrame(devices[0]);
        if (frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (!info.SendBinary(frame)) {
            CTL::System::Println("发送失败");
        }
    }
}

int GetId(CTL::Request& request , std::string &_str) {
     std::string path = request.RequestData.Path;
    std::string idStr = path.substr(12);              // 跳过 "/api/camera/"
    auto pos = idStr.find(_str);
    if (pos == std::string::npos) {
        return -1;
    }
    int deviceId = std::stoi(idStr.substr(0, pos));
    return  deviceId;
}