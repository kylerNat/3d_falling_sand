#shader GL_COMPUTE_SHADER
#include "include/header.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(location = 0) uniform usampler3D materials;

layout(std430, binding = 0) buffer run_data
{
    int next_column_id;
    int runs_size[512*512]; //negative values represent the value for this column, positive values represent this column and all before it
    uint data[];
};

#define runs_offset (256*512*512)

void main()
{
    int column_id = atomicAdd(next_column_id, 1);
    int x = column_id/512;
    int y = column_id%512;
    if(y >= 512) return;

    int my_runs_size = -runs_size[column_id];

    uint preceeding_runs_size = 0;

    for(int i = column_id-1; i >= 0; i--)
    {
        int column_runs = runs_size[i];
        if(column_runs <= 0)
        {
            preceeding_runs_size -= column_runs;
        }
        else
        {
            preceeding_runs_size += column_runs;
            break;
        }
    }
    atomicExchange(runs_size[column_id], int(preceeding_runs_size+my_runs_size));
    for(int i = 0; i < my_runs_size; i++)
    {
        data[preceeding_runs_size+i] = data[runs_offset+256*column_id+i];
    }
}
