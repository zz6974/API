#include "api/web/controller/AuthController.h"
#include <utility>

namespace api::web {

AuthController::AuthController(std::shared_ptr<app::AuthService> service)
    : service_(std::move(service)) {
}

void AuthController::RegisterRoutes(Router& router) {
    router.Add("POST", "/api/login", [this](auto& req, auto& res, auto& p) { HandleLogin(req, res, p); });
}

void AuthController::HandleLogin(CTL::Request& req, CTL::Response& res, const PathParams&) {
    auto body = ParseJsonBody(req);
    if (body.IsFail()) {
        WriteError(res, ToHttpStatus(body.Code()), body.Message());
        return;
    }
    std::string account = body.Value().value("Account", "");
    std::string password = body.Value().value("Password", "");

    auto result = service_->Authenticate(account, password);
    if (result.IsFail()) {
        WriteError(res, ToHttpStatus(result.Code()), result.Message());
        return;
    }
    WriteJson(res, {{"status", "success"}});
}

} // namespace api::web
