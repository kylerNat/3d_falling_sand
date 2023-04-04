#shader GL_COMPUTE_SHADER
#include "include/header.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(location = 0) uniform sampler2D material_physical_properties;
layout(location = 1) uniform usampler3D materials;

layout(std430, binding = 0) buffer run_data
{
    int next_column_id;
    int runs_size[512*512]; //negative values represent the value for this column, positive values represent this column and all before it
    uint data[256*512*512];
    uint runs[256*512*512];
};

#define MATERIAL_PHYSICAL_PROPERTIES
#include "include/materials_physical.glsl"

void main()
{
    int column_id = atomicAdd(next_column_id, 1);
    int x = column_id%512;
    int y = column_id/512;
    if(y >= 512) return;

    uint current_runs_data = 0;
    uint n_runs = 0;

    uint current_material = mat(texelFetch(materials, ivec3(x, y, 0), 0));
    uint run_length = 1;
    current_runs_data = current_material|(run_length<<8);

    for(int z = 1; z < 512; z++)
    {
        uvec4 c = texelFetch(materials, ivec3(x, y, z), 0);
        if(mat(c) != current_material)
        {
            uint run = current_material|(run_length<<8);

            if(n_runs++%2 == 1)
            {
                current_runs_data |= run;
            }
            else
            {
                current_runs_data |= run<<16;
                runs[column_id*256+runs_size[column_id]++] = current_runs_data;
            }

            current_material = mat(c);
            run_length = 0;
        }
        else
        {
            run_length++;
        }
    }
    if(run_length > 0 || current_runs_data != 0)
    {
        runs[column_id*256+runs_size[column_id]++] = current_runs_data;
    }

    // if(column_id == 1) n_runs[column_id] = 421;
    // else n_runs[column_id] = 1;
    runs_size[column_id] = -int((n_runs+1)>>1);
}
