#include "api/infra/id/SlotIdAllocator.h"

namespace api::infra {

SlotIdAllocator::SlotIdAllocator(int capacity)
    : slots_(static_cast<size_t>(capacity > 0 ? capacity : 20), false) {
}

common::Result<int> SlotIdAllocator::Allocate() {
    std::lock_guard<std::mutex> lock(mutex_);
    // 线性扫第一个空槽（简单策略，容量小无需优化）
    for (size_t i = 0; i < slots_.size(); i++) {
        if (!slots_[i]) {
            slots_[i] = true;
            return common::Result<int>::Ok(static_cast<int>(i));
        }
    }
    return common::Result<int>::Fail(common::ErrorCode::kNoIdAvailable, "ID 已分配完");
}

void SlotIdAllocator::Release(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (id >= 0 && static_cast<size_t>(id) < slots_.size()) { // 越界防御
        slots_[static_cast<size_t>(id)] = false;
    }
}

bool SlotIdAllocator::IsAllocated(int id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (id >= 0 && static_cast<size_t>(id) < slots_.size()) {
        return slots_[static_cast<size_t>(id)];
    }
    return false;
}

std::vector<bool> SlotIdAllocator::Snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return slots_;
}

} // namespace api::infra
