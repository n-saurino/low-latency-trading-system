#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "macros.h"

namespace Common {

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
};

};