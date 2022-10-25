#program render_chunk_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"

layout(location = 0) in vec3 x;

smooth out vec2 screen_pos;

void main()
{
    gl_Position.xyz = x;
    gl_Position.w = 1.0;
    screen_pos = x.xy;
}

/////////////////////////////////////////////////////////////////

#shader GL_FRAGMENT_SHADER
#include "include/header.glsl"

layout(location = 0) out vec4 frag_color;

layout(location = 0) uniform mat3 camera_axes;
layout(location = 1) uniform vec3 camera_pos;
layout(location = 2) uniform sampler2D material_visual_properties;
layout(location = 3) uniform usampler3D materials;
layout(location = 4) uniform usampler3D active_regions;
layout(location = 5) uniform usampler3D occupied_regions;
layout(location = 6) uniform isampler3D body_materials;
layout(location = 7) uniform ivec3 size;
layout(location = 8) uniform ivec3 origin;
layout(location = 9) uniform sampler2D lightprobe_color;
layout(location = 10) uniform sampler2D lightprobe_depth;
layout(location = 11) uniform sampler2D lightprobe_x;
layout(location = 12) uniform sampler2D blue_noise_texture;
layout(location = 13) uniform int frame_number;
layout(location = 14) uniform int n_bodies;

// #define DEBUG_DOTS
#include "include/maths.glsl"
#include "include/blue_noise.glsl"
#include "include/body_data.glsl"
#include "include/materials.glsl"
#include "include/materials_physical.glsl"
#define ACTIVE_REGIONS
#include "include/raycast.glsl"
#include "include/lightprobe.glsl"

smooth in vec2 screen_pos;

uint rand(uint seed)
{
    seed ^= seed<<13;
    seed ^= seed>>17;
    seed ^= seed<<5;
    return seed;
}

// float float_noise(uint seed)
// {
//     return fract(float(int(seed))/1.0e9);
// }

