#ifndef API_ANALYSIS_H
#define API_ANALYSIS_H

#include "CCServlet/CCRequest.h"
#include "URL.h"

class Analysis {
public:
    // ===== 生命周期 =====
    Analysis();
    ~Analysis();
    void Init();

    // ===== 请求处理 =====
    std::string HandleRequest(CTL::Request& req, int& status);
    std::string HandleGet(CTL::Request& req, int& status);
    std::string HandlePost(CTL::Request& req, int& status);

    // ===== 路由查询 =====
    std::map<std::string, RoutHandler> GetRoutes();

    // ===== 辅助 =====
    CTL::JSONObject httpParseBody(CTL::Request& req, int& status);
    static int GetDeviceId(std::string path, std::string& suffix);

private:
    URL* url;
};

#endif
