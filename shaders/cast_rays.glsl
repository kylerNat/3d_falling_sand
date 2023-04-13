#shader GL_COMPUTE_SHADER
#include "include/header.glsl"

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform sampler2D material_physical_properties;
layout(location = 2) uniform usampler3D materials;
layout(location = 3) uniform usampler3D occupied_regions;
layout(location = 4) uniform int n_rays;

#define MATERIAL_PHYSICAL_PROPERTIES
#include "include/materials_physical.glsl"

#define RAY_IN_DATA_BINDING 0
#define RAY_OUT_DATA_BINDING 1
#include "include/ray_data.glsl"

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
    int i = int(gl_GlobalInvocationID.x);
    if(i >= n_rays) return;
    vec3 d = normalize(ray_d(i));

    vec3 hit_pos;
    float hit_dist;
    ivec3 hit_cell;
    vec3 hit_dir;
    vec3 normal;
    uvec4 voxel = uvec4(-1,0,0,0);
    ivec3 size = ivec3(512);
    ivec3 origin = ivec3(0);
    int medium = rays_in[i].start_material;
    int max_steps = int(3*rays_in[i].max_dist);
    bool hit = cast_ray(materials, d, ray_x(i), size, origin, medium, true, hit_pos, hit_dist, hit_cell, hit_dir, normal, voxel, max_steps);

    float dist = min(hit_dist, rays_in[i].max_dist);

    rays_out[i].material = int(mat(voxel));
    rays_out[i].px = hit_cell.x;
    rays_out[i].py = hit_cell.y;
    rays_out[i].pz = hit_cell.z;
    rays_out[i].dist = dist;
}
