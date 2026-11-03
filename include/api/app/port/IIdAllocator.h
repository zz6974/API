#pragma once
#include "api/common/Result.h"

namespace api::app {

/**
 * @brief 设备 ID 分配器
 *
 * 端口接口，由 infrastructure 层实现。
 */
class IIdAllocator {
public:
    virtual ~IIdAllocator() = default;

    /**
     * @brief 分配一个空闲 ID
     * @return 新 ID；池满返回 kNoIdAvailable
     */
    virtual common::Result<int> Allocate() = 0;
    /**
     * @brief 归还 ID（设备下线时调用）
     * @param id 要归还的 ID
     */
    virtual void Release(int id) = 0;
};

} // namespace api::app
