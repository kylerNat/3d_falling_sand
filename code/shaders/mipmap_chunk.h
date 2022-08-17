#program mipmap_chunk_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"

layout(location = 0) in vec2 r;
layout(location = 1) in vec2 X;

layout(location = 0) uniform int layer;
layout(location = 1) uniform int mip_level;
layout(location = 3) uniform usampler3D active_regions_in;

out vec3 p;

void main()
{
    float mip_scale = pow(0.5, mip_level-1);
    int z = layer/(16>>mip_level);
    int y = int(X.y);
    int x = int(X.x);

    p=vec3(16*mip_scale*x,16*mip_scale*y,layer*pow(2, mip_level));

    gl_Position = vec4(0.0/0.0, 0.0/0.0, 0, 1);

    float scale = 2.0f/32.0f;

    uint region_active = texelFetch(active_regions_in, ivec3(x, y, z), 0).r;
    if(region_active != 0)
    {
        gl_Position.xy = -1.0f+scale*(r+X);
    }
}

/////////////////////////////////////////////////////////////////

#shader GL_FRAGMENT_SHADER
#include "include/header.glsl"

layout(location = 0) out ivec4 frag_color;

layout(location = 0) uniform int layer;
layout(location = 1) uniform int mip_level;
layout(location = 2) uniform isampler3D materials;

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
    float mip_scale = pow(2.0, mip_level);
    ivec3 pos = ivec3(p);
    ivec3 cell_p;
    cell_p.xy = ivec2(gl_FragCoord.xy*mip_scale)%int(32/mip_scale);
    pos.xy += cell_p.xy;

    frag_color.rg = ivec2(0, 16);
    for(int z = 0; z <= 1; z++)
        for(int y = 0; y <= 1; y++)
            for(int x = 0; x <= 1; x++)
            {
                ivec4 voxel  = texelFetch(materials, ivec3(pos.x+x, pos.y+y, pos.z+z), mip_level-1);
                frag_color.r = max(frag_color.r, voxel.r);
                // if(voxel.g > 1)
                // {
                //     frag_color.g = voxel.g-1;
                //     break;
                // }
                frag_color.g = min(frag_color.g, voxel.g);
            }
    frag_color.r = pos.z;
    frag_color.a = 1;
}
