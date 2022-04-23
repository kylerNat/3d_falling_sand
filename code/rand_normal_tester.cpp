#include <utils/logging.h>
#include <string.h>
#include <maths/maths.h>

int main(int n_args, char** args)
{
    union
    {
        uint32 ui;
        float32 u;
    };

    bool mistakes = false;
    for(uint32 i = 0; i < 0x800000; i++)
    {
        ui = (i) | 0x3F800000;
        u -= 1.0;
        float32 rand_n = sqrt(pi/8)*log(u/((1)-u));
        if(!isfinite(rand_n))
        {
            log_output("ui: ", ui, ", u: ", u, ", rand_n: ", rand_n, "\n");
            mistakes = true;
        }
    }
    if(!mistakes)
    {
        log_output("all good :)\n");
    }

    return 0;
}
