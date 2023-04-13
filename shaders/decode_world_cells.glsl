#shader GL_COMPUTE_SHADER
#include "include/header.glsl"

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

layout(location = 0, rgba8ui) uniform uimage3D materials_out;

#define MAX_DEPTH 16

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
    uint range_id = atomicAdd(next_range, 1);
    if(range_id >= total_runs_size) return;
    int preceeding_sum = 0;
    int my_sum = -runs_data[sums_offset+range_id];
    for(int i = int(range_id)-1; i >= 0; i--)
    {
        int previous_sum = runs_data[sums_offset+i];
        if(previous_sum <= 0)
        {
            preceeding_sum += -previous_sum;
        }
        else
        {
            preceeding_sum += previous_sum;
            break;
        }
    }
    atomicExchange(runs_data[sums_offset+range_id], preceeding_sum+my_sum);

    // uint n_invocations = gl_NumWorkGroups.x*gl_WorkGroupSize.x;
    // uint start_run = ((total_runs_size+n_invocations-1)/n_invocations)*range_id;
    // uint end_run = ((total_runs_size+n_invocations-1)/n_invocations)*(range_id+1);
    // end_run = min(end_run, total_runs_size);
    // if(total_runs_size < n_invocations)
    // {
    //     start_run = range_id;
    //     end_run = range_id+1;
    // }
    // //TODO: should divide work differently, since longer runs take longer
    // for(uint i = start_run; i < end_run; i++)
    // {
    //     ivec3 start_pos = ivec3(preceeding_sum/(512*512), (preceeding_sum/512)%512, preceeding_sum%512);
    //     if(start_pos.x >= 512) break;

    //     uint run_data = uint(runs_data[i]);

    //     for(int j = 0; j < 2; j++)
    //     {
    //         uint material = (run_data)&0xFF;
    //         uint run_length = (run_data>>8)&0xFF;
    //         run_data >>= 16;

    //         for(int z = 0; z < run_length; z++)
    //         {
    //             if(start_pos.z+z >= 512) break;
    //             uint depth = min(min(z, run_length-1-z), MAX_DEPTH); //initialize depth with distance in z direction
    //             imageStore(materials_out, ivec3(start_pos.x, start_pos.y, start_pos.z+z), uvec4(material,(depth|(1<<5)),0,0));
    //         }

    //         preceeding_sum += int(run_length);
    //         start_pos.z += int(run_length);
    //     }
    // }
}
