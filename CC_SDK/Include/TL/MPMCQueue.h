#ifndef SRCCTL_MP_MC_QUEUE_H
#define SRCCTL_MP_MC_QUEUE_H

#include <atomic>
#include <vector>
#include <cassert>
#include <memory>

namespace CTL {
    template<typename T>
    class MPMCQueue {
    public:
        explicit MPMCQueue(size_t capacity)
            : capacity_(capacity),
              buffer_(capacity),
              states_(new std::atomic<State>[capacity]()),
              head_(0),
              tail_(0) {
            assert(capacity > 1 && "capacity must be > 1");
        }
        ~MPMCQueue() {
            delete[] states_;
        }
        MPMCQueue(const MPMCQueue&) = delete;
        MPMCQueue& operator=(const MPMCQueue&) = delete;
        bool add(const T& value) {
            size_t tail = tail_.load(std::memory_order_relaxed);
            for (;;) {
                size_t idx = tail % capacity_;
                State expected = State::Empty;
                if (states_[idx].compare_exchange_weak(
                        expected, State::Writing,
                        std::memory_order_acq_rel)) {
                    buffer_[idx] = value;
                    states_[idx].store(State::Full,std::memory_order_release);
                    // 推进 tail（允许失败）
                    tail_.compare_exchange_weak(tail, tail + 1,std::memory_order_release);
                    return true;
                }

                if (states_[idx].load(std::memory_order_acquire) != State::Empty) {
                    size_t h = head_.load(std::memory_order_acquire);
                    if (tail - h >= capacity_)
                        return false; // full
                }

                tail = tail_.load(std::memory_order_relaxed);
            }
        }
        bool pop(T& out) {
            size_t head = head_.load(std::memory_order_relaxed);
            for (;;) {
                size_t idx = head % capacity_;
                State expected = State::Full;
                if (states_[idx].compare_exchange_weak(
                        expected, State::Reading,
                        std::memory_order_acq_rel)) {
                    out = buffer_[idx];
                    states_[idx].store(State::Empty,std::memory_order_release);
                    head_.compare_exchange_weak(head, head + 1,std::memory_order_release);
                    return true;
                }
                if (states_[idx].load(std::memory_order_acquire) != State::Full) {
                    size_t t = tail_.load(std::memory_order_acquire);
                    if (t <= head) {
                        return false; // empty
                    }
                }
                head = head_.load(std::memory_order_relaxed);
            }
        }
        size_t size() const {
            size_t h = head_.load(std::memory_order_acquire);
            size_t t = tail_.load(std::memory_order_acquire);
            return t - h;
        }
        bool empty() const {
            return size() == 0;
        }
    private:
        enum class State : uint8_t {
            Empty,
            Writing,
            Full,
            Reading
        };
        size_t capacity_;
        std::vector<T> buffer_;
        std::atomic<State>* states_;
        std::atomic<size_t> head_;
        std::atomic<size_t> tail_;
    };
}

#endif
