#pragma once

#include "Core/Types.h"
#include <vector>
#include <memory>
#include <cstddef>

namespace Utils {

// Memory pool for efficient allocation
template<typename T>
class MemoryPool {
public:
    MemoryPool(size_t initial_size = 1024, size_t growth_factor = 2);
    ~MemoryPool();
    
    // Allocate an object
    T* Allocate();
    
    // Deallocate an object
    void Deallocate(T* ptr);
    
    // Reset pool (mark all as free)
    void Reset();
    
    // Get pool size
    size_t GetSize() const { return pool_size_; }
    size_t GetUsed() const { return used_count_; }
    
private:
    struct Block {
        alignas(T) char data[sizeof(T)];
        bool in_use;
    };
    
    std::vector<std::unique_ptr<Block[]>> pools_;
    size_t pool_size_;
    size_t growth_factor_;
    size_t current_pool_index_;
    size_t current_pool_offset_;
    size_t used_count_;
    
    void GrowPool();
    Block* FindFreeBlock();
};

} // namespace Utils




