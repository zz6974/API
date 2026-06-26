#ifndef CCVECTOR_H
#define CCVECTOR_H

#include <list>
#include "vector"
#include "mutex"
#include <algorithm>

template<typename T>
class CCVector:public std::vector<T>
{
public:
    // 默认构造函数
    CCVector() : std::vector<T>() {}

    // 继承 std::vector 的构造函数
    using std::vector<T>::vector;

    CCVector(const std::vector<T> & v) {
        this->assign(v.begin(), v.end());
    }
    CCVector(const std::list<T> & v) {
        this->assign(v.begin(), v.end());
    }
    CCVector(const CCVector<T> & v) {
        this->assign(v.begin(), v.end());
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
    /**
   * 有锁的添加方法
   * @param item 添加的值
   */
    void add_lock(const T& item) {
        mutex_.lock();
        std::vector<T>::push_back(item);
        mutex_.unlock();
    }
    /**
     * 有锁的删除方法
     * @param item 删除的值
     */
    void remove_lock(const T& item) {
        mutex_.lock();
        std::vector<T>::remove(item);
        mutex_.unlock();
    }
    /**
     * 有锁的清空方法
     */
    void clear_lock() {
        mutex_.lock();
        std::vector<T>::clear();
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
    CCVector operator=(const CCVector<T>& v) {
        this->assign(v.begin(), v.end());
        return *this;
    }
    CCVector operator=(const std::list<T>& v) {
        this->assign(v.begin(), v.end());
        return *this;
    }
    CCVector operator=(const std::vector<T>& v) {
        this->assign(v.begin(), v.end());
        return *this;
    }
    bool remove(const T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        auto ret = std::find(this->begin(), this->end(), item);
        if (ret != this->end()) {
            this->erase(ret);
            return true;
        }
        return false;
    }
    void removeAt(const int& index) {
        std::unique_lock<std::mutex> lock(mutex_);
        auto it = this->begin();
        std::advance(it, index);
        this->erase(it);
    }
    bool IsContains(const T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        return std::find(this->begin(), this->end(), item) != this->end();
    }
private:
    std::mutex mutex_;
};
// 将 operator<< 定义为非成员函数
template <typename T>
std::ostream& operator<<(std::ostream& os, const CCVector<T>& arr) {
    os << "[";
    for (size_t i = 0; i < arr.size(); ++i) {
        if (i != 0) {
            os << ", ";
        }
        os << arr[i];
    }
    os << "]";
    return os;
}

#endif
