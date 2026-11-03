#pragma once
#include <memory>
#include "api/app/port/IIdAllocator.h"
#include "api/web/http/BaseController.h"
#include "api/web/http/Router.h"

namespace api::web {

// 设备 ID 控制器：路由 /api/GetID
class IdController : public BaseController {
public:
    explicit IdController(std::shared_ptr<app::IIdAllocator> allocator);

    void RegisterRoutes(Router& router);

private:
    void HandleGetId(CTL::Request& req, CTL::Response& res, const PathParams& params);

    std::shared_ptr<app::IIdAllocator> allocator_;
};

} // namespace api::web
