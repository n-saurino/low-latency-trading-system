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
    size_t next_free_index_ = 0;

    auto UpdateNextFreeIndex() noexcept{
        const auto initial_free_index = next_free_index_;
        while(!store_[next_free_index_].is_free_){
            ++next_free_index_;
            if(UNLIKELY(next_free_index_ == store_.size())){
                // hardware branch predictor should almost always predict this to be false already
                next_free_index_ = 0;
            }
            if(UNLIKELY(initial_free_index == next_free_index_)){
                ASSERT(initial_free_index != next_free_index_, "Memory pool out of space!");
            }
        }
    }

public: 
    /* Constructors */
    // pre-allocation of vector storage
    explicit MemPool(std::size_t num_elements): store_(num_elements, {T(), true}){
        ASSERT(reinterpret_cast<const ObjectBlock*>(&(store_[0].object_)) == &(store_[0]),"T Object should be the first member of ObjectBlock.");
    }

    // Deleting default constructors & destructors to decrease latency
    MemPool() = delete;
    // copy constructor deletion
    MemPool(const MemPool&) = delete;
    // move constructor of Rvalue reference
    MemPool(const MemPool&&) = delete;
    // assignment operator deletion
    MemPool& operator=(const MemPool&) = delete;
    MemPool& operator=(const MemPool&&) = delete;

    template<typename... Args>
    T* Allocate(Args... args) noexcept{
        // gets pointer to memory location of next free block of memory in our memory pool
        auto obj_block = &(store_[next_free_index_]);
        // confirms that the memory is free
        ASSERT(obj_block->is_free_, "Expected free ObjectBlock at index: " + std::to_string(next_free_index_));
        // placement new to create new memory at 
        T* ret = new(&(obj_block->object_)) T(args...);
        obj_block->is_free_ = false;
        UpdateNextFreeIndex();
        return ret;
    }

    auto Deallocate(const T* elem) noexcept{
        // casting to ObjectBlock* which stores the memory address of our T Object
        // then taking the difference from the memory address of the start of the memory pool to find index
        const auto elem_index = (reinterpret_cast<const ObjectBlock*>(elem) - &store_[0]);
        ASSERT(elem_index >= 0 && static_cast<size_t>(elem_index) < store_.size(), "Element being deallocated does not belong to this memory");
        ASSERT(!store_[elem_index].is_free_, "Expected in-use ObjectBlock at index: " + std::to_string(elem_index));
        // deallocate by setting is_free_ to true and allowing future overwrite
        store_[elem_index].is_free_ = true;
    }
};

};