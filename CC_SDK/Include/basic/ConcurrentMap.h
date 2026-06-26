#ifndef CONCURRENTMAP_H
#define CONCURRENTMAP_H

#include <iostream>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <iterator>
#include <atomic>

#include "ConcurrentLinkedList.h"

namespace CTL {
    /**
     * ConcurrentMap类提供了一个线程安全的映射实现。
     * 它使用链表和哈希映射的组合来存储键值对，以实现并发访问和修改。
     *
     * @tparam Key 键的类型
     * @tparam Value 值的类型
     */
    template <typename Key, typename Value>
    class ConcurrentMap {
    public:
        // 类型别名定义
        using key_type = Key;
        using mapped_type = Value;
        using value_type = std::pair<const Key, Value>;
        using size_type = std::size_t;

    private:
        // 链表节点结构体
        struct Node {
            value_type data;
            std::atomic<Node*> next;
            std::atomic<Node*> prev;
            Node(const value_type& d) : data(d), next(nullptr), prev(nullptr) {}
        };

        using node_ptr = std::shared_ptr<Node>;  // 使用 shared_ptr 管理节点

        // 头尾指针和元素计数
        std::atomic<Node*> head;
        std::atomic<Node*> tail;
        std::unordered_map<Key, node_ptr> map;  // 使用 shared_ptr 代替 atomic
        std::atomic<size_type> count;

    public:
        // 默认构造函数
        ConcurrentMap() : head(nullptr), tail(nullptr), count(0) {}

        // 析构函数，清理所有节点
        ~ConcurrentMap() {
            clear();
        }

        // 清空链表和哈希映射中的所有节点
        void clear() {
            Node* current = head.load();
            while (current) {
                Node* next = current->next.load();
                map.erase(current->data.first);  // 从 map 中移除节点
                current->next.store(nullptr);
                current->prev.store(nullptr);
                current = next;
            }
            head.store(nullptr);
            tail.store(nullptr);
            map.clear();
            count.store(0);
        }

        // 获取当前元素数量
        size_type size() const {
            return count.load();
        }

        // 检查映射是否为空
        bool IsEmpty() const {
            return count.load() == 0;
        }

        // 插入或获取指定键的值
        Value& operator[](const Key& key) {
            auto newNode = std::make_shared<Node>(value_type(key, Value()));
            node_ptr insertedNode = insertOrGetNode(key, newNode);
            if (insertedNode != newNode) {
                // 不需要显式删除 newNode，shared_ptr 会自动管理
            }
            return insertedNode->data.second;
        }

        // 获取指定键的值，如果键不存在则抛出异常
        Value& get(const Key& key) {
            node_ptr node = getNode(key);
            if (!node) {
                throw std::out_of_range("Key not found in CCLinkedMap");
            }
            return node->data.second;
        }

        // 获取指定键的值，如果键不存在则抛出异常（常量版本）
        const Value& get(const Key& key) const {
            node_ptr node = getNode(key);
            if (!node) {
                throw std::out_of_range("Key not found in CCLinkedMap");
            }
            return node->data.second;
        }

