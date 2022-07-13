#program get_voxel_data_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"
layout(location = 0) in vec3 x;

smooth out vec2 uv;

void main()
{
    gl_Position.xyz = x;
    gl_Position.w = 1.0;
    uv = 0.5*x.xy+0.5;
}

/////////////////////////////////////////////////////////////////

#shader GL_FRAGMENT_SHADER
#include "include/header.glsl"

layout(location = 0) out ivec2 material;
layout(location = 1) out vec3 normal;

layout(location = 0) uniform isampler3D materials;
layout(location = 1) uniform ivec3 pos;

smooth in vec2 uv;

void main()
{
    ivec3 x = pos;

    vec3 gradient = vec3(
        texelFetch(materials, ivec3(x+vec3(1,0,0)), 0).g-texelFetch(materials, ivec3(x+vec3(-1,0,0)), 0).g,
        texelFetch(materials, ivec3(x+vec3(0,1,0)), 0).g-texelFetch(materials, ivec3(x+vec3(0,-1,0)), 0).g,
        texelFetch(materials, ivec3(x+vec3(0,0,1)), 0).g-texelFetch(materials, ivec3(x+vec3(0,0,-1)), 0).g+0.001f
        );
    gradient = normalize(gradient);

    material = texelFetch(materials, x, 0).rg;
    normal = gradient;
}
