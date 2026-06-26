#include "basic/CCMemoryPool.h"
#include <stdexcept>

#include "CCAutoLock.h"

std::size_t CTL::align_up(const std::size_t n, const std::size_t a) {
    return (n + (a - 1)) & ~(a - 1);
}

// 模板类 MemoryPool<T> 的实现
template<typename T>
CTL::MemoryPool<T>::MemoryPool()
    : block_size_t(adjust_block_size(sizeof(T))),
      block_per_size_t(1024),
      free_list(nullptr) {
}

template<typename T>
CTL::MemoryPool<T>::MemoryPool(const std::size_t b_per_size)
    : block_size_t(adjust_block_size(sizeof(T))),
      block_per_size_t(b_per_size),
      free_list(nullptr) {
    if (b_per_size == 0) {
        throw std::invalid_argument("Blocks per size must be positive");
    }
}

template<typename T>
CTL::MemoryPool<T>::~MemoryPool() {
    CCUniqueLock lock(mutex_t);
    for (void* page : pages) {
        ::operator delete [](page);
    }
}

template<typename T>
T* CTL::MemoryPool<T>::allocate() {
    CCUniqueLock lock(mutex_t);
    if (!free_list) {
        expand();
    }
    Node* head = free_list;
    free_list = head->next;
    return static_cast<T*>(static_cast<void*>(head));
}

template<typename T>
void CTL::MemoryPool<T>::deallocate(T* p) {
    CCUniqueLock lock(mutex_t);
    if (!p) {
        return;
    }
    const auto node = static_cast<Node*>(static_cast<void*>(p));
    node->next = free_list;
    free_list = node;
}

template<typename T>
std::size_t CTL::MemoryPool<T>::block_size() const {
    return block_size_t;
}

template<typename T>
std::size_t CTL::MemoryPool<T>::block_per_size() const {
    return block_per_size_t;
}

template<typename T>
void CTL::MemoryPool<T>::expand() {
    const std::size_t page_size = block_size_t * block_per_size_t;
    const auto page = static_cast<char*>(::operator new [](page_size));
    pages.push_back(page);
    for (std::size_t i = 0; i < block_per_size_t; ++i){
        const auto addr = page + i * block_size_t;
        const auto n = reinterpret_cast<Node*>(addr);
        n->next = free_list;
        free_list = n;
    }
}

template<typename T>
std::size_t CTL::MemoryPool<T>::adjust_block_size(const std::size_t s) {
    constexpr std::size_t min = sizeof(void*);
    const std::size_t a = align_up(s < min ? min : s, alignof(void*));
    return a;
}

// 特化版本 MemoryPool<void> 的实现
CTL::MemoryPool<void>::MemoryPool()
    : block_size_t(adjust_block_size(sizeof(void*))),
      block_per_size_t(1024),
      free_list(nullptr) {
}

CTL::MemoryPool<void>::MemoryPool(const std::size_t b_size, const std::size_t b_per_size)
    : block_size_t(adjust_block_size(b_size)),
      block_per_size_t(b_per_size),
      free_list(nullptr) {
    if (b_size == 0 || b_per_size == 0) {
        throw std::invalid_argument("Block size and blocks per size must be positive");
    }
}

CTL::MemoryPool<void>::~MemoryPool() {
    CCUniqueLock lock(mutex_t);
    for (void* page : pages) {
        ::operator delete [](page);
    }
}

void* CTL::MemoryPool<void>::allocate() {
    CCUniqueLock lock(mutex_t);
    if (!free_list) {
        expand();
    }
    Node* head = free_list;
    free_list = head->next;
    return head;
}

void CTL::MemoryPool<void>::deallocate(void *p) {
    CCUniqueLock lock(mutex_t);
    if (!p) {
        return;
    }
    const auto node = static_cast<Node*>(p);
    node->next = free_list;
    free_list = node;
}

std::size_t CTL::MemoryPool<void>::block_size() const {
    return block_size_t;
}

std::size_t CTL::MemoryPool<void>::block_per_size() const {
    return block_per_size_t;
}

void CTL::MemoryPool<void>::expand() {
    const std::size_t page_size = block_size_t * block_per_size_t;
    const auto page = static_cast<char*>(::operator new [](page_size));
    pages.push_back(page);
    for (std::size_t i = 0; i < block_per_size_t; ++i){
        const auto addr = page + i * block_size_t;
        const auto n = reinterpret_cast<Node*>(addr);
        n->next = free_list;
        free_list = n;
    }
}

std::size_t CTL::MemoryPool<void>::adjust_block_size(const std::size_t s) {
    constexpr std::size_t min = sizeof(void*);
    const std::size_t a = align_up(s < min ? min : s, alignof(void*));
    return a;
}

// 显式实例化常用的类型
template class CTL::MemoryPool<int>;
template class CTL::MemoryPool<double>;
template class CTL::MemoryPool<char>;
