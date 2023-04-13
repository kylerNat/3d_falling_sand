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

    int sum = runs_data[sums_offset+range_id];

    //TODO: should divide work differently, since longer runs take longer
    for(uint i = end_run-1; i >= start_run; i--)
    {
        ivec3 start_pos = ivec3(sum/(512*512), (sum/512)%512, sum%512);
        if(sum < 0 || start_pos.x >= 512) break;

        uint run_data = uint(runs_data[i]);

        for(int j = 1; j >= 0; j--)
        {
            uint material = (run_data>>(16*j))&0xFF;
            uint run_length = (run_data>>(8+16*j))&0xFF;
            start_pos.z -= int(run_length);
            sum -= int(run_length);

            for(int z = 0; z < run_length; z++)
            {
                if(start_pos.z+z < 0 || start_pos.z+z >= 512) break;
                uint depth = min(min(z, run_length-1-z), MAX_DEPTH); //initialize depth with distance in z direction
                imageStore(materials_out, ivec3(start_pos.x, start_pos.y, start_pos.z+z), uvec4(material,(depth|(1<<5)),0,0));
            }
        }
    }
}
