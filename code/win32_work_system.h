#ifndef WIN32_WORK_SYSTEM
#define WIN32_WORK_SYSTEM

#define AUDIO_BUFFER_MAX_LEN (50)
#define AUDIO_BUFFER_MAX_SIZE (2*AUDIO_BUFFER_MAX_LEN)
struct worker_context
{
    int id;
    uint32 seed;
    int16* audio_buffer;
    memory_manager* manager;
};

struct work_task
{
    void (*func)(worker_context*, void*);
    void* data;
};

volatile work_task* work_stack;
volatile int n_work_entries; //number of unstarted tasks
volatile int n_remaining_tasks; //number of incomplete tasks
HANDLE work_semephore;
const int n_max_workers = 7;

worker_context* main_context;

worker_context* worker_context_list[8] = {};
int n_worker_contexts = 0;

bool pop_work(worker_context* context)
{
    // int work_index = InterlockedDecrement((volatile long*) &n_work_entries);
    int work_index = n_work_entries;
    work_task work;
    if(work_index > 0)
    {
        work.func = work_stack[work_index-1].func;
        work.data = work_stack[work_index-1].data;
        _ReadBarrier();
        if(work_index != InterlockedCompareExchange((volatile long*) &n_work_entries, work_index-1, work_index))
            return true;
        work.func(context, work.data);
        InterlockedDecrement((volatile long*) &n_remaining_tasks);
        return true;
    }
    return false;
}

worker_context* create_thread_context(int i)
{
    memory_block* block = allocate_new_block(block_size);
    memory_manager* worker_manager = (memory_manager*) (block->memory + block->used);
    block->used += sizeof(memory_manager);
    worker_manager->first = block;
    worker_manager->current = worker_manager->first;

    worker_context* context = (worker_context*) permalloc(worker_manager, sizeof(worker_context));
    worker_context_list[n_worker_contexts++] = context;
    context->audio_buffer = (int16*) permalloc(worker_manager, AUDIO_BUFFER_MAX_SIZE);
    context->manager = worker_manager;
    context->id = i;
    context->seed = i;
    randui(&context->seed);
    return context;
}

DWORD WINAPI thread_proc(void* param)
{
    worker_context* context = create_thread_context((int) param);

    for ever
    {
        if(!pop_work(context)) WaitForSingleObjectEx(work_semephore, -1, false);
    }
}

void push_work(void (*func)(worker_context*, void*), void* data)
{
    int work_index;
    _ReadWriteBarrier();
    do {
        work_index = n_work_entries;
        _ReadBarrier();
        work_stack[work_index].func = func;
        work_stack[work_index].data = data;
        _WriteBarrier();
    } while(work_index != InterlockedCompareExchange((volatile long*) &n_work_entries, work_index+1, work_index));
    InterlockedIncrement((volatile long*) &n_remaining_tasks);

    ReleaseSemaphore(work_semephore, 1, NULL);
}

void busy_work(int i, void* data)
{
    // log_output("thread ", i, ": ", (char*) data);
    printf("thread %i: %s", i, (char*) data);
}

#endif //WIN32_WORK_SYSTEM
