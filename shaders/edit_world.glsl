#program simulate_chunk_atomic_program
/////////////////////////////////////////////////////////////////

#shader GL_COMPUTE_SHADER
#include "include/header.glsl"

#define localgroup_size 8
#define subgroup_size 2
layout(local_size_x = localgroup_size, local_size_y = localgroup_size, local_size_z = localgroup_size) in;

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform sampler2D material_physical_properties;
layout(location = 2) uniform usampler3D materials;
layout(location = 3, rgba8ui) uniform uimage3D materials_out;
layout(location = 4) uniform usampler3D active_regions_in;
layout(location = 5) uniform writeonly uimage3D active_regions_out;
layout(location = 6) uniform writeonly uimage3D occupied_regions_out;
layout(location = 7) uniform int update_cells;
layout(location = 8) uniform int n_explosions;
layout(location = 9) uniform int n_beams;

#define MATERIAL_PHYSICAL_PROPERTIES
#include "include/materials_physical.glsl"

#include "include/particle_data.glsl"
#define EXPLOSION_DATA_BINDING 1
#include "include/explosion_data.glsl"
#define BEAM_DATA_BINDING 2
#include "include/beam_data.glsl"

uint rand(uint seed)
{
    seed ^= seed<<13;
    seed ^= seed>>17;
    seed ^= seed<<5;
    return seed;
}

float float_noise(uint seed)
{
    return fract(float(int(seed))/1.0e10);
}

uint conduction_dir(ivec3 pos)
{
    return rand(rand(frame_number+rand(rand(rand(pos.z)+pos.y)+pos.x)))%6;
}

bool c_active = false;
bool u_active = false;
bool d_active = false;
bool r_active = false;
bool l_active = false;
bool f_active = false;
bool b_active = false;

void simulate_voxel(ivec3 pos, ivec3 rpos)
{
    ivec3 cell_p = pos%16;
    // ivec3 cell_p = gl_LocalInvocationID.xyz;
    // ivec3 rpos = gl_WorkGroupID.xyz;

    if(cell_p == ivec3(0,0,0)) imageStore(occupied_regions_out, rpos, uvec4(0,0,0,0));

    uvec4 c  = texelFetch(materials, ivec3(pos.x, pos.y, pos.z),0);
    uvec4 old_voxel  = c;


    if(out_voxel.r != 0) imageStore(occupied_regions_out, ivec3(pos.x/16, pos.y/16, pos.z/16), uvec4(1,0,0,0));

    // memoryBarrier();
    imageStore(materials_out, pos, out_voxel);
}

void main()
{
    ivec3 pos = subgroup_size*ivec3(gl_GlobalInvocationID.xyz);
    ivec3 rpos = pos>>4;

    uint region_active = texelFetch(active_regions_in, rpos, 0).r;
    if(region_active == 0)
    {
        return;
    }

    for(int x = 0; x < subgroup_size; x++)
        for(int y = 0; y < subgroup_size; y++)
            for(int z = 0; z < subgroup_size; z++)
                simulate_voxel(pos+ivec3(x,y,z), rpos);

    if(c_active) imageStore(active_regions_out, ivec3(rpos.x, rpos.y, rpos.z),   uvec4(1,0,0,0));
    if(u_active) imageStore(active_regions_out, ivec3(rpos.x, rpos.y, rpos.z+1), uvec4(1,0,0,0));
    if(d_active) imageStore(active_regions_out, ivec3(rpos.x, rpos.y, rpos.z-1), uvec4(1,0,0,0));
    if(r_active) imageStore(active_regions_out, ivec3(rpos.x, rpos.y+1, rpos.z), uvec4(1,0,0,0));
    if(l_active) imageStore(active_regions_out, ivec3(rpos.x, rpos.y-1, rpos.z), uvec4(1,0,0,0));
    if(f_active) imageStore(active_regions_out, ivec3(rpos.x+1, rpos.y, rpos.z), uvec4(1,0,0,0));
    if(b_active) imageStore(active_regions_out, ivec3(rpos.x-1, rpos.y, rpos.z), uvec4(1,0,0,0));
}
