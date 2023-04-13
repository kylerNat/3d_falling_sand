#shader GL_COMPUTE_SHADER
#include "include/header.glsl"

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

layout(location = 0, rgba8ui) uniform uimage3D materials_out;

layout(std430, binding = 0) buffer run_data
{
    int next_range;
    int runs_size[512*512-1];
    int total_runs_size;
    int runs_data[];
};

#define sums_offset (256*512*512)

void main()
{
    uint range_id = gl_GlobalInvocationID.x;
    if(range_id >= total_runs_size) return;
    uint n_invocations = gl_NumWorkGroups.x*gl_WorkGroupSize.x;
    uint start_run = ((total_runs_size+n_invocations-1)/n_invocations)*range_id;
    uint end_run = ((total_runs_size+n_invocations-1)/n_invocations)*(range_id+1);
    end_run = min(end_run, total_runs_size);
    if(total_runs_size < n_invocations)
    {
        start_run = range_id;
        end_run = range_id+1;
    }
    int current_sum = 0;
    for(uint i = start_run; i < end_run; i++)
    {
        current_sum += int((uint(runs_data[i])>>8)&0xFF);
        current_sum += int((uint(runs_data[i])>>24)&0xFF);
    }
    runs_data[sums_offset+range_id] = -current_sum;
}
