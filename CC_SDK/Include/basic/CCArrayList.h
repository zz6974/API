#ifndef CC_ArrayList_H
#define CC_ArrayList_H
#include <stdexcept>
#include <memory> // for std::allocator
#include "CCAllocator.h"

namespace CTL{
    template<typename T, class Alloc = Allocator<T>>
    class ArrayList {
        //-------------------------------------------------------------------------
        // 类型定义
        typedef typename std::allocator_traits<Alloc>::template rebind_alloc<T> Tp_alloc_type;
        typedef typename std::allocator_traits<Tp_alloc_type>::pointer pointer;
        typedef pointer iterator;
        typedef const pointer const_iterator;
        //-------------------------------------------------------------------------
        // 成员变量
        Tp_alloc_type allocator_; // 引入 allocator 成员变量
        iterator _start = nullptr;
        iterator _finish = nullptr;
        iterator _end_of_storage = nullptr;
        //-------------------------------------------------------------------------
        // reserve函数，用于增加数组容量
        void reserve(size_t new_capacity) {
            if(new_capacity > capacity()) {
                size_t old_size = size();
                pointer new_start = allocator_.allocate(new_capacity); // 使用 allocator 分配内存
                try {
                    for (size_t i = 0; i < old_size; i++) {
                        std::allocator_traits<Tp_alloc_type>::construct(allocator_, &new_start[i], _start[i]); // 使用 allocator 构造对象
                    }
                }
                catch (...) {
                    for (size_t i = 0; i < old_size; i++) {
                        std::allocator_traits<Tp_alloc_type>::destroy(allocator_, &new_start[i]); // 如果构造失败，销毁已构造的对象
                    }
                    allocator_.deallocate(new_start, new_capacity); // 释放内存
                    throw;
                }
                for (size_t i = 0; i < old_size; i++) {
                    std::allocator_traits<Tp_alloc_type>::destroy(allocator_, &_start[i]); // 销毁旧的对象
                }
                allocator_.deallocate(_start, capacity()); // 释放旧的内存
                _start = new_start;
                _finish = _start + old_size;
                _end_of_storage = _start + new_capacity;
            }
        }
    public:
        //-------------------------------------------------------------------------
        // 默认构造函数
        ArrayList() = default;
        // 拷贝构造函数
        ArrayList(const ArrayList<T>& list) {
            reserve(list.capacity());
            for (auto i : list) {
                add(i);
            }
        }
        // 初始化列表构造函数
        ArrayList(std::initializer_list<T> list) {
            reserve(list.size());
            for (auto i : list) {
                push_back(i);
            }
        }
        // 迭代器构造函数
        ArrayList(iterator first, iterator last) {
            assign(first, last);
        }
        // 析构函数
        ~ArrayList() {
            for (size_t i = 0; i < size(); i++) {
                std::allocator_traits<Tp_alloc_type>::destroy(allocator_, &_start[i]); // 销毁对象
            }
            allocator_.deallocate(_start, capacity()); // 释放内存
            _start = _finish = _end_of_storage = nullptr;
        }
        //-------------------------------------------------------------------------
        // 迭代器相关函数
        iterator begin() {
            return _start;
        }
        iterator end() {
            return _finish;
        }
        const_iterator begin() const {
            return _start;
        }
        const_iterator end() const {
            return _finish;
        }
        // 返回数组大小
        [[nodiscard]] size_t size() const {
            return _finish - _start;
        }
        // 返回数组容量
        [[nodiscard]] size_t capacity() const {
            return _end_of_storage - _start;
        }
        // 判断数组是否为空
        [[nodiscard]] bool isEmpty() const {
            return _start == _finish;
        }
        // 调整数组大小
        void resize(size_t new_size, const T& value = T()) {
            if(new_size > size()) {
                reserve(new_size);
                while (_finish < _start + new_size) {
                    std::allocator_traits<Tp_alloc_type>::construct(allocator_, _finish, value); // 使用 allocator 构造对象
                    ++_finish;
                }
            }
            else {
                while (_finish > _start + new_size) {
                    --_finish;
                    std::allocator_traits<Tp_alloc_type>::destroy(allocator_, _finish); // 使用 allocator 销毁对象
                }
            }
        }
        // 插入元素
        void insert(iterator position, const T& value) {
            if(_finish == _end_of_storage) {
                size_t len = position - _start;
                reserve(capacity() == 0 ? 4 : capacity() * 2);
                position = _start + len;
            }
            //插入数据
            iterator it = _finish - 1;
            while (position <= it)
            {
                std::allocator_traits<Tp_alloc_type>::construct(allocator_, it + 1, *it); // 使用 allocator 构造对象
                std::allocator_traits<Tp_alloc_type>::destroy(allocator_, it); // 使用 allocator 销毁对象
                --it;
            }
            std::allocator_traits<Tp_alloc_type>::construct(allocator_, position, value); // 使用 allocator 构造对象
            ++_finish;
        }
        // 删除元素
        iterator remove(iterator position) {
            iterator it = position;
            while (it < _finish - 1)
            {
                std::allocator_traits<Tp_alloc_type>::construct(allocator_, it, *(it + 1)); // 使用 allocator 构造对象
                std::allocator_traits<Tp_alloc_type>::destroy(allocator_, it + 1); // 使用 allocator 销毁对象
                ++it;
            }
            --_finish;
            std::allocator_traits<Tp_alloc_type>::destroy(allocator_, _finish); // 使用 allocator 销毁对象
            return position;
        }
        // 添加元素
        void add(const T& value) {
            insert(end(),value);
        }
        // 移除最后一个元素
        void pop_back() {
            remove(end() - 1);
        }
        // 赋值
        void assign(iterator first, iterator last) {
            reserve(last - first);
            _finish = _start + (last - first);
            for (size_t i = 0; i < last - first; i++) {
                std::allocator_traits<Tp_alloc_type>::construct(allocator_, &_start[i], first[i]); // 使用 allocator 构造对象
            }
        }
        //-------------------------------------------------------------------------
        // 下标运算符重载
        T& operator[](size_t index){
            if(index < size()) {
                return _start[index];
            }
            else {
                throw std::out_of_range("Index out of range");
            }
        }
        const T& operator[](size_t index) const {
            if(index < size()) {
                return _start[index];
            }
            else {
                throw std::out_of_range("Index out of range");
            }
        }
        // 赋值运算符重载
        ArrayList<T>& operator = (const ArrayList<T> list) {
            if (this != &list) {
                for (size_t i = 0; i < size(); i++) {
                    std::allocator_traits<Tp_alloc_type>::destroy(allocator_, &_start[i]); // 销毁对象
                }
                allocator_.deallocate(_start, capacity()); // 释放内存
                reserve(list.capacity());
                for (auto i : list) {
                    push_back(i);
                }
            }
            return *this;
        }
        //-------------------------------------------------------------------------
    };
}
#endif

