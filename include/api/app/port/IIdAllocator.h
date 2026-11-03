#pragma once
#include "api/common/Result.h"

namespace api::app {

// 设备 ID 分配器（端口接口，由 infrastructure 层实现）
class IIdAllocator {
public:
    virtual ~IIdAllocator() = default;

    virtual common::Result<int> Allocate() = 0;
    virtual void Release(int id) = 0;
};

} // namespace api::app
