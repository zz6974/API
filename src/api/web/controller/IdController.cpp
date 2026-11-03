#include "api/web/controller/IdController.h"
#include <utility>

namespace api::web {

IdController::IdController(std::shared_ptr<app::IIdAllocator> allocator)
    : allocator_(std::move(allocator)) {
}

void IdController::RegisterRoutes(Router& router) {
    router.Add("GET", "/api/GetID", [this](auto& req, auto& res, auto& p) { HandleGetId(req, res, p); });
}

void IdController::HandleGetId(CTL::Request& req, CTL::Response& res, const PathParams&) {
    auto result = allocator_->Allocate();
    if (result.IsFail()) {
        WriteError(res, ToHttpStatus(result.Code()), result.Message());
        return;
    }
    int id = result.Value();
    // 键为 "camN"（如 cam3），值为数字 N，兼容终端注册时回填的 id 字段
    WriteJson(res, {{std::string("cam") + std::to_string(id), id}});
}

} // namespace api::web
