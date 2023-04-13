#shader GL_COMPUTE_SHADER
#include "include/header.glsl"

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform sampler2D material_physical_properties;
layout(location = 2) uniform usampler3D materials;
layout(location = 3) uniform usampler3D occupied_regions;
layout(location = 4) uniform writeonly uimage3D active_regions_out;
layout(location = 5) uniform int n_beams;

#define MATERIAL_PHYSICAL_PROPERTIES
#include "include/materials_physical.glsl"

#include "include/beam_data.glsl"

#include "include/raycast.glsl"

uint rand(uint seed)
{
    seed ^= seed<<13;
    seed ^= seed>>17;
    seed ^= seed<<5;
    return seed;
}

void main()
{
    int b = int(gl_GlobalInvocationID.x);
    if(b >= n_beams) return;
    vec3 d = normalize(beam_d(b));

    vec3 hit_pos;
    float hit_dist;
    ivec3 hit_cell;
    vec3 hit_dir;
    vec3 normal;
    uvec4 voxel;
    ivec3 size = ivec3(512);
    ivec3 origin = ivec3(0);
    int medium = 0;
    int max_steps = int(3*beams[b].max_length);
    bool hit = cast_ray(materials, d, beam_x(b), size, origin, medium, true, hit_pos, hit_dist, hit_cell, hit_dir, normal, voxel, max_steps);

    float beam_len = min(hit_dist, beams[b].max_length);

    //wake up regions the beam passes through
    vec3 pos = beam_x(b);
    vec3 ray_dir = d;
    vec3 ray_sign = sign(ray_dir);
    vec3 invabs_ray_dir = ray_sign/ray_dir;
    vec3 cross_section_radius = beams[b].r*inversesqrt(1.0-sq(d));
    float epsilon = 0.02;
    for(int i = 0; i < 3*32; i++)
    {
        //wake up regions touched by square that bounds cross section
        ivec3 lower_region = ivec3((pos-cross_section_radius)/16.0);
        ivec3 upper_region = ivec3((pos+cross_section_radius)/16.0);
        for(int z = lower_region.z; z <= upper_region.z; z++)
            for(int y = lower_region.y; y <= upper_region.y; y++)
                for(int x = lower_region.x; x <= upper_region.x; x++)
                {
                    imageStore(active_regions_out, ivec3(x,y,z), uvec4(1,0,0,0));
                }

        vec3 dist = ((0.5f*ray_sign+0.5f)*16.0f+ray_sign*(16.0f*floor(pos*(1.0f/16.0f))-pos))*invabs_ray_dir;
        vec3 min_dir = step(dist.xyz, dist.zxy)*step(dist.xyz, dist.yzx);
        float min_dist = dot(dist, min_dir);
        hit_dir = min_dir;

        float step_dist = min_dist+epsilon;
        pos += step_dist*ray_dir;
    }

    d = beam_len*d;

    beams[b].dx = d.x;
    beams[b].dy = d.y;
    beams[b].dz = d.z;
}
