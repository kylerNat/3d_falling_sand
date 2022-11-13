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

//TODO: switch this to block allocation
#define max_dynamic_size (1*gigabyte/2)
struct memory_region
{
    memory_region* prev;
    memory_region* next;
    size_t size;
    uint32 free;
};

struct memory_manager
{
    memory_block * first;
    memory_block * current;

    memory_region* first_region;
};

// byte* dynamic_alloc(memory_region* first, size_t size)
// {
//     return (byte*) malloc(size);
// }

// void dynamic_free(void* memory)
// {
//     return free(memory);
// }

byte* dynamic_alloc(memory_region* first, size_t size)
{
    size = 4*((size+3)/4);
    // log_output("allocating ", size, " bytes\n");
    memory_region* region = first;
    for(;;)
    {
        if(region->free && region->size >= size)
        {
            const size_t min_size = 8;
            if(region->size >= size+sizeof(memory_region)+min_size)
            {
                memory_region* new_region = (memory_region*)(((byte*)region)+sizeof(memory_region)+size);
                *new_region = {
                    .prev = region,
                    .next = region->next,
                    .size = region->size-(size+sizeof(memory_region)),
                    .free = true,
                };
                region->next = new_region;
                region->size = size;
            }

            region->free = false;
            return (byte*)(region+1);
        }

        if(region->next) region = region->next;
        else
        {
            log_error("out of memory\n");
            return 0;
        }
    }
}

void dynamic_free(void* memory)
{
    memory_region* region = (memory_region*) ((byte*) memory-sizeof(memory_region));
    if(region->prev && region->prev->free)
    {
        if(region->next) region->next->prev = region->prev;
        region->prev->next = region->next;
        region->prev->size += region->size+sizeof(memory_region);
        region = region->prev;
    }
    if(region->next && region->next->free)
    {
        if(region->next->next) region->next->next->prev = region;
        region->size += region->next->size+sizeof(memory_region);
        region->next = region->next->next;
    }
    region->free = true;
}

//_copy added to help make sure this is only used when the memory inside needs to be copied exactly, if it does not, using dynamic_free and dynamic_alloc will probably be better
byte* dynamic_realloc_copy(memory_region* first, void* memory, size_t size)
{
    memory_region* region = (memory_region*) ((byte*) memory-sizeof(memory_region));
    size_t region_size = region->size;
    if(size < region_size-sizeof(memory_region))
    {
        memory_region* new_region = (memory_region*)(((byte*)region)+sizeof(memory_region)+size);
        *new_region = {
            .prev = region,
            .next = region->next,
            .size = region->size-(size+sizeof(memory_region)),
            .free = true,
        };
        region->next = new_region;
        region->size = size;
    }
    else if(size > region_size)
    {
        if(region->next && region->next->free && region->next->size+sizeof(memory_region)+region->size >= size)
        { //merge with next region
            if(region->next->size+region->size > size)
            {
                memory_region* new_region = (memory_region*)(((byte*)region)+sizeof(memory_region)+size);
                *new_region = {
                    .prev = region,
                    .next = region->next,
                    .size = region->size-(size+sizeof(memory_region)),
                    .free = true,
                };
                region->next = new_region;
            }
            else region->next = region->next->next;
            region->size = size;
        }
        else if(region->prev && region->prev->free && region->prev->size+sizeof(memory_region)+region->size >= size)
        { //merge with previous region
            memory_region* next = region->next;
            memory_region* prev = region->prev;
            memmove(region->prev+1, region+1, region->size);
            region = prev;

            if(region->prev->size+region->size > size)
            {
                memory_region* new_region = (memory_region*)(((byte*)region)+sizeof(memory_region)+size);
                *new_region = {
                    .prev = region,
                    .next = next,
                    .size = region->size+region->size-size,
                    .free = true,
                };
                region->next = new_region;
            }
            else region->next = next;
            region->size = size;
        }
        else if(region->next && region->prev && region->next->free && region->prev->free && region->next->size+region->prev->size+2*sizeof(memory_region)+region->size >= size)
        { //merge with both previous and next
            memory_region* next = region->next;
            memory_region* next_next = next->next;
            memory_region* prev = region->prev;
            memmove(region->prev+1, region+1, region->size);
            region = prev;

            if(region->prev->size+region->size+next->size+sizeof(memory_region) > size)
            {
                memory_region* new_region = (memory_region*)(((byte*)region)+sizeof(memory_region)+size);
                *new_region = {
                    .prev = region,
                    .next = next_next,
                    .size = region->prev->size+region->size+next->size+sizeof(memory_region)-size,
                    .free = true,
                };
                region->next = new_region;
            }
            else region->next = next_next;
            region->size = size;
        }
        else
        { //free and reallocate
            byte* new_memory = dynamic_alloc(first, size);
            memcpy(new_memory, memory, region->size);
            dynamic_free(memory);
            return new_memory;
        }
    }
    return (byte*)(region+1);
}

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
