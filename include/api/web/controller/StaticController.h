#ifndef API_STATICCONTROLLER_H
#define API_STATICCONTROLLER_H
#pragma once
#include <string>
#include "api/web/http/BaseController.h"
#include "api/web/http/Router.h"
namespace api::web {
    class StaticController {
    public:
        StaticController(std::string router) : Router(router) {};
        ~StaticController();
        void RegisterRoutes(api::web::Router& router);
    private:
        std::string Router;
    };
}


#endif //API_STATICCONTROLLER_H
