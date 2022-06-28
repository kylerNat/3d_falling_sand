#program simulate_particles_program

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

layout(location = 0) out vec3 new_x;
layout(location = 1) out vec3 new_x_dot;

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform isampler3D materials;
layout(location = 2) uniform sampler2D old_x;
layout(location = 3) uniform sampler2D old_x_dot;

smooth in vec2 uv;

uint rand(uint seed)
{
    seed ^= seed<<13;
    seed ^= seed>>17;
    seed ^= seed<<5;
    return seed;
}

float float_noise(uint seed)
{
    return float(int(seed))/1.0e10;
}

const int chunk_size = 256;

ivec4 voxelFetch(isampler3D tex, ivec3 coord)
{
    return texelFetch(materials, coord, 0);
}

void main()
{
    float scale = 1.0/chunk_size;
    vec3 x = texture(old_x, vec2(0, 0)).rgb;
    vec3 x_dot = texture(old_x_dot, vec2(0, 0)).rgb;
    float epsilon = 0.001;
    // float epsilon = 0.02;

    vec3 x_ddot = vec3(0,0,-0.08);
    x_dot += x_ddot;

    float remaining_dist = length(x_dot);
    if(remaining_dist > epsilon)
    {
        x += x_dot;
        if(x.z < epsilon && x_dot.z < 0)
        {
            x.z = epsilon;
            x_dot.z = 0;
            // x_dot *= 0.99;
        }
        int max_iterations = 50;
        int i = 0;
        // while(voxelFetch(materials, ivec3(x)).r == 1 || voxelFetch(materials, ivec3(x)).r == 3)
        while(voxelFetch(materials, ivec3(x)).r != 0)
        {
            vec3 gradient = vec3(
                voxelFetch(materials, ivec3(x+vec3(1,0,0))).g-voxelFetch(materials, ivec3(x+vec3(-1,0,0))).g,
                voxelFetch(materials, ivec3(x+vec3(0,1,0))).g-voxelFetch(materials, ivec3(x+vec3(0,-1,0))).g,
                voxelFetch(materials, ivec3(x+vec3(0,0,1))).g-voxelFetch(materials, ivec3(x+vec3(0,0,-1))).g+0.001f
                );
            gradient = normalize(gradient);
            x += 0.05*gradient;
            float rej = dot(x_dot, gradient);
            x_dot -= gradient*rej;
            // x_dot *= 0.99;
            // x_dot += 1.0*gradient;
            if(++i > max_iterations) break;

            // x.z += 0.01;
            // x.z = ceil(x.z);
        }
        if(voxelFetch(materials, ivec3(x)).r == 2)
        {
            x_dot *= 0.95;
        }

        // if(hit_dir == 0) x.x -= epsilon*sign.x;
        // if(hit_dir == 1) x.y -= epsilon*sign.y;
        // if(hit_dir == 2) x.z -= epsilon*sign.z;
    }

    new_x = x;
    new_x_dot = x_dot;
}
