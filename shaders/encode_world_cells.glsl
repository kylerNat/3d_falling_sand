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

    uint current_runs_data = 0;
    uint n_runs = 0;

    uint current_material = texelFetch(materials, ivec3(x, y, 0), 0).r;
    uint run_length = 1;
    current_runs_data = current_material|(run_length<<8);
    int my_runs_size = 0;

    for(int z = 1; z < 512; z++)
    {
        uint material = texelFetch(materials, ivec3(x, y, z), 0).r;
        if(material != current_material || run_length >= 255)
        {
            uint run = current_material|(run_length<<8);

            if(n_runs++%2 == 0)
            {
                current_runs_data = run;
            }
            else
            {
                current_runs_data |= run<<16;
                data[runs_offset+column_id*256+my_runs_size++] = current_runs_data;
                current_runs_data = 0;
            }

            current_material = material;
            run_length = 1;
        }
        else
        {
            run_length++;
        }
    }
    if(run_length > 0 || current_runs_data != 0)
    {
        if(run_length > 0)
        {
            uint run = current_material|(run_length<<8);
            if(n_runs++%2 == 0)
                current_runs_data = run;
            else
                current_runs_data |= run<<16;
        }

        data[runs_offset+column_id*256+my_runs_size++] = current_runs_data;
    }

    runs_size[column_id] = -my_runs_size;
}