float float_noise(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    float fov = pi*120.0/180.0;
    float screen_dist = 1.0/tan(fov/2);
    vec3 ray_dir = (16.0/9.0*screen_pos.x*camera_axes[0]
                    +        screen_pos.y*camera_axes[1]
                    -        screen_dist *camera_axes[2]);
    // vec3 ray_dir = (16.0/9.0*sin(screen_pos.x)*camera_axes[0]
    //                 +        sin(screen_pos.y)*camera_axes[1]
    //                 -        cos(screen_pos.x)*cos(screen_pos.y)*screen_dist *camera_axes[2]);
    ray_dir = normalize(ray_dir);

    frag_color = vec4(0,0,0,1);
    vec3 reflectivity = vec3(1,1,1);

    vec3 pos = camera_pos;
    vec3 ray_sign = sign(ray_dir);

    vec3 invabs_ray_dir = ray_sign/ray_dir;

    float epsilon = 0.02;
    int i = 0;
    float total_dist = 0;

    // // float prepass_jump_dist = max(texture(prepass_depth, 0.5f*screen_pos.xy+0.5f, 0).a-2.0f, 0.0f);
    // float prepass_jump_dist = 10*max_iterations;
    // for(int x = -1; x < 1; x++)
    // for(int y = -1; y < 1; y++)
    // {
    //     prepass_jump_dist = clamp(
    //         texture(prepass_depth, 0.5f*screen_pos.xy+0.5f+0.5f*vec2((x)/360.0f,(y)/180.0f), 0).a-4.0f,
    //         0.0f, prepass_jump_dist);
    // }
    // // {
    // //     frag_color.rgb = vec3(1.0f-prepass_jump_dist/200.0f);
    // //     return;
    // // }
    // pos += prepass_jump_dist*ray_dir;
    // total_dist += prepass_jump_dist;

    float bounding_jump_dist = 0.0;
    if(pos.x < 0 && ray_dir.x > 0)      bounding_jump_dist = max(bounding_jump_dist, -epsilon+(-pos.x)/(ray_dir.x));
    if(pos.x > size.x && ray_dir.x < 0) bounding_jump_dist = max(bounding_jump_dist, -epsilon+(size.x-pos.x)/(ray_dir.x));
    if(pos.y < 0 && ray_dir.y > 0)      bounding_jump_dist = max(bounding_jump_dist, -epsilon+(-pos.y)/(ray_dir.y));
    if(pos.y > size.y && ray_dir.y < 0) bounding_jump_dist = max(bounding_jump_dist, -epsilon+(size.y-pos.y)/(ray_dir.y));
    if(pos.z < 0 && ray_dir.z > 0)      bounding_jump_dist = max(bounding_jump_dist, -epsilon+(-pos.z)/(ray_dir.z));
    if(pos.z > size.z && ray_dir.z < 0) bounding_jump_dist = max(bounding_jump_dist, -epsilon+(size.z-pos.z)/(ray_dir.z));

    pos += bounding_jump_dist*ray_dir;
    total_dist += bounding_jump_dist;

    vec3 hit_pos;
    float hit_dist;
    ivec3 hit_cell;
    vec3 hit_dir;
    vec3 normal;
    uvec4 voxel;
    bool hit = cast_ray(materials, ray_dir, pos, size, origin, hit_pos, hit_dist, hit_cell, hit_dir, normal, voxel, 200);
    // bool hit = coarse_cast_ray(ray_dir, pos, hit_pos, hit_dist, hit_cell, hit_dir, normal);
    // voxel = texelFetch(materials, hit_cell, 0);

    for(int b = 0; b < n_bodies; b++)
    {
        vec3 body_x_cm = vec3(bodies[b].x_cm_x, bodies[b].x_cm_y, bodies[b].x_cm_z);
        vec3 body_x = vec3(bodies[b].x_x, bodies[b].x_y, bodies[b].x_z);
        vec4 body_orientation = vec4(bodies[b].orientation_r, bodies[b].orientation_x, bodies[b].orientation_y, bodies[b].orientation_z);
        ivec3 body_size = ivec3(bodies[b].size_x,
                                bodies[b].size_y,
                                bodies[b].size_z);
        ivec3 body_origin = ivec3(bodies[b].materials_origin_x,
                                  bodies[b].materials_origin_y,
                                  bodies[b].materials_origin_z);

        //ray info in the bodies frame
        vec3 body_pos = apply_rotation(conjugate(body_orientation), pos-body_x) + body_x_cm;
        vec3 body_ray_dir = apply_rotation(conjugate(body_orientation), ray_dir);

        float body_jump_dist = 0.0;
        if(body_pos.x < 0 && body_ray_dir.x > 0)      body_jump_dist = max(body_jump_dist, -epsilon+(-body_pos.x)/(body_ray_dir.x));
        if(body_pos.x > body_size.x && body_ray_dir.x < 0) body_jump_dist = max(body_jump_dist, -epsilon+(body_size.x-body_pos.x)/(body_ray_dir.x));
        if(body_pos.y < 0 && body_ray_dir.y > 0)      body_jump_dist = max(body_jump_dist, -epsilon+(-body_pos.y)/(body_ray_dir.y));
        if(body_pos.y > body_size.y && body_ray_dir.y < 0) body_jump_dist = max(body_jump_dist, -epsilon+(body_size.y-body_pos.y)/(body_ray_dir.y));
        if(body_pos.z < 0 && body_ray_dir.z > 0)      body_jump_dist = max(body_jump_dist, -epsilon+(-body_pos.z)/(body_ray_dir.z));
        if(body_pos.z > body_size.z && body_ray_dir.z < 0) body_jump_dist = max(body_jump_dist, -epsilon+(body_size.z-body_pos.z)/(body_ray_dir.z));

        body_pos += body_jump_dist*body_ray_dir;

        vec3 body_hit_pos;
        float body_hit_dist;
        ivec3 body_hit_cell;
        vec3 body_hit_dir;
        vec3 body_normal;
        uvec4 body_voxel;
        bool body_hit = cast_ray(body_materials, body_ray_dir, body_pos, body_size, body_origin, body_hit_pos, body_hit_dist, body_hit_cell, body_hit_dir, body_normal, body_voxel, 100);
        // if(body_hit && (!hit || body_jump_dist+body_hit_dist < hit_dist) && b != 13 && b != 12)
        if(body_hit && (!hit || body_jump_dist+body_hit_dist < hit_dist))
        {
            hit = true;
            hit_pos = apply_rotation(body_orientation, body_hit_pos-body_x_cm)+body_x;
            hit_dist = body_jump_dist+body_hit_dist;
            hit_cell = body_hit_cell;
            hit_dir = body_hit_dir;
            normal = apply_rotation(body_orientation, body_normal);
            voxel = body_voxel;
        }
    }

    total_dist += hit_dist;
    if(hit)
    {
        gl_FragDepth = 1.0f/total_dist;
        uint material_id = voxel.r;
        float roughness = get_roughness(material_id);
        vec3 emission = get_emission(material_id);

        frag_color.rgb += -(emission)*dot(normal, ray_dir);

        float n_probe_samples = 1;
        float n_ray_samples = 0;
        float n_samples = n_probe_samples+n_ray_samples;
        for(int samp = 0; samp < n_probe_samples; samp++)
        {
            vec3 reflection_normal = normal+0.5*roughness*(blue_noise(gl_FragCoord.xy/256.0+samp*vec2(0.82,0.34)).xyz-0.5f);
            // vec3 reflection_normal = normal;
            reflection_normal = normalize(reflection_normal);
            vec3 reflection_dir = ray_dir - 2*dot(ray_dir, reflection_normal)*reflection_normal;
            // vec3 reflection_dir = normal;
            vec2 sample_depth;
            frag_color.rgb += (1.0f/n_samples)*fr(material_id, reflection_dir, -ray_dir, normal)
                *sample_lightprobe_color(hit_pos, normal, vec_to_oct(reflection_dir), sample_depth);
        }
        // for(int samp = 0; samp < n_ray_samples; samp++)
        // {
        //     vec3 reflection_dir = ray_dir - 2*dot(ray_dir, normal)*normal;
        //     // reflection_dir += roughness*(blue_noise(gl_FragCoord.xy/256.0+(n_probe_samples*samp)*vec2(0.82,0.34)).xyz-0.5f);
        //     reflection_dir = normalize(reflection_dir);
        //     vec3 reflect_pos = hit_pos+0.5*reflection_dir;
        //     // bool hit = coarse_cast_ray(reflection_dir, reflect_pos, hit_pos, hit_dist, hit_cell, hit_dir, normal);
        //     bool hit = cast_ray(materials, reflection_dir, reflect_pos, size, origin, hit_pos, hit_dist, hit_cell, hit_dir, normal, voxel, 50);
        //     vec3 reflectance = fr(material_id, reflection_dir, -ray_dir, normal);
        //     if(hit)
        //     {
        //         // int material_id = texelFetch(materials, hit_cell, 0).r;
        //         int material_id = voxel.r;
        //         float roughness = get_roughness(material_id);
        //         vec3 emission = get_emission(material_id);

        //         frag_color.rgb += (1.0f/n_samples)*reflectance*(emission)*dot(normal, reflection_dir);

        //         vec2 sample_depth;

        //         vec3 reflection2_dir = ray_dir - 2*dot(ray_dir, normal)*normal;
        //         reflection2_dir += roughness*(blue_noise(gl_FragCoord.xy/256.0+samp*vec2(0.82,0.34)).xyz-0.5f);
        //         reflection2_dir = normalize(reflection2_dir);
        //         frag_color.rgb += (1.0f/n_samples)*reflectance
        //             *fr(material_id, reflection2_dir, -reflection_dir, normal)
        //             *sample_lightprobe_color(hit_pos, normal, vec_to_oct(reflection_dir), sample_depth);
        //     }
        //     else
        //     {
        //         vec2 sample_depth;
        //         frag_color.rgb += (1.0f/n_samples)*fr(material_id, reflection_dir, -ray_dir, normal)
        //             *sample_lightprobe_color(hit_pos, normal, vec_to_oct(reflection_dir), sample_depth);
        //     }
        // }
    }
    // else
    // {
    //     vec2 sample_depth;
    //     vec3 sample_color = sample_lightprobe_color(hit_pos, ray_dir, vec_to_oct(ray_dir), sample_depth);
    //     gl_FragDepth = 1.0f-sample_depth.r/(512.0f*sqrt(3));
    //     gl_FragDepth = clamp(gl_FragDepth, -1, 1);
    //     frag_color.rgb = sample_color;
    //     frag_color.a = 1.0f;
    // }

    // frag_color.rgb = mix(frag_color.rgb, sample_lightprobe_color(camera_pos, ray_dir, vec_to_oct(ray_dir)), 0.2);

    // frag_color.rgb = mix(vec3(1,0,0), vec3(0,0,1), n_texture_reads*1.0/20);
    // frag_color.rgb = mix(vec3(1,0,0), vec3(0,0,1), i*1.0/20);
    // frag_color.rgb *= clamp(1.0-1.0*total_dist/chunk_size, 0, 1);

    frag_color.rgb = clamp(frag_color.rgb, 0, 1);
}
