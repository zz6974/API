#ifndef SDK_TL_LIST_H
#define SDK_TL_LIST_H

#include "list"
#include "mutex"
#include "iostream"
#include "vector"

namespace CTL{
    template<typename T>
    class List{
    private:
        std::mutex _mutex;
        std::list<T> _list;
    public:
        //--------------------------------------------------------------------------------------------------------------
        List() = default;
        explicit List(const std::list<T> list){
            _list = list;
        }
        explicit List(const std::vector<T> list){
            for (int i = 0; i < list.size(); ++i) {
                _list.push_back(list[i]);
            }
        }
        List(const List<T>& list) {
            auto L = list._list;
            for (auto O : L) {
                _list.push_back(O);
            }
        }
        List(const std::initializer_list<T>& list) {
            for (auto item : list) {
                _list.push_back(item);
            }
        }
        ~List() = default;
        //--------------------------------------------------------------------------------------------------------------
        friend std::ostream& operator<<(std::ostream& os, const List<T>& list) {
            os << "[";
            const auto& items = list._list;
            if (!items.empty()) {
                auto it = items.begin();
                os << *it;
                ++it;
                for (; it != items.end(); ++it) {
                    os << "," << *it;
                }
            }
            os << "]";
            return os;
        }
        //--------------------------------------------------------------------------------------------------------------
        void Add(const T &item){
            _mutex.lock();
            _list.push_back(item);
            _mutex.unlock();
        }
        bool Set(const int index, const T &item){
            _mutex.lock();
            if (index >= 0 && index < _list.size()) {
                auto it = _list.begin();
                std::advance(it, index);  // 将迭代器移动到指定索引
                *it = item;           // 修改该位置的值
                _mutex.unlock();
                return true;
            }
            else {
                _mutex.unlock();
                return false;
            }
        }
        T* Get(int index){
            _mutex.lock(); // 加锁，保证线程安全
            if (index >= 0 && index < _list.size()) { // 检查索引是否合法
                auto it = _list.begin();
                std::advance(it, index); // 将迭代器移动到指定索引
                _mutex.unlock(); // 解锁
                return &(*it); // 返回元素的指针（原代码返回 it 是错误的，应返回指向对象的指针）
            }
            else {
                _mutex.unlock(); // 解锁
                return nullptr; // 索引越界返回空指针
            }
        }
        bool Remove(int index){
            _mutex.lock();
            if (index >= 0 && index < _list.size()) {
                auto it = _list.begin();
                std::advance(it, index);
                _list.erase(it);
                _mutex.unlock();
                return true;
            }
            else {
                _mutex.unlock();
                return false;
            }
        }
        void RemoveAll(const T& item){
            _mutex.lock();
            _list.remove(item);
            _mutex.unlock();
        }
        void Clear(){
            _mutex.lock();
            _list.clear();
            _mutex.unlock();
        }
        int Size(){
            _mutex.lock();
            int size = _list.size();
            _mutex.unlock();
            return size;
        }
        bool IsEmpty(){
            _mutex.lock();
            bool isEmpty = _list.empty();
            _mutex.unlock();
            return isEmpty;
        }
        bool Insert(const int index, const T &item){
            _mutex.lock();
            if (index >= 0 && index <= _list.size()) {
                auto it = _list.begin();
                std::advance(it, index);
                _list.insert(it, item);
                _mutex.unlock();
                return true;
            }
            else {
                _mutex.unlock();
                return false;
            }
        }
        void Sort(){
            _mutex.lock();
            _list.sort();
            _mutex.unlock();
        }
        void Reverse(){
            _mutex.lock();
            _list.reverse();
            _mutex.unlock();
        }
        int find(const T& value) {
            const int count = _list.size();
            for (int i = 0; i < count; ++i) {
                if(Get(i) == &value){
                    return i;
                }
            }
            return -1; // 未找到
        }
        bool IsContains(const T& item){
            _mutex.lock();
            bool isContains = find(item) != -1;
            _mutex.unlock();
            return isContains;
        }
        std::list<T> ToList(){
            _mutex.lock();
            std::list<T> list = _list;
            _mutex.unlock();
            return list;
        }
        std::vector<T> ToVector(){
            _mutex.lock();
            std::vector<T> vector;
            for (auto& item : _list) {
                vector.push_back(item);
            }
            _mutex.unlock();
            return vector;
        }
        typename std::list<T>::iterator begin() {
            return _list.begin();
        }
        typename std::list<T>::iterator end() {
            return _list.end();
        }
        List<T> operator=(const List<T>& list){
            _mutex.lock();
            _list = list._list;
            _mutex.unlock();
            return list;
        }
        List<T> operator=(const std::list<T>& list){
            _mutex.lock();
            _list = list;
            _mutex.unlock();
            return *this;
        }
        //--------------------------------------------------------------------------------------------------------------
    private:
        //--------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------
    };
}


#endif
