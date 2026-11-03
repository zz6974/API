#pragma once
#include <memory>
#include "api/app/port/IIdAllocator.h"
#include "api/web/http/BaseController.h"
#include "api/web/http/Router.h"

namespace api::web {

/**
 * @brief 设备 ID 控制器
 *
 * 路由 /api/GetID。
 */
class IdController : public BaseController {
public:
    /**
     * @brief 构造
     * @param allocator ID 分配器（端口接口）
     */
    explicit IdController(std::shared_ptr<app::IIdAllocator> allocator);

    /**
     * @brief 注册路由
     * @param router 路由表
     */
    void RegisterRoutes(Router& router); // 注册 1 条路由：取设备 ID

private:
    void HandleGetId(CTL::Request& req, CTL::Response& res, const PathParams& params); ///< 从 ID 池取一个空闲号并占用

    std::shared_ptr<app::IIdAllocator> allocator_; ///< 直接注入端口接口（无需用例层中转）
};

} // namespace api::web
