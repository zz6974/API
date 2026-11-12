#ifndef API_STATICCONTROLLER_H
#define API_STATICCONTROLLER_H
#pragma once
#include <string>
#include "api/web/http/BaseController.h"
#include "api/web/http/Router.h"
namespace api::web {
    class StaticController {
    public:
        StaticController(std::string router) : Router(std::move(router)) {};
        ~StaticController();
        void RegisterRoutes(api::web::Router& router);
    private:
        void ServeFile(CTL::Response &res , const std::string &fileName , const std::string &contentType);
        std::string Router;
    };
}


#endif //API_STATICCONTROLLER_H
