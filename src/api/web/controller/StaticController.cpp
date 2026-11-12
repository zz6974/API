#include "../../../../include/api/web/controller/StaticController.h"

#include <fstream>

#include "CCResponse.h"
#include "api/common/Logger.h"

api::web::StaticController::~StaticController() {

}

void api::web::StaticController::RegisterRoutes(api::web::Router &router) {
    router.Add("GET" , "/" , [this](auto &req , auto &res ,auto&)
        { ServeFile(res , "index.html" , "text/html; charset=utf-8"); });
    router.Add("GET" , "/index.html" , [this](auto &req , auto &res ,auto&)
        { ServeFile(res , "index.html" , "text/html; charset=utf-8"); });

    router.Add("GET" , "/style.css" , [this](auto &req , auto &res ,auto&)
        { ServeFile(res , "style.css" , "text/css; charset=utf-8"); });
    router.Add("GET" , "/script.js" , [this](auto &req , auto &res ,auto&)
        { ServeFile(res , "script.js" , "text/javascript; charset=utf-8"); });
}

void api::web::StaticController::ServeFile(CTL::Response &res, const std::string &fileName,
    const std::string &contentType) {
    std::ifstream file(Router + "/" + fileName, std::ios::binary);
    if (!file) {
        LOG_ERROR("[Controller] File not found: '静态文件不存在'");
    }
    std::string content((std::istream_iterator<char>(file)), std::istream_iterator<char>());
    res.SetHeader("Content-type" , contentType);
    res.Write(content , 200);
}
