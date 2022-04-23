#include <utils/hashmap_common.h>
#include <utils/logging.h>
#include <string.h>

int main(int n_args, char** args)
{
    if(n_args < 2) log_error("need an input number\n");
    int key = atoi(args[1]);
    log_output(djb2(key));
    return 0;
}
