#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "macros.h"

namespace Common {

/* UPDATE POSSIBLE: Memory Pool implementation. Fixed size upon Creation. Could extend it for resize functionality. */
// Memory pool allocates memory dynamically with creation of the vector store_. So we should create the memory pool
// before the execution of the critical path (due to latency issues).

template <typename T>
class MemPool final{
private:
    // This will represent a block of memory containing an object and a flag to see if the block is free
    // in the memory pool
    struct ObjectBlock{
        T object_;
        bool is_free_ = true;
    };
    // vector of ObjectBlocks to represent the pool of all available memory
    std::vector<ObjectBlock> store_;
    // index to track where te next free block is located in the pool
    size_t next_free_index = 0;

    auto UpdateNextFreeIndex() noexcept{
        const auto initial_free_index = next_free_index;
        while(!store_[next_free_index].is_free_){
            ++next_free_index;
            if(UNLIKELY(next_free_index == store_.size())){
                // hardware branch predictor should almost always predict this to be false already
                next_free_index = 0;
            }
            if(UNLIKELY(initial_free_index == next_free_index)){
                Assert(initial_free_index != next_free_index, "Memory pool out of space!");
            }
        }
    }

public: 
    /* Constructors */
    // pre-allocation of vector storage
    explicit MemPool(std::size_t num_elements): store_(num_elements, {T(), true}){
        ASSERT(reinterpret_cast<const ObjectBlock*>(&(store_[0].object_)) == &(store_[0],"T Object should be the first member of ObjectBlock."));
    }

    // Deleting default constructors & destructors to decrease latency
    MemPool() = delete;
    // copy constructor deletion
    MemPool(const MemPool&) = delete;
    // move constructor of Rvalue reference
    Mempool(const MemPool&&) = delete;
    // assignment operator deletion
    MemPool& operator=(const MemPool&) = delete;
    MemPool& operator=(const MemPool&&) = delete;

    template<typename... Args>
    T* allocate(Args... args) noexcept{
        // gets pointer to memory location of next free block of memory in our memory pool
        auto obj_block = &(store_[next_free_index_]);
        // confirms that the memory is free
        ASSERT(obj_block->isfree_, "Expected free ObjectBlock at index:" + std::to_string(next_free_index_));
        // placement new to create new memory at 
        T* ret = new(&(obj_block->object_)) T(args...);
        obj_block->is_free_ = false;
        UpdateNextFreeIndex();
        return ret;
    }
};

};