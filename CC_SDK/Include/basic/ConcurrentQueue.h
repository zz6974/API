#ifndef CONCURRENTQUEUE_H
#define CONCURRENTQUEUE_H

#include <atomic>
#include <memory>

/**
 * @namespace CTL
 * @brief 并发数据结构库
 */
namespace CTL {
    /**
     * @template class ConcurrentQueue
     * @brief 无锁并发队列实现
     *
     * 该队列设计用于多线程环境，允许多个线程在无需锁的情况下安全地入队和出队元素。通过使用原子操作实现线程安全性。
     */
    template<typename T>
    class ConcurrentQueue {
        /**
         * @struct Node
         * @brief 队列内部节点结构
         */
        struct Node {
            std::shared_ptr<T> data; // 节点存储的数据
            std::atomic<Node*> next; // 指向下一个节点的指针
            /**
             * Node 构造函数
             *
             * @param value 要存储在节点中的数据
             */
            explicit Node(T const& value) : data(std::make_shared<T>(value)) {}
        };
        std::atomic<Node*> head; // 队列头指针
        std::atomic<Node*> tail; // 队列尾指针
        std::atomic<size_t> size_; // 新增一个原子计数器来记录队列大小

    public:
        /**
         * ConcurrentQueue 构造函数
         * 初始化头指针和尾指针，并将大小设置为0
         */
        ConcurrentQueue() : head(new Node(T())), tail(head.load()), size_(0) {}

        /**
         * ConcurrentQueue 析构函数
         * 删除队列中的所有节点
         */
        ~ConcurrentQueue() {
            while (Node* const oldHead = head.load()) {
                head.store(oldHead->next);
                delete oldHead;
            }
        }

        /**
         * 将元素添加到队列末尾
         *
         * @param data 要添加到队列中的数据
         */
        void add(T const& data) {
            std::shared_ptr<T> newData(std::make_shared<T>(data));
            Node* p = new Node(T());
            Node* oldTail = tail.load();
            oldTail->data.swap(newData);
            oldTail->next.store(p);
            tail.store(p);
            size_.fetch_add(1); // 增加计数器
        }

        /**
         * 移除并返回队列的第一个元素
         *
         * @return 指向移除元素的共享指针，如果队列为空则返回空的共享指针
         */
        std::shared_ptr<T> front() {
            Node* oldHead = head.load();
            while (oldHead && !head.compare_exchange_weak(oldHead, oldHead->next)) {
                // 重试
            }
            if (!oldHead) {
                return std::shared_ptr<T>();
            }
            std::shared_ptr<T> res(oldHead->data);
            delete oldHead;
            size_.fetch_sub(1); // 减少计数器
            return res;
        }
        /**
         * 返回当前队列的大小
         *
         * @return 队列的大小
         */
        [[nodiscard]] size_t size() const {
            return size_.load(); // 返回当前队列大小
        }
        /**
         * 检查队列是否为空
         *
         * @return 如果队列为空返回 true，否则返回 false
         */
        [[nodiscard]] bool isEmpty() const {
            return size_.load() == 0; // 判断队列是否为空
        }
    };
}

#endif
