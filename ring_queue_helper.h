// cite from : https://blog.csdn.net/techenliu/article/details/132450899

#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

#include <cstddef>
#include <mutex>
#include <condition_variable> 
template <typename T>
class CircularQueue {
public:
    explicit CircularQueue(size_t capacity = 512) :
        capacity_(capacity),
        size_(0),
        head_(0),
        tail_(0),
        buffer_(new T[capacity]) {}

    ~CircularQueue() {
        delete[] buffer_;
    }

    bool empty() {
        std::unique_lock<std::mutex> lock(mutex_);
        return size_ == 0;
    }

    bool full() {
        std::unique_lock<std::mutex> lock(mutex_);
        return size_ == capacity_;
    }

    size_t size() {
        std::unique_lock<std::mutex> lock(mutex_);
        return size_;
    }

    size_t capacity() {
        return capacity_;
    }

    bool push(const T& value, bool block = true) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (block) {
            while (size_ == capacity_) {
                not_full_.wait(lock);
            }
        } else {
            if (size_ == capacity_) {
                return false;
            }
        }

        buffer_[tail_] = value;
        tail_ = (tail_ + 1) % capacity_;
        ++size_;

        not_empty_.notify_one();

        return true;
    }

    bool push(T&& value, bool block = true) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (block) {
            while (size_ == capacity_) {
                not_full_.wait(lock);
            }
        } else {
            if (size_ == capacity_) {
                return false;
            }
        }

        buffer_[tail_] = std::move(value);
        tail_ = (tail_ + 1) % capacity_;
        ++size_;

        not_empty_.notify_one();

        return true;
    }

    bool pop(T& value, bool block = true) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (block) {
            while (size_ == 0) {
                not_empty_.wait(lock);
            }
        } else {
            if (size_ == 0) {
                return false;
            }
        }

        value = std::move(buffer_[head_]);
        head_ = (head_ + 1) % capacity_;
        --size_;

        not_full_.notify_one();

        return true;
    }

private:
    size_t capacity_;
    size_t size_; 
    size_t head_;
    size_t tail_;
    T* buffer_;
    std::mutex mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
};

#endif // CIRCULAR_QUEUE_H