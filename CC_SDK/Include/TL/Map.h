#ifndef SDK_TL_MAP_H
#define SDK_TL_MAP_H

#include "iostream"
#include "vector"
#include "list"
#include "map"
#include "unordered_map"
#include "mutex"
#include "string"

namespace CTL{
    template<typename K, typename V>
    class Map{
    private:
        //--------------------------------------------------------------------------------------------------------------
        mutable std::mutex _mutex;
        std::map<K, V> _map;
        //--------------------------------------------------------------------------------------------------------------
    public:
        //--------------------------------------------------------------------------------------------------------------
        friend std::ostream& operator<<(std::ostream& os,Map<K,V>& map) {
            os << "[\n";
            auto items = map.ToStdMap();
            auto it = items.begin();
            while (it != items.end()) {
                os << "    {\n         " << it->first << ": " << it->second << "\n    }";
                ++it;
                if (it != items.end()) {
                    os << ",\n";
                }
            }
            os << "\n]";
            return os;
        }
        friend std::ostream& operator<<(std::ostream& os,const Map<K,V>& map) {
            os << "[\n";
            auto items = map.ToStdMap();
            auto it = items.begin();
            while (it != items.end()) {
                os << "    {\n         " << it->first << ": " << it->second << "\n    }";
                ++it;
                if (it != items.end()) {
                    os << ",\n";
                }
            }
            os << "\n]";
            return os;
        }
        //--------------------------------------------------------------------------------------------------------------
        Map() = default;
        explicit Map(const std::map<K,V>& map){
            std::lock_guard<std::mutex> lock(_mutex);
            _map = map;
        }
        Map(const Map<K,V>& map){
            std::lock_guard<std::mutex> lock(_mutex);
            this->_map = map._map;
        }
        Map(const std::initializer_list<std::pair<K, V>>& list){
            std::lock_guard<std::mutex> lock(_mutex);
            for (const auto& item : list) {
                _map.insert(item);
            }
        }
        ~Map() {
            Clear();
        }
        //--------------------------------------------------------------------------------------------------------------
        void Put(K key, V value){
            std::lock_guard<std::mutex> lock(_mutex);
            _map.insert(std::make_pair(key, value));
        }
        void put(K key, V value){
            Put(key,value);
        }
        void Revise(K key, V value){
            std::lock_guard<std::mutex> lock(_mutex);
            _map[key] = value;
        }
        void revise(K key, V value){
            std::lock_guard<std::mutex> lock(_mutex);
            _map[key] = value;
        }
        void Remove(K key){
            std::lock_guard<std::mutex> lock(_mutex);
            _map.erase(key);
        }
        void remove(K key){
            std::lock_guard<std::mutex> lock(_mutex);
            _map.erase(key);
        }
        void Clear(){
            std::lock_guard<std::mutex> lock(_mutex);
            std::map<K, V>().swap(this->_map);
        }
        void clear(){
            std::lock_guard<std::mutex> lock(_mutex);
            std::map<K, V>().swap(this->_map);
        }
        static void swap(Map<K,V>* map) {
            std::map<K, V>().swap(map->_map);
        }
        V* Get(const K& key){
            std::lock_guard<std::mutex> lock(_mutex);
            const bool F = _map.find(key) != _map.end();
            if (!F) {
                return nullptr;
            }
            return &_map[key];
        }
        V* get(const K& key){
           return Get(key);
        }
        V at(const K& key) const{
            std::lock_guard<std::mutex> lock(_mutex);
            return _map.at(key);
        }
        V* operator[](const K& key){
            std::lock_guard<std::mutex> lock(_mutex);
            const bool F = _map.find(key) != _map.end();
            if (!F) {
                return nullptr;
            }
            return &_map[key];
        }
        int Size(){
            std::lock_guard<std::mutex> lock(_mutex);
            return _map.size();
        }
        int size(){
            std::lock_guard<std::mutex> lock(_mutex);
            return _map.size();
        }
        bool IsContains(const K& key){
            std::lock_guard<std::mutex> lock(_mutex);
            return _map.find(key) != _map.end();
        }
        bool IsEmpty(){
            std::lock_guard<std::mutex> lock(_mutex);
            return _map.empty();
        }
        bool IsFull(){
            std::lock_guard<std::mutex> lock(_mutex);
            return _map.size() == _map.max_size();
        }
        std::list<K> Keys(){
            std::lock_guard<std::mutex> lock(_mutex);
            std::list<K> keys;
            for(auto& item : _map){
                keys.push_back(item.first);
            }
            return keys;
        }
        std::list<K> keys(){
            return Keys();
        }
        std::vector<V> Values(){
            std::lock_guard<std::mutex> lock(_mutex);
            std::vector<V> values;
            for(auto& item : _map){
                values.push_back(item.second);
            }
            return values;
        }
        std::vector<V> values(){
            return Values();
        }
        std::map<K,V> ToStdMap(){
            return _map;
        }
        std::map<K,V> ToStdMap() const {  // 实现处同样添加 const
            return _map; // 假设内部存储为 _map
        }
        void SetStdMap(const Map<K,V>& map) {
            std::lock_guard<std::mutex> lock(_mutex);
            _map = map._map;
        }
        void SetStdMap(const std::map<K,V>& map) {
            std::lock_guard<std::mutex> lock(_mutex);
            _map = map;
        }
        typename std::map<K, V>::iterator begin() {
            return _map.begin();
        }
        typename std::map<K, V>::iterator end() {
            return _map.end();
        }
        Map<K,V> operator=(const Map<K,V>& map) {
            std::lock_guard<std::mutex> lock(_mutex);
            _map = map._map;
            return map;
        }
        Map<K,V> operator=(const std::map<K,V>& map) {
            std::lock_guard<std::mutex> lock(_mutex);
            _map = map;
            return this;
        }
        //--------------------------------------------------------------------------------------------------------------
    private:
    };
    template<typename K, typename V>
    class UMap{
    private:
        //--------------------------------------------------------------------------------------------------------------
        mutable std::mutex _mutex;
        std::unordered_map<K, V> _map;
        //--------------------------------------------------------------------------------------------------------------
    public:
        //--------------------------------------------------------------------------------------------------------------
        friend std::ostream& operator<<(std::ostream& os,UMap<K,V>& map) {
            os << "[\n";
            auto items = map.ToStdMap();
            auto it = items.begin();
            while (it != items.end()) {
                os << "    {\n         " << it->first << ": " << it->second << "\n    }";
                ++it;
                if (it != items.end()) {
                    os << ",\n";
                }
            }
            os << "\n]";
            return os;
        }
        friend std::ostream& operator<<(std::ostream& os,const UMap<K,V>& map) {
            os << "[\n";
            auto items = map.ToStdMap();
            auto it = items.begin();
            while (it != items.end()) {
                os << "    {\n         " << it->first << ": " << it->second << "\n    }";
                ++it;
                if (it != items.end()) {
                    os << ",\n";
                }
            }
            os << "\n]";
            return os;
        }
        //--------------------------------------------------------------------------------------------------------------
        UMap() = default;
        explicit UMap(const std::unordered_map<K,V>& map){
            std::lock_guard<std::mutex> lock(_mutex);
            _map = map;
        }
        UMap(const UMap<K,V>& map){
            std::lock_guard<std::mutex> lock(_mutex);
            this->_map = map._map;
        }
        UMap(const std::initializer_list<std::pair<K, V>>& list){
            std::lock_guard<std::mutex> lock(_mutex);
            for (const auto& item : list) {
                _map.insert(item);
            }
        }
        ~UMap() {
            Clear();
        }
        //--------------------------------------------------------------------------------------------------------------
        void Put(K key, V value){
            std::lock_guard<std::mutex> lock(_mutex);
            _map.insert(std::make_pair(key, value));
        }
        void put(K key, V value){
            Put(key,value);
        }
        void Revise(K key, V value){
            std::lock_guard<std::mutex> lock(_mutex);
            _map[key] = value;
        }
        void revise(K key, V value){
            std::lock_guard<std::mutex> lock(_mutex);
            _map[key] = value;
        }
        void Remove(K key){
            std::lock_guard<std::mutex> lock(_mutex);
            _map.erase(key);
        }
        void remove(K key){
            std::lock_guard<std::mutex> lock(_mutex);
            _map.erase(key);
        }
        void Clear(){
            std::lock_guard<std::mutex> lock(_mutex);
            std::unordered_map<K, V>().swap(this->_map);
        }
        void clear(){
            std::lock_guard<std::mutex> lock(_mutex);
            std::unordered_map<K, V>().swap(this->_map);
        }
        static void swap(UMap<K,V>* map) {
            std::unordered_map<K, V>().swap(map->_map);
        }
        V* Get(const K& key){
            std::lock_guard<std::mutex> lock(_mutex);
            const bool F = _map.find(key) != _map.end();
            if (!F) {
                return nullptr;
            }
            return &_map[key];
        }
        V* get(const K& key){
           return Get(key);
        }
        V at(const K& key) const{
            std::lock_guard<std::mutex> lock(_mutex);
            return _map.at(key);
        }
        V* operator[](const K& key){
            std::lock_guard<std::mutex> lock(_mutex);
            const bool F = _map.find(key) != _map.end();
            if (!F) {
                return nullptr;
            }
            return &_map[key];
        }
        int Size(){
            std::lock_guard<std::mutex> lock(_mutex);
            return _map.size();
        }
        int size(){
            std::lock_guard<std::mutex> lock(_mutex);
            return _map.size();
        }
        bool IsContains(const K& key){
            std::lock_guard<std::mutex> lock(_mutex);
            return _map.find(key) != _map.end();
        }
        bool IsEmpty(){
            std::lock_guard<std::mutex> lock(_mutex);
            return _map.empty();
        }
        bool IsFull(){
            std::lock_guard<std::mutex> lock(_mutex);
            return _map.size() == _map.max_size();
        }
        std::list<K> Keys(){
            std::lock_guard<std::mutex> lock(_mutex);
            std::list<K> keys;
            for(auto& item : _map){
                keys.push_back(item.first);
            }
            return keys;
        }
        std::list<K> keys(){
            return Keys();
        }
        std::vector<V> Values(){
            std::lock_guard<std::mutex> lock(_mutex);
            std::vector<V> values;
            for(auto& item : _map){
                values.push_back(item.second);
            }
            return values;
        }
        std::vector<V> values(){
            return Values();
        }
        std::unordered_map<K,V> ToStdMap(){
            return _map;
        }
        std::unordered_map<K,V> ToStdMap() const {  // 实现处同样添加 const
            return _map; // 假设内部存储为 _map
        }
        void SetStdMap(const Map<K,V>& map) {
            std::lock_guard<std::mutex> lock(_mutex);
            _map = map._map;
        }
        void SetStdMap(const std::map<K,V>& map) {
            std::lock_guard<std::mutex> lock(_mutex);
            _map = map;
        }
        typename std::unordered_map<K, V>::iterator begin() {
            return _map.begin();
        }
        typename std::unordered_map<K, V>::iterator end() {
            return _map.end();
        }
        UMap<K,V> operator=(const UMap<K,V>& map) {
            std::lock_guard<std::mutex> lock(_mutex);
            _map = map._map;
            return map;
        }
        UMap<K,V> operator=(const std::unordered_map<K,V>& map) {
            std::lock_guard<std::mutex> lock(_mutex);
            _map = map;
            return this;
        }
        //--------------------------------------------------------------------------------------------------------------
    private:
    };
    template<typename K, typename V>
    class MapPtr:public Map<K,V*> {
    public:

    };
}

#endif
