#program simulate_chunk_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"

layout(location = 0) in vec2 r;
layout(location = 1) in vec2 X;

layout(location = 0) uniform int layer;
layout(location = 4) uniform usampler3D active_regions_in;
layout(location = 6) uniform writeonly uimage3D occupied_regions_out;

out vec3 p;

void main()
{
    int z = layer/16;
    int y = int(X.y);
    int x = int(X.x);

    p=vec3(16*x,16*y,layer);

    gl_Position = vec4(-2,-2,0,1);

    float scale = 2.0f/32.0f;

    uint region_active = texelFetch(active_regions_in, ivec3(x, y, z), 0).r;
    if(region_active != 0)
    {
        if(gl_VertexID == 0 && layer%16 == 0) imageStore(occupied_regions_out, ivec3(x,y,z), uvec4(0,0,0,0));
        gl_Position.xy = -1.0f+scale*(r+X);
    }
}

/////////////////////////////////////////////////////////////////

#shader GL_FRAGMENT_SHADER
#include "include/header.glsl"

layout(location = 0) out uvec4 out_voxel;

layout(location = 0) uniform int layer;
layout(location = 1) uniform int frame_number;
layout(location = 2) uniform sampler2D material_physical_properties;
layout(location = 3) uniform usampler3D materials;
layout(location = 5) uniform writeonly uimage3D active_regions_out;
layout(location = 6) uniform writeonly uimage3D occupied_regions_out;
layout(location = 7) uniform int update_cells;

#define MATERIAL_PHYSICAL_PROPERTIES
#include "include/materials_physical.glsl"

in vec3 p;

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

const int chunk_size = 256;

void main()
{
    float scale = 1.0/chunk_size;
    // ivec2 pos = ivec2(chunk_size*uv);
    // ivec2 pos = ivec2(gl_FragCoord.xy);
    ivec3 pos = ivec3(p);
    ivec3 cell_p;
    cell_p.xy = ivec2(gl_FragCoord.xy)%16;
    cell_p.z = pos.z%16;
    pos.xy += cell_p.xy;

    //+,0,-,0
    //0,+,0,-
    // int rot = (frame_number+layer)%4;
    uint rot = rand(rand(rand(frame_number)))%4;
    int i = 0;
    ivec2 dir = ivec2(((rot&1)*(2-rot)), (1-(rot&1))*(1-rot));

    uvec4 c  = texelFetch(materials, ivec3(pos.x, pos.y, pos.z),0);
    uvec4 u  = texelFetch(materials, ivec3(pos.x, pos.y, pos.z+1),0);
    uvec4 d  = texelFetch(materials, ivec3(pos.x, pos.y, pos.z-1),0);
    uvec4 r  = texelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z),0);
    uvec4 l  = texelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z),0);
    uvec4 dr = texelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z-1),0);
    uvec4 ur = texelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z+1),0);

    uvec4 f  = texelFetch(materials, ivec3(pos.x-dir.y, pos.y+dir.x, pos.z),0);
    uvec4 b  = texelFetch(materials, ivec3(pos.x+dir.y, pos.y-dir.x, pos.z),0);

    uvec4 ul = texelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z+1),0);
    uvec4 ll = texelFetch(materials, ivec3(pos.x-2*dir.x, pos.y-2*dir.y, pos.z),0);
    uvec4 dl = texelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z-1),0);

    out_voxel = c;
    if(update_cells == 1)
    {
        if(mat(c) == 0)
        { //central cell is empty

            //check if upper cell fell           , liquids and sands
            //then check if upper left cell fell , liquids and sands
            //then check if left cell flowed     , liquids
            if(mat(u) != 0 && phase(u) != phase_solid && transient(u)==0)
                out_voxel = u;
            else if(mat(l) != 0 && mat(ul) != 0 && phase(u) != phase_solid && phase(ul) != phase_solid && transient(ul)==0)
                out_voxel = ul;
            else if(mat(dl) != 0 && mat(d) != 0 && phase(l) == phase_liquid && mat(ll) != 0 && transient(l)==0)
                out_voxel = l;
            //might be better to multiply things by 0 instead of branching
        }
        else if(phase(c) != phase_solid && transient(c)==0)
        { //if the cell is not empty and not solid check if it fell or flew
            bool fall_allowed = (pos.z > 0 && (mat(d) == 0 || (mat(dr) == 0 && mat(r) == 0)));
            bool flow_allowed = (pos.z > 0 && mat(r) == 0 && mat(ur) == 0 && mat(u) == 0 && mat(l) != 0 && phase(c) > phase_sand);
            if(fall_allowed || flow_allowed) out_voxel = uvec4(0);
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
    if(hardness(mat(out_voxel)) == 0.0)                        phase = phase_sand;
    if(float(temp(out_voxel)) > melting_point(mat(out_voxel))) phase = phase_liquid;
    if(float(temp(out_voxel)) > boiling_point(mat(out_voxel))) phase = phase_gas;

    uint transient = transient(out_voxel);
    if(transient == 1)
    {
        out_voxel.r = 0;
        transient = 0;
    }

    uint temp = 8;

    out_voxel.g = uint(depth) | (phase << 5) | (transient << 7);

    out_voxel.b = temp;

    // bool changed = c.r != out_voxel.r || c.g != out_voxel.g || c.b != out_voxel.b;
    bool changed = c.r != out_voxel.r || c.g != out_voxel.g;
    if(changed)
    {
        imageStore(active_regions_out, ivec3(pos.x/16, pos.y/16, pos.z/16), uvec4(1,0,0,0));
        if(cell_p.x==15) imageStore(active_regions_out, ivec3(pos.x/16+1, pos.y/16, pos.z/16), uvec4(1,0,0,0));
        if(cell_p.x== 0) imageStore(active_regions_out, ivec3(pos.x/16-1, pos.y/16, pos.z/16), uvec4(1,0,0,0));
        if(cell_p.y==15) imageStore(active_regions_out, ivec3(pos.x/16, pos.y/16+1, pos.z/16), uvec4(1,0,0,0));
        if(cell_p.y== 0) imageStore(active_regions_out, ivec3(pos.x/16, pos.y/16-1, pos.z/16), uvec4(1,0,0,0));
        if(cell_p.z==15) imageStore(active_regions_out, ivec3(pos.x/16, pos.y/16, pos.z/16+1), uvec4(1,0,0,0));
        if(cell_p.z== 0) imageStore(active_regions_out, ivec3(pos.x/16, pos.y/16, pos.z/16-1), uvec4(1,0,0,0));
    }

    if(c.r > 0) imageStore(occupied_regions_out, ivec3(pos.x/16, pos.y/16, pos.z/16), uvec4(1,0,0,0));
}
