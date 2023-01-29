#ifndef CONTEXT
#define CONTEXT

#include "memory.h"

struct context_t
{
    memory_manager* manager;
};

context_t * get_context();

#endif //CONTEXT
