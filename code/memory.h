#ifndef MEMORY
#define MEMORY

#include <utils/misc.h>
#include <stdio.h>

#define block_size (1*gigabyte/2)
struct memory_block
{
    byte* memory;
    size_t available;
    size_t used;

    bool is_reserved; //set if the block is being used temporarily

    memory_block* next;
};

struct memory_manager
{
    memory_block * first;
    memory_block * current;
};

memory_block* allocate_new_block(size_t size)
{
    memory_block* new_block = (memory_block*) platform_big_alloc(size);
    new_block->memory = (byte*) new_block;
    new_block->available = block_size;
    new_block->used = sizeof(memory_block);
    new_block->next = 0;
    return new_block;
}

byte* permalloc(memory_manager* manager, size_t size)
{
    assert(!manager->current->is_reserved, "tried to allocate permanent memory while current memory block is reserved");
    if(manager->current->used+size > manager->current->available)
    {
        assert(block_size >= size);
        manager->current->next = allocate_new_block(block_size);
        manager->current = manager->current->next;
    }
    byte* out = manager->current->memory + manager->current->used;
    manager->current->used += size;
    return out;
}

byte* permalloc_clear(memory_manager* manager, size_t size)
{
    byte* out = permalloc(manager, size);
    memset(out, 0, size);
    return out;
}

void allocate_reserved(memory_manager* manager, size_t size)
{
    assert(!manager->current->is_reserved, "tried to allocate permanent memory while current memory block is reserved");
    manager->current->used += size;
    assert(manager->current->used <= manager->current->available, "overflowed reserved block");
}

//reserves a block for temporary use with at least size bytes unused
byte* reserve_block(memory_manager* manager, size_t size)
{
    assert(!manager->current->is_reserved, "tried to reserve block that is already reserved");
    if(manager->current->used+size > manager->current->available)
    {
        assert(block_size >= size);
        manager->current->next = allocate_new_block(block_size);
        manager->current = manager->current->next;
    }
    manager->current->is_reserved = true;
    return manager->current->memory + manager->current->used;
}

size_t current_block_unused(memory_manager* manager)
{
    return manager->current->available - manager->current->used;
}

void unreserve_block(memory_manager* manager)
{
    assert(manager->current->is_reserved, "tried to unreserve block that is not reserved");
    manager->current->is_reserved = false;
}

#endif //MEMORY
