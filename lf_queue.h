#pragma once
#include <iostream>
#include <vector>
#include <atomic>

namespace Common{

template<typename T>
class LFQueue final{
private:
    std::vector<T> store_;
    // must be atomic data types because the read/write operations on these will be performed by different threads
    std::atomic<size_t> next_write_index_ = {0};
    std::atomic<size_t> next_read_index_ = {0};
    std::atomic<size_t> num_elements_ = {0};

public:
    /* Constructors */
    // pre-allocation of vector storage
    LFQueue(std::size_t num_elems): store_(num_elems, T()){

    }

    // deleting defaults to reduce latency
    LFQueue() = delete;
    LFQueue(const LFQueue&) = delete;
    LFQueue(const LFQueue&&) = delete;
    LFQueue& operator=(const LFQueue&) = delete;
    LFQueue& operator=(const LFQueue&&) = delete;

    auto GetNextToWriteTo() noexcept{
        return &store[next_write_index_];
    }

    auto UpdateWriteIndex() noexcept{
        next_write_index_ = (next_write_index_ + 1) % store_.size();
        num_elements_++;
    }
};

};