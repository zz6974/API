#pragma once
#include <mutex>
#include <vector>
#include "api/app/port/IIdAllocator.h"

namespace api::infra {

// 槽位式 ID 分配器（由旧 ID.cpp 平移改造）：
// 去掉全局静态 ID_t 与全局函数，改为实例状态，实现 app::IIdAllocator 端口
class SlotIdAllocator : public app::IIdAllocator {
public:
    explicit SlotIdAllocator(int capacity = 20);
    ~SlotIdAllocator() override = default;

    common::Result<int> Allocate() override;
    void Release(int id) override;

    bool IsAllocated(int id) const;
    std::vector<bool> Snapshot() const;

private:
    std::vector<bool> slots_;
    mutable std::mutex mutex_;
};

} // namespace api::infra
