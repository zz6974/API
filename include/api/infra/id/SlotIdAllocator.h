#pragma once
#include <mutex>
#include <vector>
#include "api/app/port/IIdAllocator.h"

namespace api::infra {

/**
 * @brief 槽位式 ID 分配器（由旧 ID.cpp 平移改造）
 *
 * 去掉全局静态 ID_t 与全局函数，改为实例状态，实现 app::IIdAllocator 端口。
 */
class SlotIdAllocator : public app::IIdAllocator {
public:
    /**
     * @brief 构造
     * @param capacity 槽位总数（即最大同时在线设备数）
     */
    explicit SlotIdAllocator(int capacity = 20);
    ~SlotIdAllocator() override = default;

    /**
     * @brief 分配 ID
     * @return 第一个空闲槽位的下标；全满回 kNoIdAvailable
     */
    common::Result<int> Allocate() override;
    /**
     * @brief 归还 ID
     * @param id 要归还的 ID，越界静默忽略
     */
    void Release(int id) override;

    /**
     * @brief 查询槽位占用状态
     * @param id 设备 ID
     * @return 是否已占用
     */
    bool IsAllocated(int id) const;
    /**
     * @brief 槽位占用快照
     * @return 每个槽位的占用状态
     */
    std::vector<bool> Snapshot() const;

private:
    std::vector<bool> slots_;  ///< 每个下标对应一个设备号，true=已占用
    mutable std::mutex mutex_; ///< 保护槽位表
};

} // namespace api::infra
