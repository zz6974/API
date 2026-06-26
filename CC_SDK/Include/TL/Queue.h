#ifndef SDK_TL_QUEUE_H
#define SDK_TL_QUEUE_H

#include "mutex"
#include "AutoDestruct.h"
#include "deque"

namespace CTL{
    template<typename T>
    class Queue{
    private:
        //--------------------------------------------------------------------------------------------------------------
        std::mutex _mutex;
        std::deque<T> _deque;
        //--------------------------------------------------------------------------------------------------------------
    public:
        //--------------------------------------------------------------------------------------------------------------
        Queue() = default;
        ~Queue() = default;
        //--------------------------------------------------------------------------------------------------------------
        void Add(const T& data){
            _mutex.lock();
            _deque.push_back(std::move(data));
            _mutex.unlock();
        }
        void AddFirst(const T& data){
            _mutex.lock();
            _deque.push_front(std::move(data));
            _mutex.unlock();
        }
        AutoDestruct<T> Poll(){
            _mutex.lock();
            if(_deque.empty()){
                _mutex.unlock();
                return nullptr;
            }
            T* data = new T(_deque.front());
            _deque.pop_front();
            _mutex.unlock();
            return data;
        }
        AutoDestruct<T> PollBack(){
            _mutex.lock();
            if(_deque.empty()){
                _mutex.unlock();
                return nullptr;
            }
            T* data = new T(_deque.back());
            _deque.pop_back();
            _mutex.unlock();
            return data;
        }
        bool IsEmpty(){
            _mutex.lock();
            bool empty = _deque.empty();
            _mutex.unlock();
            return empty;
        }
        int Size(){
            _mutex.lock();
            int size = _deque.size();
            _mutex.unlock();
            return size;
        }
        //--------------------------------------------------------------------------------------------------------------
    };
}

#endif
