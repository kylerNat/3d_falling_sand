#program cast_probes_program

#shader GL_COMPUTE_SHADER
#include "include/header.glsl"

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform sampler2D material_visual_properties;
layout(location = 2) uniform usampler3D materials;
layout(location = 3) uniform usampler3D occupied_regions;
layout(location = 4) uniform usampler3D active_regions;
layout(location = 5) uniform sampler2D lightprobe_color;
layout(location = 6) uniform sampler2D lightprobe_depth;
layout(location = 7) uniform sampler2D lightprobe_x;
layout(location = 8) uniform sampler2D blue_noise_texture;

#include "include/maths.glsl"
#include "include/blue_noise.glsl"
#include "include/materials_visual.glsl"
#include "include/materials_physical.glsl"
#define ACTIVE_REGIONS
#include "include/raycast.glsl"
#include "include/lightprobe.glsl"
#include "include/probe_ray.glsl"

layout(local_size_x = rays_per_lightprobe, local_size_y = 1, local_size_z = 1) in;

uint rand(uint seed)
{
    seed ^= seed<<13;
    seed ^= seed>>17;
    seed ^= seed<<5;
    return seed;
}

void main()
{
    ivec2 probe_coord = ivec2(gl_WorkGroupID.xy);
    int probe_i = probe_coord.x+lightprobes_w*probe_coord.y;
    int sample_i = int(gl_LocalInvocationID.x);
    int ray_i = probe_i*rays_per_lightprobe+sample_i;

    ivec2 texel_coord = lightprobe_padded_resolution*probe_coord+1;
    ivec2 depth_texel_coord = lightprobe_depth_padded_resolution*probe_coord+1;

    // vec3 ray_dir = quasinoise_3(probe_id+lightprobes_w*lightprobes_h*(frame_number%100))-0.5;
    // vec3 ray_dir = blue_noise_3(vec2(probe_coord)/256.0)-0.5;
    // vec2 Omega = blue_noise_2(vec2(probe_coord)/256.0+sample_i*vec2(0.86, 0.24));
    // vec2 Omega = halton(frame_number*n_samples+sample_i);
    vec2 Omega = quasinoise_2(frame_number*rays_per_lightprobe+sample_i);
    float cosphi = cos((2*pi)*Omega.x);
    float sinphi = sin((2*pi)*Omega.x);
    float costheta = 2*Omega.y-1;
    float sintheta = sqrt(max(0, 1.0-sq(costheta)));
    vec3 ray_dir = vec3(sintheta*cosphi, sintheta*sinphi, costheta);
    ray_dir = normalize(ray_dir);
    vec3 original_ray_dir = ray_dir;

    vec3 probe_x = texelFetch(lightprobe_x, probe_coord, 0).xyz;

    vec3 hit_pos;
    float hit_dist;
    ivec3 hit_cell;
    vec3 hit_dir;
    vec3 normal;
    ivec3 origin = ivec3(0);
    ivec3 size = ivec3(512);
    uvec4 voxel;
    uint medium = texelFetch(materials, ivec3(probe_x), 0).r;
    if(opacity(medium) == 1) medium = 0;
    bool hit = cast_ray(materials, ray_dir, probe_x, size, origin, medium, true, hit_pos, hit_dist, hit_cell, hit_dir, normal, voxel, 200);

    vec3 color = vec3(0);

    float first_hit_dist = hit_dist;

    if(hit)
    {
        uvec4 voxel = texelFetch(materials, hit_cell, 0);
        uint material_id = voxel.r;

        vec4 transmission = vec4(1);
        vec4 transparent_color = vec4(0);
        // for(int i = 0; i < 5; i++)
        // {
        //     if(opacity(mat(voxel)) >= 1) break;
        //     vec3 ray_pos = hit_pos;
        //     if(ivec3(ray_pos) != hit_cell) ray_pos += 0.001*ray_dir;
        //     float c = dot(ray_dir, normal);
        //     float r = refractive_index(medium)/refractive_index(mat(voxel));
        //     float square = 1.0-sq(r)*(1.0-sq(c));
        //     if(square > 0) ray_dir = r*ray_dir+(-r*c+sign(c)*sqrt(square))*normal;
        //     else ray_dir = ray_dir - 2*c*normal; //total internal reflection
        //     // ray_dir = normalize(ray_dir);
        //     medium = mat(voxel);
        //     bool hit = cast_ray(materials, ray_dir, ray_pos, size, origin, medium, true, hit_pos, hit_dist, hit_cell, hit_dir, normal, voxel, 24);

        //     if(hit)
        //     {
        //         transmission *= exp(-opacity(mat(medium))*hit_dist);
        //     }
        //     else
        //     {
        //         break;
        //     }

        //     if(dot(transmission, transmission) < 0.001) break;

        //     uint material_id = voxel.r;
        //     float roughness = get_roughness(material_id);
        //     vec3 emission = get_emission(material_id);

        //     // transparent_color.rgb += -(emission)*dot(normal, ray_dir);
        //     transparent_color.rgb += emission;

        //     //TODO: actual blackbody color
        //     transparent_color.rgb += vec3(1,0.05,0.1)*clamp((1.0/127.0)*(float(temp(voxel))-128), 0.0, 1.0);

        //     transparent_color.rgb += vec3(0.7,0.3,1.0)*clamp((1.0/15.0)*(float(volt(voxel))), 0.0, 1.0);

        //     vec3 reflection_dir = normal;
        //     vec2 sample_depth;
        //     transparent_color.rgb += fr(material_id, reflection_dir, -ray_dir, normal)
        //         *sample_lightprobe_color(hit_pos, normal, vec_to_oct(reflection_dir), sample_depth);
        //     transparent_color *= transmission;
        // }

        color += transparent_color.rgb;

        float roughness = get_roughness(material_id);
        vec3 emission = get_emission(material_id);

        emission += vec3(1,0.05,0.1)*clamp((1.0/127.0)*(float(temp(voxel))-128), 0.0, 1.0);

        // color += -(emission)*dot(normal, ray_dir);
        color += emission;

        // float total_weight = 0.0;

        // vec3 reflection_normal = normal+0.5*roughness*(blue_noise(gl_FragCoord.xy/256.0+vec2(0.82,0.34)).xyz-0.5f);
        vec3 reflection_normal = normal;
        reflection_normal = normalize(reflection_normal);
        // vec3 reflection_dir = ray_dir-(2*dot(ray_dir, reflection_normal))*reflection_normal;
        vec3 reflection_dir = normal;
        // reflection_dir += roughness*blue_noise(gl_FragCoord.xy/256.0f+vec2(0.4f,0.6f)).xyz;

        vec2 sample_depth;
        color += fr(material_id, reflection_dir, -ray_dir, normal)*sample_lightprobe_color(hit_pos, normal, vec_to_oct(normal), sample_depth);
    }
    else
    {
        vec2 sample_depth;
        vec3 sample_color = sample_lightprobe_color(hit_pos, ray_dir, vec_to_oct(ray_dir), sample_depth);
        first_hit_dist += sample_depth.r;
        color.rgb = sample_color;
    }

    // color = clamp(color, 0, 1);
    color = max(color, 0);

    probe_rays[ray_i].rel_hit_pos = first_hit_dist*original_ray_dir;
    probe_rays[ray_i].hit_color = color;
}
