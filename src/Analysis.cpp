#include <iostream>
#include <thread>

#include "CCServlet/CCRequest.h"
#include "../include/Analysis.h"
#include "../include/URL.h"
#include "../include/CameraManager.h"

// ============================================================
// 生命周期
// ============================================================

Analysis::Analysis() : url(nullptr) {}

Analysis::~Analysis() {
    delete url;
}

void Analysis::Init() {
    if (!url) {
        url = new URL;
    }
    url->Init();
}

// ============================================================
// 请求处理入口
// ============================================================

std::string Analysis::HandleRequest(CTL::Request& req, int& status) {
    std::string Method = req.RequestData.Method;
    std::string result;

    if (Method == "GET") {
        result = HandleGet(req, status);
        std::cout << "[Thread " << std::this_thread::get_id()
              << "] [GET] " << req.RequestData.Body << std::endl;
    }else if (Method == "POST") {
        result = HandlePost(req, status);
        std::cerr << "[Thread " << std::this_thread::get_id()
              << "] [POST] " << req.RequestData.Body << std::endl;
    }else {
        status = 405;
        return R"({"code":405,"msg":"不支持的请求方法"})";
    }
    if ((status != 404)) {
        return result;
    }
    // 精确匹配：摄像头列表
    std::string path = req.RequestData.Path;
    std::string suffix;
    int deviceId = GetDeviceId(path, suffix);
    if (path == "/api/camera/list") {
        return URL::HandleCameraList(req, status);
    }
    // 带 deviceId 的路径：提取 ID 和 suffix，按 suffix 分发
    std::cout << "[Thread " << std::this_thread::get_id()
              << "] [deviceId] " << deviceId << std::endl;
 // if (suffix == "/frame") {
    //     std::cerr << "[frame] : 进来了" << std::endl;
    //
    //     return URL::HandleCameraFrame(req, status);
    // }

    if (suffix == "/photo") {
        std::cerr << "[photo] : 进来了" << std::endl;
        return URL::HandleTakePhoto(req, status);
    }
    if (suffix == "/stream/start") {
        std::cerr << "[start] : 进来了" << std::endl;
        return url -> HandleStreamControl(req, status);
    }
    if (suffix == "/stream/stop") {
        std::cerr << "[stop] : 进来了" << std::endl;
        return url -> HandleStreamControl(req, status);
    }

    status = 404;
    return R"({"code":404,"msg":"未知摄像头操作"})";
}

// ============================================================
// GET / POST 分发（精确路由走 URL::Dispatch）
// ============================================================

std::string Analysis::HandleGet(CTL::Request& req, int& status) {
    std::string result;
    if (url && url->Dispatch(req.RequestData.Path, req, result, status)) {
        return result;
    }
    status = 404;
    return R"({"code":404,"msg":"未找到路由"})";
}

std::string Analysis::HandlePost(CTL::Request& req, int& status) {
    std::string result;
    if (url && url->Dispatch(req.RequestData.Path, req, result, status)) {
        return result;
    }
    status = 404;
    return R"({"code":404,"msg":"未找到路由"})";
}

// ============================================================
// 辅助函数
// ============================================================

CTL::JSONObject Analysis::httpParseBody(CTL::Request& req, int& status) {
    if (req.RequestData.Body.IsEmpty()) {
        status = 400;
        return CTL::JSONObject();
    }
    std::cerr << "[Thread " << std::this_thread::get_id()
              << "] [JSON] 原始 Body：" << req.RequestData.Body << std::endl;
    return req.GetJson();
}

std::map<std::string, RoutHandler> Analysis::GetRoutes() {
    if (url) {
        return url->Getroutes();
    }
    return {};
}

int Analysis::GetDeviceId(std::string path, std::string& suffix) {
    std::string rest = path.substr(12);
    size_t slashPos = rest.find('/');
    if (slashPos == std::string::npos) {
        // 无后缀: "/api/camera/0"
        suffix = "";
        try { return std::stoi(rest); }
        catch (...) { return -1; }
    }
    // 有后缀: "/api/camera/0/photo"
    suffix = rest.substr(slashPos);
    try { return std::stoi(rest.substr(0, slashPos)); }
    catch (...) { return -1; }
}
