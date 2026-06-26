#ifndef CC_MEMORY_POOL_H
#define CC_MEMORY_POOL_H

#include <cstddef>
#include <mutex>
#include <vector>

namespace CTL {
    static std::size_t align_up(std::size_t n, std::size_t a);

    template<typename T>
    class MemoryPool {
        struct Node {
            Node* next;
        };

        std::size_t block_size_t;
        std::size_t block_per_size_t;
        Node* free_list;
        std::vector<void*> pages;
        std::mutex mutex_t;
    public:
        MemoryPool();
        explicit MemoryPool(std::size_t b_per_size);
        ~MemoryPool();

        // 禁止拷贝
        MemoryPool(const MemoryPool&) = delete;
        MemoryPool& operator=(const MemoryPool&) = delete;

        T* allocate();
        void deallocate(T* p);
        std::size_t block_size() const;
        std::size_t block_per_size() const;

    private:
        void expand();
        static std::size_t adjust_block_size(std::size_t s);
    };

    // 通用内存池模板特化
    template<>
    class MemoryPool<void> {
        struct Node {
            Node* next;
        };

        std::size_t block_size_t;
        std::size_t block_per_size_t;
        Node* free_list;
        std::vector<void*> pages;
        std::mutex mutex_t;
    public:
        MemoryPool();
        MemoryPool(std::size_t b_size, std::size_t b_per_size);
        ~MemoryPool();

        // 禁止拷贝
        MemoryPool(const MemoryPool&) = delete;
        MemoryPool& operator=(const MemoryPool&) = delete;

        void* allocate();
        void deallocate(void* p);
        std::size_t block_size() const;
        std::size_t block_per_size() const;

    private:
        void expand();
        static std::size_t adjust_block_size(std::size_t s);
    };
}

#endif
