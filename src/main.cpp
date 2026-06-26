#include <iostream>
#include <thread>

#include "../include/Analysis.h"
#include "../include/CameraManager.h"
#include "../include/HeadBeat.h"
#include "CCServlet/CCWebServlet.h"

#define Address "192.168.1.175"
#define Port 8082

int main() {
    system("chcp 65001");

    Headbeat heartbeat;
    heartbeat.Start();//8083

    Analysis analysis;
    analysis.Init();

    CameraManager::Instance().Start(8083);

    CTL::WebServlet server;
    server.Init(Address, Port);

    for (const auto& [path, _] : analysis.GetRoutes()) {
        server.SetWebServlet(path, [&analysis](CTL::Request& req, CTL::Response& res) {
            int status = 200;
            std::string body = analysis.HandleRequest(req, status);
            res.SetHeader("Content-Type",   "application/json; charset=utf-8");
            res.SetHeader("Content-Length", std::to_string(body.size()));
            res.Write(body, status);
        });
    }

    server.SetWebServletFun([&analysis](CTL::Request& req, CTL::Response& res) {
        int status = 200;
        std::string body = analysis.HandleRequest(req, status);
        res.SetHeader("Content-Type",   "application/json; charset=utf-8");
        res.SetHeader("Content-Length", std::to_string(body.size()));
        res.Write(body, status);
    });

    std::cerr << "[Thread " << std::this_thread::get_id()
              << "] [Server] 启动在 " << Address << ":" << Port << std::endl;
    server.Running();
    return 0;
}
