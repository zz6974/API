#ifndef CC_LINKEDMAP_H
#define CC_LINKEDMAP_H

#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <iterator>
#include <vector>
#include <list>
#include <iostream>

namespace CTL {
    template <typename Key, typename Value>
    class LinkedMap {
    public:
        using key_type = Key;
        using mapped_type = Value;
        using value_type = std::pair<const Key, Value>;
        using size_type = std::size_t;

    private:
        // 定义链表节点结构体
        struct Node {
            value_type data;
            std::shared_ptr<Node> next;
            std::weak_ptr<Node> prev;
            Node(const value_type& d) : data(d), next(nullptr) {}
        };

        using node_ptr = std::shared_ptr<Node>;
        using weak_node_ptr = std::weak_ptr<Node>;

        // 链表头尾指针
        node_ptr head;
        node_ptr tail;
        // 哈希表用于快速查找
        std::unordered_map<Key, node_ptr> map;
        size_type count;

    public:
        // 默认构造函数
        LinkedMap() : head(nullptr), tail(nullptr), count(0) {}

        // 析构函数，清除所有节点
        ~LinkedMap() {
            clear();
        }

        // 清除所有节点和哈希表项
        void clear() {
            head.reset();
            tail.reset();
            map.clear();
            count = 0;
        }

        // 返回链表中节点的数量
        size_type size() const {
            return count;
        }

        // 判断链表是否为空
        bool empty() const {
            return count == 0;
        }

        // 通过键值获取对应的值，如果键不存在则创建新节点
        Value& operator[](const Key& key) {
            auto it = map.find(key);
            if (it != map.end()) {
                return it->second->data.second;
            }
            node_ptr newNode = std::make_shared<Node>(value_type(key, Value()));
            map[key] = newNode;
            if (!head) {
                head = tail = newNode;
            } else {
                tail->next = newNode;
                newNode->prev = tail;
                tail = newNode;
            }
            ++count;
            return newNode->data.second;
        }

        // 通过键值获取对应的值，如果键不存在则抛出异常
        Value& get(const Key& key) {
            auto it = map.find(key);
            if (it != map.end()) {
                return it->second->data.second;
            }
            throw std::out_of_range("Key not found in CCLinkedMap");
        }

        // 通过键值获取对应的值，如果键不存在则抛出异常，const版本
        const Value& get(const Key& key) const {
            auto it = map.find(key);
            if (it != map.end()) {
                return it->second->data.second;
            }
            throw std::out_of_range("Key not found in CCLinkedMap");
        }

        // 移除指定键值的节点，如果键不存在则抛出异常
        void remove(const Key& key) {
            auto it = map.find(key);
            if (it != map.end()) {
                node_ptr nodeToRemove = it->second;
                map.erase(it);

                if (nodeToRemove->prev.lock()) {
                    nodeToRemove->prev.lock()->next = nodeToRemove->next;
                } else {
                    head = nodeToRemove->next;
                }

                if (nodeToRemove->next) {
                    nodeToRemove->next->prev = nodeToRemove->prev;
                } else {
                    tail = nodeToRemove->prev.lock();
                }

                --count;
            }
        }

        // 定义迭代器类
        class iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = std::pair<const Key, Value>;
            using difference_type = std::ptrdiff_t;
            using pointer = value_type*;
            using reference = value_type&;

            iterator(node_ptr ptr) : current(ptr) {}

            reference operator*() const {
                return current->data;
            }

            pointer operator->() const {
                return &(current->data);
            }

            iterator& operator++() {
                if (current) {
                    current = current->next;
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
            node_ptr current;
        };

        // 定义常量迭代器类
        class const_iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = std::pair<const Key, Value>;
            using difference_type = std::ptrdiff_t;
            using pointer = const value_type*;
            using reference = const value_type&;

            const_iterator(node_ptr ptr) : current(ptr) {}

            reference operator*() const {
                return current->data;
            }

            pointer operator->() const {
                return &(current->data);
            }

            const_iterator& operator++() {
                if (current) {
                    current = current->next;
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
            node_ptr current;
        };

        // 返回链表开始的迭代器
        iterator begin() {
            return iterator(head);
        }

        // 返回链表结束的迭代器
        iterator end() {
            return iterator(nullptr);
        }

        // 返回常量链表开始的迭代器
        const_iterator begin() const {
            return const_iterator(head);
        }

        // 返回常量链表结束的迭代器
        const_iterator end() const {
            return const_iterator(nullptr);
        }

        // 返回常量链表开始的迭代器
        const_iterator cbegin() const {
            return const_iterator(head);
        }

        // 返回常量链表结束的迭代器
        const_iterator cend() const {
            return const_iterator(nullptr);
        }

        // 返回所有值的向量
        std::vector<Value> values() {
            std::vector<Value> result;
            for (const auto& pair : *this) {
                result.push_back(pair.second);
            }
            return result;
        }

        // 返回所有值的列表
        std::list<Value> valuesList() {
            std::list<Value> result;
            for (const auto& pair : *this) {
                result.push_back(pair.second);
            }
            return result;
        }

        // 返回所有值的ArrayList
        ArrayList<Value> valuesArrayList() {
            ArrayList<Value> result;
            for (const auto& pair : *this) {
                result.add(pair.second);
            }
            return result;
        }

        // 重载输出运算符，输出链表的所有节点
        friend std::ostream& operator<<(std::ostream& os, const LinkedMap<Key, Value>& lm) {
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
