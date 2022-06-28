#program update_lightmap_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"

layout(location = 0) uniform int frame_number;
layout(location = 6) uniform sampler2D blue_noise_texture;

#include "include/blue_noise.glsl"

smooth out vec2 sample_oct;

void main()
{
    vec2 sample_coord = blue_noise(vec2(gl_InstanceID/16.0f, 0)).xy;
    gl_Position.xy = 2.0f*sample_coord-1.0f;
    gl_Position.z = 0.0f;
    gl_Position.w = 1.0f;

    sample_oct = 2.0f*fract(gl_Position.xy*vec2(256, 128))-1.0f;
}

/////////////////////////////////////////////////////////////////

#shader GL_FRAGMENT_SHADER
#include "include/header.glsl"

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 depth;

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform isampler3D materials;
layout(location = 2) uniform usampler3D occupied_regions;
layout(location = 3) uniform sampler2D lightprobe_color;
layout(location = 4) uniform sampler2D lightprobe_depth;
layout(location = 5) uniform sampler2D lightprobe_x;
layout(location = 6) uniform sampler2D blue_noise_texture;

ivec3 size = ivec3(512);
ivec3 origin = ivec3(0);

#include "include/raycast.glsl"
#include "include/blue_noise.glsl"
#include "include/lightprobe.glsl"

smooth in vec2 sample_oct;

void main()
{
    //really all this ray casting could happen in the vertex shader, not sure if there's an advantage either way
    ivec2 probe_coord = ivec2(gl_FragCoord.xy/16);
    int probe_index = probe_coord.x+probe_coord.y*256;
    ivec3 probe_pos = ivec3(probe_index%32, (probe_index/32)%32, probe_index/(32*32));

    vec3 ray_dir = oct_to_vec(sample_oct);
    vec3 pos = texelFetch(lightprobe_x, probe_coord, 0).xyz;

    vec3 hit_pos;
    float hit_dist;
    ivec3 hit_cell;
    vec3 hit_dir;
    vec3 normal;
    cast_ray(ray_dir, pos, hit_pos, hit_dist, hit_cell, hit_dir, normal);

    ivec4 voxel = texelFetch(materials, hit_cell, 0);

    const float decay_fraction = 1.0/60.0; //average over the order 1/decay_fraction samples

    depth.r = hit_dist;
    depth.g = sq(hit_dist);
    depth.a = decay_fraction;
    //TODO: this arbitrarily gives later samples in the same frame more weight

    vec3 emission = vec3(0.0f);
    if(voxel.r == 2)
    {
        emission = vec3(0.05,0.05,0.1);
    }

    vec3 reflectivity = vec3(0.5, 0.5, 0.5);
    float roughness = 0.0;
    if(voxel.r == 1)
    {
        reflectivity = vec3(0.54,0.44,0.21);
        roughness = 0.9;
    }
    else if(voxel.r == 3)
    {
        reflectivity = vec3(0.5,0.5,1.0);
        roughness = 0.1;
    }
    else
    {
        reflectivity = vec3(0.1,0.1,0.2);
        roughness = 0.1;
    }

    color.rgb = vec3(0);
    color.rgb += emission;

    float total_weight = 0.0;

    vec3 reflection_dir = ray_dir-(2*dot(ray_dir, normal))*normal;

    color.rgb += reflectivity*sample_lightprobe_color(hit_pos, normal, vec_to_oct(reflection_dir));
    color.a = decay_fraction;
}
