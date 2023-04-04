#ifndef MEMORY_IMPL
#define MEMORY_IMPL

#include "memory.h"
#include "context.h"

memory_stack* allocate_new_stack(size_t size)
{
    memory_stack* new_stack = (memory_stack*) platform_big_alloc(size);
    new_stack->memory = (byte*) new_stack;
    new_stack->available = memory_stack_size;
    new_stack->used = sizeof(memory_stack);
    new_stack->next = 0;
    return new_stack;
}

byte* stalloc(size_t size)
{
    memory_manager* manager = get_context()->manager;

    if(manager->current->used+size > manager->current->available)
    {
        assert(memory_stack_size >= size, "attempted to allocate ", size, "bytes. stalloc can only allocate up to ", memory_stack_size, "bytes");
        if(!manager->current->next) manager->current->next = allocate_new_stack(memory_stack_size);
        manager->current = manager->current->next;
    }
    byte* out = manager->current->memory + manager->current->used;
    manager->current->used += size;
    assert(manager->n_stallocs < N_MAX_STALLOCS);
    manager->stallocs[manager->n_stallocs++] = {out, manager->current};
    return out;
}

byte* stalloc_clear(size_t size)
{
    memory_manager* manager = get_context()->manager;

    byte* out = stalloc(size);
    memset(out, 0, size);
    return out;
}

void stunalloc(void * memory)
{
    memory_manager *manager = get_context()->manager;
    assert(manager->n_stallocs > 0 && memory == manager->stallocs[manager->n_stallocs-1].data, "attempted to stunalloc memory that is not stack allocated, or that is on top of the stack");
    manager->current = manager->stallocs[--manager->n_stallocs].stack;
    manager->current->used = (byte*) memory - manager->current->memory;
}

#endif //MEMORY_IMPL
