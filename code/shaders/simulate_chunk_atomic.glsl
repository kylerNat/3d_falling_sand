#program simulate_chunk_atomic_program
/////////////////////////////////////////////////////////////////

#shader GL_COMPUTE_SHADER
#include "include/header.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform sampler2D material_physical_properties;
layout(location = 2, r32ui) uniform uimage3D materials;
layout(location = 3) uniform usampler3D active_regions_in;
layout(location = 4) uniform writeonly uimage3D active_regions_out;
layout(location = 5) uniform writeonly uimage3D occupied_regions_out;
layout(location = 6) uniform int update_cells;

#define MATERIAL_PHYSICAL_PROPERTIES
#define UINT_PACKED_MATERIALS
#include "include/materials_physical.glsl"

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

void main()
{
    ivec3 pos = ivec3(gl_GlobalInvocationID.xyz);
    ivec3 cell_p = pos%16;
    ivec3 rpos = pos/16;
    // ivec3 cell_p = gl_LocalInvocationID.xyz;
    // ivec3 rpos = gl_WorkGroupID.xyz;

    uint region_active = texelFetch(active_regions_in, rpos, 0).r;
    if(region_active == 0)
    {
        return;
    }

    if(cell_p == ivec3(0,0,0)) imageStore(occupied_regions_out, rpos, uvec4(0,0,0,0));

    uint c = imageLoad(materials, pos).r;
    uint old_c = c;

    // if(update_cells == 1)
    // {
    //     if(phase(c) >= phase_sand)
    //     {
    //         if(imageAtomicCompSwap(materials, pos+ivec3(0,0,-1), 0, c) == 0) c=0;
    //         else if(imageAtomicCompSwap(materials, pos+ivec3(0,+1,-1), 0, c) == 0) c=0;
    //         else if(imageAtomicCompSwap(materials, pos+ivec3(+1,0,-1), 0, c) == 0) c=0;
    //         else if(imageAtomicCompSwap(materials, pos+ivec3(0,-1,-1), 0, c) == 0) c=0;
    //         else if(imageAtomicCompSwap(materials, pos+ivec3(-1,0,-1), 0, c) == 0) c=0;
    //         else if(phase(c)>=phase_liquid)
    //         {
    //             if(imageAtomicCompSwap(materials, pos+ivec3(0,+1,0), 0, c) == 0) c=0;
    //             else if(imageAtomicCompSwap(materials, pos+ivec3(+1,0,0), 0, c) == 0) c=0;
    //             else if(imageAtomicCompSwap(materials, pos+ivec3(0,-1,0), 0, c) == 0) c=0;
    //             else if(imageAtomicCompSwap(materials, pos+ivec3(-1,0,0), 0, c) == 0) c=0;
    //         }
    //     }
    // }

    uint r = imageLoad(materials, pos+ivec3(+1,0,0)).r;
    uint l = imageLoad(materials, pos+ivec3(-1,0,0)).r;
    uint f = imageLoad(materials, pos+ivec3(0,+1,0)).r;
    uint b = imageLoad(materials, pos+ivec3(0,-1,0)).r;
    uint u = imageLoad(materials, pos+ivec3(0,0,+1)).r;
    uint d = imageLoad(materials, pos+ivec3(0,0,-1)).r;

    if(phase(c) >= phase_sand)
    {
        if(mat(d) == 0)
        {
            imageAtomicExchange(materials, pos+ivec3(0,0,-1), c);
            c = 0;
        }
    }

    int depth = SURF_DEPTH;
    if(mat(c) != 0 && transient(c)==0)
    {
        if(mat(l) == 0 || transient(l) != 0 ||
           mat(r) == 0 || transient(r) != 0 ||
           mat(u) == 0 || transient(u) != 0 ||
           mat(d) == 0 || transient(d) != 0 ||
           mat(f) == 0 || transient(f) != 0 ||
           mat(b) == 0 || transient(b) != 0) depth = SURF_DEPTH;
        else
        {
            depth = MAX_DEPTH-1;
            depth = min(depth, depth(l)+1);
            depth = min(depth, depth(r)+1);
            depth = min(depth, depth(u)+1);
            depth = min(depth, depth(d)+1);
            depth = min(depth, depth(f)+1);
            depth = min(depth, depth(b)+1);
        }
    }
    else
    {
        depth = 0;
        depth = max(depth, depth(l)-1);
        depth = max(depth, depth(r)-1);
        depth = max(depth, depth(u)-1);
        depth = max(depth, depth(d)-1);
        depth = max(depth, depth(f)-1);
        depth = max(depth, depth(b)-1);
    }

    uint phase = phase_solid;
    if(hardness(mat(c)) == 0.0)                phase = phase_sand;
    if(float(temp(c)) > melting_point(mat(c))) phase = phase_liquid;
    if(float(temp(c)) > boiling_point(mat(c))) phase = phase_gas;

    uint transient = transient(c);

    uint temp = 8;

    c = mat(c) | (uint(depth)<<8) | (phase << 13) | (temp<<16);

    if(transient == 1)
    {
        c = 0;
    }

    bool changed = c != old_c;
    if(changed)
    {
        imageStore(active_regions_out, ivec3(rpos.x, rpos.y, rpos.z), uvec4(1,0,0,0));
        if(cell_p.x==15) imageStore(active_regions_out, ivec3(rpos.x+1, rpos.y, rpos.z), uvec4(1,0,0,0));
        if(cell_p.x== 0) imageStore(active_regions_out, ivec3(rpos.x-1, rpos.y, rpos.z), uvec4(1,0,0,0));
        if(cell_p.y==15) imageStore(active_regions_out, ivec3(rpos.x, rpos.y+1, rpos.z), uvec4(1,0,0,0));
        if(cell_p.y== 0) imageStore(active_regions_out, ivec3(rpos.x, rpos.y-1, rpos.z), uvec4(1,0,0,0));
        if(cell_p.z==15) imageStore(active_regions_out, ivec3(rpos.x, rpos.y, rpos.z+1), uvec4(1,0,0,0));
        if(cell_p.z== 0) imageStore(active_regions_out, ivec3(rpos.x, rpos.y, rpos.z-1), uvec4(1,0,0,0));
    }

    if(c != 0) imageStore(occupied_regions_out, ivec3(pos.x/16, pos.y/16, pos.z/16), uvec4(1,0,0,0));

    // memoryBarrier();
    imageAtomicCompSwap(materials, pos, old_c, c);
}
