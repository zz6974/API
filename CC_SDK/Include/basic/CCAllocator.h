#ifndef CC_ALLOCATOR_H
#define CC_ALLOCATOR_H
#include <cstddef>
#include <new>
#include <iostream>

namespace CTL {
    template <typename T>
    class Allocator {
    public:
        using value_type = T;

        // 构造函数
        Allocator() noexcept = default;

        // 复制构造函数
        template <typename U>
        explicit Allocator(const Allocator<U>&) noexcept {}

        // 分配内存
        T* allocate(std::size_t n) {
            if (n > std::size_t(-1) / sizeof(T)) {
                throw std::bad_alloc();
            }
            if (auto p = static_cast<T*>(std::malloc(n * sizeof(T)))) {
                return p;
            }
            throw std::bad_alloc();
        }

        // 释放内存
        void deallocate(T* p, std::size_t) noexcept {
            std::free(p);
        }

        // 返回最大可分配的元素数量
        std::size_t max_size() const noexcept {
            return std::size_t(-1) / sizeof(T);
        }
    };

    // 特化 std::allocator_traits
    template <typename T, typename U>
    bool operator==(const Allocator<T>&, const Allocator<U>&) { return true; }

    template <typename T, typename U>
    bool operator!=(const Allocator<T>&, const Allocator<U>&) { return false; }
}
#endif
