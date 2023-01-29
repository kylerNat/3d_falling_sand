#ifndef MEMORY
#define MEMORY

#include <utils/misc.h>
#include <stdio.h>

#define memory_stack_size (1*gigabyte/2)

struct memory_stack
{
    byte* memory;
    size_t available;
    size_t used;

    memory_stack* next;
};

struct stack_allocation
{
    void* data;
    memory_stack* stack;
};

#define N_MAX_STALLOCS 4096

struct memory_manager
{
    memory_stack * first;
    memory_stack * current;

    stack_allocation stallocs[N_MAX_STALLOCS];
    int n_stallocs;
};

memory_stack* allocate_new_stack(size_t size);

byte* stalloc(size_t size);

byte* stalloc_clear(size_t size);

void stunalloc(void * memory);

byte* dynamic_alloc(size_t size)
{
    return (byte*) malloc(size);
}

void dynamic_free(void * data)
{
    free(data);
}


#endif //MEMORY
