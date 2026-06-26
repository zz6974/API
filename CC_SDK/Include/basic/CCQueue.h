#ifndef CCQUEUE_H
#define CCQUEUE_H
#include "queue"
#include "mutex"
template<typename T>
class CCQueue :public std::queue<T> {
public:
    using std::queue<T>::queue;
    /**
     * 有锁的添加方法
     * @param item 添加的值
     */
    void add_lock(const T& item) {
        mutex_.lock();
        std::queue<T>::push(item);
        mutex_.unlock();
    }
    /**
     * 有锁的删除方法
     * @param item 删除的值
     */
    void remove_lock(const T& item) {
        mutex_.lock();
        std::queue<T>::pop(item);
        mutex_.unlock();
    }
    /**
     * 有锁的清空方法
     */
    void clear_lock() {
        mutex_.lock();
        std::queue<T>::clear();
        mutex_.unlock();
    }
    /**
     * 有锁的获取方法
     * @return 取出队列第一个元素
     */
    T poll_lock() {
        mutex_.lock();
        T item = std::queue<T>::front();
        std::queue<T>::pop();
        mutex_.unlock();
        return item;
    }
    /**
     * 有锁的判断非空
     * @return 是否为空
    */
    bool IsEmpty() {
        mutex_.lock();
        const bool is_empty = std::queue<T>::empty();
        mutex_.unlock();
        return is_empty;
    }
protected:
    std::mutex mutex_;
};

#endif
