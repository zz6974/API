#ifndef CCLIST_H
#define CCLIST_H

#include <ostream>

#include "list"
#include "vector"

template<class T>
class CCList :public std::list<T> {
public:
    // 默认构造函数
    CCList() : std::list<T>() {}

    using std::list<T>::list;
    CCList(const std::list<T>& v) {
        this->assign(v.begin(), v.end());
    }
    CCList(const std::vector<T>& v) {
        this->assign(v.begin(), v.end());
    }
    CCList(const CCList<T>& v) {
        this->assign(v.begin(), v.end());
    }
    void add(const T& item) {
        this->push_back(item);
    }
    void append(const std::vector<T> & v) {
        for (auto & i : v) {
            this->push_back(i);
        }
    }
    void append(const std::list<T> & v) {
        for (auto & i : v) {
            this->push_back(i);
        }
    }
    friend std::ostream &operator<<(std::ostream &os, const CCList<T> &list) {
        os<< "[";
        for (auto it = list.begin(); it != list.end(); ++it) {
            os << *it;
            if (std::next(it) != list.end()) {
                os << ", ";
            }
        }
        os << "]";
        return os;
    }
    /**
    * 有锁的添加方法
    * @param item 添加的值
    */
    void add_lock(const T& item) {
        mutex_.lock();
        std::list<T>::emplace_back(item);
        mutex_.unlock();
    }
    /**
     * 有锁的删除方法
     * @param item 删除的值
     */
    void remove_lock(const T& item) {
        mutex_.lock();
        std::list<T>::remove(item);
        mutex_.unlock();
    }
    /**
     * 有锁的清空方法
     */
    void clear_lock() {
        mutex_.lock();
        std::list<T>::clear();
        mutex_.unlock();
    }
    /**
     * 获取方法
     * @return 获取队列索引元素
     */
    const T& get(const int index) {
        if (index < 0 || index >= static_cast<int>(this->size())) {
            throw std::out_of_range("Index out of range");
        }
        auto it = this->begin();
        std::advance(it, index);  // 使用 std::advance 移动迭代器
        return *it;
    }
    /**
     * 获取方法 多线程情况下可能会出现野指针
     * @return 获取队列索引元素的指针地址
    */
    T* getPtr(const int index) {
        if (index < 0 || index >= static_cast<int>(this->size())) {
            throw std::out_of_range("Index out of range");
        }
        auto it = this->begin();
        std::advance(it, index);  // 使用 std::advance 移动迭代器
        return &(*it);
    }
    const T& operator [] (const int index) {
        if (index < 0 || index >= static_cast<int>(this->size())) {
            throw std::out_of_range("Index out of range");
        }
        auto it = this->begin();
        std::advance(it, index);  // 使用 std::advance 移动迭代器
        return *it;
    }
    CCList operator=(const CCList<T>& v) {
        this->assign(v.begin(), v.end());
        return *this;
    }
    CCList operator=(const std::list<T>& v) {
        this->assign(v.begin(), v.end());
        return *this;
    }
    CCList operator=(const std::vector<T>& v) {
        this->assign(v.begin(), v.end());
        return *this;
    }
private:
    std::mutex mutex_;
};





#endif
