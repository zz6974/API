#ifndef CONCURRENTLINKEDLIST_H
#define CONCURRENTLINKEDLIST_H

#include <atomic>
#include <memory> // for std::allocator
#include <stdexcept> // for std::out_of_range

/**
 * 命名空间 CTL，用于并发线程安全链表
 */
namespace CTL {

    /**
     * 模板类 ConcurrentLinkedList 实现了一个线程安全的单向链表。
     *
     * @tparam T 链表中存储的数据类型
     * @tparam Alloc 分配器类型，默认为 std::allocator<T>
     */
    template<typename T, class Alloc = std::allocator<T>>
    class ConcurrentLinkedList {
        /**
         * 链表节点结构体
         */
    public:
        struct Node {
            T data; // 节点中存储的数据
            std::atomic<Node*> next; // 指向下一个节点的原子指针
            explicit Node(const T& value) : data(value), next(nullptr) {} // 构造函数
        };

        typedef typename std::allocator_traits<Alloc>::template rebind_alloc<Node> Node_alloc_type;
        Node_alloc_type allocator_; // 节点分配器
        std::atomic<Node*> head; // 指向链表头节点的原子指针

    public:
        /**
         * 构造函数，初始化空链表
         */
        ConcurrentLinkedList() : head(nullptr) {}
        ConcurrentLinkedList(const ConcurrentLinkedList& other) {
            typename ConcurrentLinkedList<T, Alloc>::Node* current = other.head.load();
            while (current) {
                add(current->data);
                current = current->next.load();
            }
        }
        ConcurrentLinkedList(const std::list<T> &list) {
            for (const auto &item : list) {
                add(item);
            }
        }
        /**
         * 析构函数，释放链表中的所有节点
         */
        ~ConcurrentLinkedList() {
            Node* current = head.load();
            while (current != nullptr) {
                Node* next = current->next.load();
                std::allocator_traits<Node_alloc_type>::destroy(allocator_, current);
                allocator_.deallocate(current, 1);
                current = next;
            }
        }

        /**
         * 在链表头部插入一个元素
         *
         * @param value 要插入的元素值
         */
        void push_front(const T& value) {
            Node* new_node = allocator_.allocate(1);
            std::allocator_traits<Node_alloc_type>::construct(allocator_, new_node, value);
            new_node->next.store(head.load());
            while (!head.compare_exchange_weak(new_node->next.load(), new_node)) {
                new_node->next.store(head.load());
            }
        }

        /**
         * 移除并返回链表头部的元素
         *
         * @param result 存储移除的元素值
         * @return 如果操作成功返回 true，否则返回 false
         */
        bool pop_front(T& result) {
            Node* old_head = head.load();
            while (old_head != nullptr && !head.compare_exchange_weak(old_head, old_head->next.load())) {
                // 重试
            }
            if (old_head == nullptr) {
                return false;
            }
            result = old_head->data;
            std::allocator_traits<Node_alloc_type>::destroy(allocator_, old_head);
            allocator_.deallocate(old_head, 1);
            return true;
        }

        /**
         * 在链表尾部添加一个元素
         *
         * @param value 要添加的元素值
         */
        void add(const T& value) {
            Node* new_node = allocator_.allocate(1);
            std::allocator_traits<Node_alloc_type>::construct(allocator_, new_node, value);
            new_node->next.store(nullptr);
            Node* current = head.load();
            if (!current) {
                head.store(new_node);
                return;
            }

            while (true) {
                Node* next = current->next.load();
                if (!next) {
                    if (current->next.compare_exchange_weak(next, new_node)) {
                        return;
                    }
                }
                else {
                    current = next;
                }
            }
        }

        /**
         * 移除链表中第一个匹配指定值的元素
         *
         * @param value 要移除的元素值
         * @return 如果操作成功返回 true，否则返回 false
         */
        bool remove(const T& value) {
            Node* current = head.load();
            Node* prev = nullptr;
            while (current) {
                Node* next = current->next.load();
                if (current->data == value) {
                    if (prev) {
                        prev->next.store(next);
                    }
                    else {
                        head.store(next);
                    }
                    std::allocator_traits<Node_alloc_type>::destroy(allocator_, current);
                    allocator_.deallocate(current, 1);
                    return true;
                }
                prev = current;
                current = next;
            }
            return false;
        }

        /**
         * 返回链表中元素的数量
         *
         * @return 链表中元素的数量
         */
        size_t size() const {
            size_t count = 0;
            Node* current = head.load();
            while (current) {
                count++;
                current = current->next.load();
            }
            return count;
        }

        /**
         * 返回链表中指定索引处的元素值
         *
         * @param index 元素的索引位置
         * @return 索引处的元素值
         * @note 如果索引超出范围，行为未定义
         */
        T get(size_t index) const {
            Node* current = head.load();
            size_t count = 0;
            while (current) {
                if (count == index) {
                    return current->data;
                }
                count++;
                current = current->next.load();
            }
            // throw std::out_of_range("Index out of range");
            return T();
        }

        /**
         * 检查链表是否为空
         *
         * @return 如果链表为空返回 true，否则返回 false
         */
        bool isEmpty() const {
            return head.load() == nullptr;
        }
        friend std::ostream &operator<<(std::ostream &os, const ConcurrentLinkedList<T, Alloc> &list) {
            typename ConcurrentLinkedList<T, Alloc>::Node* current = list.head.load();
            os<< "[";
            while (current) {
                os << current->data << ", ";
                current = current->next.load();
            }
            os << "]";
            return os;
        }
    };
}

#endif
