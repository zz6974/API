#ifndef CCMAP_H
#define CCMAP_H

#include <mutex>
#include "CCArrayList.h"
#include "CCList.h"
#include "CCVector.h"
#include "map"

template<typename K,typename V>
class CCMap:public std::map<K,V> {
    std::mutex mutex_s;
public:
    // 默认构造函数
    CCMap() : std::map<K,V>() {}

    // 继承 std::map 的构造函数
    using std::map<K,V>::map;

    CCMap(std::map<K,V> map) {
        this->insert(map.begin(),map.end());
    }
    CCMap(const std::map<K,V>& map) {
        this->insert(map.begin(),map.end());
    }
    CCMap(const CCMap<K,V>& map) {
        this->insert(map.begin(),map.end());
    }
    CCList<V> values() {
        CCList<V> list;
        for(auto it=this->begin();it!=this->end();++it) {
            list.add(it->second);
        }
        return list;
    }
    CCVector<V> valuesVector() {
        CCVector<V> vector;
        for(auto it=this->begin();it!=this->end();++it) {
            vector.add(it->second);
        }
        return vector;
    }
    CTL::ArrayList<V> valuesArrayList() {
        CTL::ArrayList<V> arrayList;
        for(auto it=this->begin();it!=this->end();++it) {
            arrayList.add(it->second);
        }
        return arrayList;
    }
    /**
     * 有锁的添加键值对
     * @param key 键
     * @param value 值
     */
    void put_lock(K key,V value) {
        mutex_s.lock();
        this->insert(std::pair<K,V>(key,value));
        mutex_s.unlock();
    }
    /**
     * 有锁的修改键值对
     * @param key 键
     * @param value 值
     */
    void replace_lock(K key,V value) {
        mutex_s.lock();
        this->at(key) = value;
        mutex_s.unlock();
    }
    /**
     * 有锁的删除键值对
     * @param key 键
     */
    void remove_lock(K key) {
        mutex_s.lock();
        this->erase(key);
        mutex_s.unlock();
    }
    /**
     * @param key 键
     * @return 值
     */
    V get(K key) {
        std::unique_lock<std::mutex> lock(mutex_s);
        return this->at(key);
    }
    bool IsEmpty() {
        std::unique_lock<std::mutex> lock(mutex_s);
        return this->empty();
    }
    operator std::map<K,V>() {
        return *this;
    }
    CCMap operator = (const std::map<K,V>& map) {
        this->clear();
        this->insert(map.begin(),map.end());
        return *this;
    }
    CCMap operator = (const CCMap<K,V>& map) {
        this->clear();
        this->insert(map.begin(),map.end());
        return *this;
    }
    CCMap operator = (const CCMap<K,V>* map) {
        this->clear();
        this->insert(map->begin(),map->end());
        return *this;
    }
    bool exist(K key) {
        std::unique_lock<std::mutex> lock(mutex_s);
        return this->find(key) != this->end();
    }
};

#endif
