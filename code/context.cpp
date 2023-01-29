#ifndef CONTEXT_IMPL
#define CONTEXT_IMPL

#include "context.h"

context_t* create_context()
{
    memory_stack* stack = allocate_new_stack(memory_stack_size);
    memory_manager* manager = (memory_manager*) (stack->memory + stack->used);
    stack->used += sizeof(memory_manager);
    manager->first = stack;
    manager->current = manager->first;
    manager->n_stallocs = 0;

    context_t* context = (context_t*) (stack->memory+stack->used);
    stack->used += sizeof(context_t);

    context->manager = manager;
    return context;
}

context_t* current_context;

//TODO: the purpose of doing things this way is so each thread can have its own context, haven't implemented that yet though
context_t * get_context()
{
    return current_context;
}

#endif //CONTEXT_IMPL
