#ifndef SRC_CTL_RING_BUFFER_H
#define SRC_CTL_RING_BUFFER_H

#include "mutex"
#include <stdexcept>
#include <iterator>

namespace CTL {
    template <typename T>
    class RingBuffer {
    private:
        T* buffer;
        size_t capacity;
        size_t head;
        size_t tail;
        size_t count;
        mutable std::mutex mutex_t;

    public:
        // 迭代器类
        class iterator {
        private:
            const RingBuffer* ringBuffer;
            size_t currentIndex;

        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T*;
            using reference = T&;

            iterator(const RingBuffer* rb, size_t index)
                : ringBuffer(rb), currentIndex(index) {}

            reference operator*() const {
                if (currentIndex >= ringBuffer->count) {
                    throw std::out_of_range("Iterator dereference out of range");
                }
                return ringBuffer->buffer[(ringBuffer->head + currentIndex) % ringBuffer->capacity];
            }

            pointer operator->() const {
                if (currentIndex >= ringBuffer->count) {
                    throw std::out_of_range("Iterator access out of range");
                }
                return &(ringBuffer->buffer[(ringBuffer->head + currentIndex) % ringBuffer->capacity]);
            }

            iterator& operator++() {
                ++currentIndex;
                return *this;
            }

            iterator operator++(int) {
                iterator temp = *this;
                ++(*this);
                return temp;
            }

            iterator& operator--() {
                if (currentIndex == 0) {
                    throw std::out_of_range("Iterator decrement out of range");
                }
                --currentIndex;
                return *this;
            }

            iterator operator--(int) {
                iterator temp = *this;
                --(*this);
                return temp;
            }

            bool operator==(const iterator& other) const {
                return ringBuffer == other.ringBuffer && currentIndex == other.currentIndex;
            }

            bool operator!=(const iterator& other) const {
                return !(*this == other);
            }

            friend class RingBuffer;
        };

        // const 迭代器类
        class const_iterator {
        private:
            const RingBuffer* ringBuffer;
            size_t currentIndex;

        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = const T*;
            using reference = const T&;

            const_iterator(const RingBuffer* rb, size_t index)
                : ringBuffer(rb), currentIndex(index) {}

            // 从 iterator 转换
            const_iterator(const iterator& it)
                : ringBuffer(it.ringBuffer), currentIndex(it.currentIndex) {}

            reference operator*() const {
                if (currentIndex >= ringBuffer->count) {
                    throw std::out_of_range("Const iterator dereference out of range");
                }
                return ringBuffer->buffer[(ringBuffer->head + currentIndex) % ringBuffer->capacity];
            }

            pointer operator->() const {
                if (currentIndex >= ringBuffer->count) {
                    throw std::out_of_range("Const iterator access out of range");
                }
                return &(ringBuffer->buffer[(ringBuffer->head + currentIndex) % ringBuffer->capacity]);
            }

            const_iterator& operator++() {
                ++currentIndex;
                return *this;
            }

            const_iterator operator++(int) {
                const_iterator temp = *this;
                ++(*this);
                return temp;
            }

            const_iterator& operator--() {
                if (currentIndex == 0) {
                    throw std::out_of_range("Const iterator decrement out of range");
                }
                --currentIndex;
                return *this;
            }

            const_iterator operator--(int) {
                const_iterator temp = *this;
                --(*this);
                return temp;
            }

            bool operator==(const const_iterator& other) const {
                return ringBuffer == other.ringBuffer && currentIndex == other.currentIndex;
            }

            bool operator!=(const const_iterator& other) const {
                return !(*this == other);
            }

            friend class RingBuffer;
        };

    public:
        RingBuffer() {
            capacity = 0;
            head = 0;
            tail = 0;
            count = 0;
            resize(2048);
        }
        explicit RingBuffer(const size_t size)
            : head(0), tail(0), count(0) {
            resize(size);
        }
        ~RingBuffer() {
            delete[] buffer;
            buffer = nullptr;
        }
        RingBuffer(const RingBuffer&) = delete;
        RingBuffer& operator=(const RingBuffer&) = delete;
        void clear() {
            std::unique_lock<std::mutex> lock(mutex_t);
            head = tail;
            count = 0;
        }
        void resize(const uint64_t size) {
            std::unique_lock<std::mutex> lock(mutex_t);
            if (buffer && capacity > 0) {
                delete[] buffer;
            }
            capacity = size;
            buffer = new T[size]();
        }
        bool add(const T& item) {
            std::unique_lock<std::mutex> lock(mutex_t);
            buffer[tail] = item;
            tail = (tail + 1) % capacity;
            ++count;
            return true;
        }
        bool take(T* item) {
            if (isEmpty() || !item) {
                return false;
            }
            std::unique_lock<std::mutex> lock(mutex_t);
            *item = buffer[head];
            head = (head + 1) % capacity;
            --count;
            return true;
        }
        T front() const {
            if (isEmpty()) {
                throw std::runtime_error("RingBuffer is empty");
            }
            std::unique_lock<std::mutex> lock(mutex_t);
            return buffer[head];
        }
        T back() const {
            if (isEmpty()) {
                throw std::runtime_error("RingBuffer is empty");
            }
            std::unique_lock<std::mutex> lock(mutex_t);
            // 计算最后一个有效元素的位置
            size_t last_pos = (tail == 0) ? (capacity - 1) : (tail - 1);
            return buffer[last_pos];
        }
        T operator[](const size_t index) const {
            std::unique_lock<std::mutex> lock(mutex_t);
            if (index >= count) {
                throw std::out_of_range("RingBuffer operator[] Index out of range");
            }
            return buffer[(head + index) % capacity];
        }
        T& operator[](const size_t index) {
            std::unique_lock<std::mutex> lock(mutex_t);
            if (index >= count) {
                throw std::out_of_range("RingBuffer operator[] Index out of range");
            }
            return buffer[(head + index) % capacity];
        }
        [[nodiscard]] bool isEmpty() const {
            std::unique_lock<std::mutex> lock(mutex_t);
            return count == 0;
        }
        [[nodiscard]] bool isFull() const {
            std::unique_lock<std::mutex> lock(mutex_t);
            return count == capacity;
        }
        [[nodiscard]] size_t size() const {
            std::unique_lock<std::mutex> lock(mutex_t);
            return count;
        }
        [[nodiscard]] size_t getCapacity() const {
            std::unique_lock<std::mutex> lock(mutex_t);
            return capacity;
        }
        // 迭代器相关方法
        iterator begin() {
            return iterator(this, 0);
        }
        iterator end() {
            return iterator(this, count);
        }
        const_iterator begin() const {
            return const_iterator(this, 0);
        }
        const_iterator end() const {
            return const_iterator(this, count);
        }
        const_iterator c_begin() const {
            return const_iterator(this, 0);
        }
        const_iterator c_end() const {
            return const_iterator(this, count);
        }
    };
}

#endif
