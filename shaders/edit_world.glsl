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

#define MATERIAL_PHYSICAL_PROPERTIES
#include "include/materials_physical.glsl"

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

    uvec4 c  = texelFetch(materials, ivec3(pos.x, pos.y, pos.z),0);
    uvec4 old_voxel = c;

    ivec3 pu = ivec3(pos.x, pos.y, pos.z+1);
    ivec3 pd = ivec3(pos.x, pos.y, pos.z-1);
    ivec3 pr = ivec3(pos.x+1, pos.y, pos.z);
    ivec3 pl = ivec3(pos.x-1, pos.y, pos.z);
    ivec3 pf = ivec3(pos.x, pos.y+1, pos.z);
    ivec3 pb = ivec3(pos.x, pos.y-1, pos.z);

    uvec4 u  = texelFetch(materials, pu, 0);
    uvec4 d  = texelFetch(materials, pd, 0);
    uvec4 r  = texelFetch(materials, pr, 0);
    uvec4 l  = texelFetch(materials, pl, 0);
    uvec4 f  = texelFetch(materials, pf, 0);
    uvec4 b  = texelFetch(materials, pb, 0);

    uvec4 out_voxel = c;

    int depth = MAX_DEPTH-1;
    uint solidity = solidity(c);
    if((solidity(u) != solidity) ||
       (solidity(d) != solidity) ||
       (solidity(r) != solidity) ||
       (solidity(l) != solidity) ||
       (solidity(f) != solidity) ||
       (solidity(b) != solidity)) depth = 0;
    else
    {
        depth = min(depth, depth(u)+1);
        depth = min(depth, depth(d)+1);
        depth = min(depth, depth(r)+1);
        depth = min(depth, depth(l)+1);
        depth = min(depth, depth(f)+1);
        depth = min(depth, depth(b)+1);
    }

    out_voxel.g = uint(depth) | 1<<5;
    out_voxel.a = 0;

    bool changed = old_voxel != out_voxel;
    if(changed)
    {
        c_active = true;
        u_active = u_active || cell_p.z>=15;
        d_active = d_active || cell_p.z<= 0;
        r_active = r_active || cell_p.y>=15;
        l_active = l_active || cell_p.y<= 0;
        f_active = f_active || cell_p.x>=15;
        b_active = b_active || cell_p.x<= 0;
    }

    if(out_voxel.r != 0) imageStore(occupied_regions_out, ivec3(pos.x/16, pos.y/16, pos.z/16), uvec4(1,0,0,0));

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

    if(gl_LocalInvocationID == uvec3(0,0,0)) imageStore(occupied_regions_out, rpos, uvec4(0,0,0,0));
    memoryBarrier();

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