        // 移除指定键的节点
        void remove(const Key& key) {
            node_ptr nodeToRemove = getNode(key);
            if (!nodeToRemove) {
                return;
            }

            Node* prevNode = nodeToRemove->prev.load();
            Node* nextNode = nodeToRemove->next.load();

            if (prevNode) {
                prevNode->next.store(nextNode);
            } else {
                head.store(nextNode);
            }

            if (nextNode) {
                nextNode->prev.store(prevNode);
            } else {
                tail.store(prevNode);
            }

            map.erase(key);
            count.fetch_sub(1);
        }
        std::list<Value> values() {
            std::list<Value> result;
            for (auto it = map.begin(); it != map.end(); ++it) {
                result.push_back(it->second->data.second);
            }
            return result;
        }
        ArrayList<Value> toArrayList() {
            ArrayList<Value> result;
            for (auto it = map.begin(); it != map.end(); ++it) {
                result.add(it->second->data.second);
            }
            return result;
        }
        ConcurrentLinkedList<Value> toConcurrentLinkedList() {
            ConcurrentLinkedList<Value> result;
            for (auto it = map.begin(); it != map.end(); ++it) {
                result.add(it->second->data.second);
            }
            return result;
        }
    private:
        // 插入新节点或获取已存在节点的指针
        node_ptr insertOrGetNode(const Key& key, node_ptr newNode) {
            while (true) {
                auto it = map.find(key);
                if (it != map.end()) {
                    return it->second;
                }

                auto [itInserted, success] = map.emplace(key, newNode);
                if (success) {
                    Node* prevTail = tail.load();
                    while (!tail.compare_exchange_weak(prevTail, newNode.get())) {
                        if (prevTail) {
                            prevTail->next.store(newNode.get());
                            newNode->prev.store(prevTail);
                        } else {
                            head.store(newNode.get());
                        }
                    }
                    count.fetch_add(1);
                    return newNode;
                } else {
                    return itInserted->second;
                }
            }
        }

        // 获取指定键的节点指针，如果键不存在则返回nullptr
        node_ptr getNode(const Key& key) const {
            auto it = map.find(key);
            if (it != map.end()) {
                return it->second;
            }
            return nullptr;
        }

    public:
        // 迭代器类，用于遍历ConcurrentMap
        class iterator {
        public:
            // 迭代器类型定义
            using iterator_category = std::forward_iterator_tag;
            using value_type = std::pair<const Key, Value>;
            using difference_type = std::ptrdiff_t;
            using pointer = value_type*;
            using reference = value_type&;

            iterator(Node* ptr) : current(ptr) {}

            reference operator*() const {
                return current->data;
            }

            pointer operator->() const {
                return &(current->data);
            }

            iterator& operator++() {
                if (current) {
                    current = current->next.load();
                }
                return *this;
            }

            iterator operator++(int) {
                iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            friend bool operator==(const iterator& a, const iterator& b) {
                return a.current == b.current;
            }

            friend bool operator!=(const iterator& a, const iterator& b) {
                return a.current != b.current;
            }

        private:
            Node* current;
        };

        // 常量迭代器类，用于遍历ConcurrentMap
        class const_iterator {
        public:
            // 迭代器类型定义
            using iterator_category = std::forward_iterator_tag;
            using value_type = std::pair<const Key, Value>;
            using difference_type = std::ptrdiff_t;
            using pointer = const value_type*;
            using reference = const value_type&;

            const_iterator(Node* ptr) : current(ptr) {}

            reference operator*() const {
                return current->data;
            }

            pointer operator->() const {
                return &(current->data);
            }

            const_iterator& operator++() {
                if (current) {
                    current = current->next.load();
                }
                return *this;
            }

            const_iterator operator++(int) {
                const_iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            friend bool operator==(const const_iterator& a, const const_iterator& b) {
                return a.current == b.current;
            }

            friend bool operator!=(const const_iterator& a, const const_iterator& b) {
                return a.current != b.current;
            }

        private:
            Node* current;
        };

        // 迭代器访问方法
        iterator begin() {
            return iterator(head.load());
        }

        iterator end() {
            return iterator(nullptr);
        }

        const_iterator begin() const {
            return const_iterator(head.load());
        }

        const_iterator end() const {
            return const_iterator(nullptr);
        }

        const_iterator cbegin() const {
            return const_iterator(head.load());
        }

        const_iterator cend() const {
            return const_iterator(nullptr);
        }

        // 友元函数，用于输出ConcurrentMap的内容
        friend std::ostream& operator<<(std::ostream& os, const ConcurrentMap<Key, Value>& lm) {
            os << "[";
            bool first = true;
            for (const auto& pair : lm) {
                if (!first) {
                    os << ",";
                }
                os << "{ " << pair.first << " : " << pair.second << " }";
                first = false;
            }
            os << "]";
            return os;
        }
    };
}

#endif
