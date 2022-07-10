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
layout(location = 2) uniform isampler3D materials;
layout(location = 3) uniform usampler3D occupied_regions;
layout(location = 4) uniform ivec3 size;
layout(location = 5) uniform ivec3 origin;
layout(location = 6) uniform sampler2D lightprobe_color;
layout(location = 7) uniform sampler2D lightprobe_depth;
layout(location = 8) uniform sampler2D lightprobe_x;
layout(location = 9) uniform sampler2D blue_noise_texture;
layout(location = 10) uniform int frame_number;

// #define DEBUG_DOTS
#include "include/blue_noise.glsl"
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

    ivec3 ipos = ivec3(floor(pos));

    vec3 hit_pos;
    float hit_dist;
    ivec3 hit_cell;
    vec3 hit_dir;
    vec3 normal;
    bool hit = cast_ray(ray_dir, pos, hit_pos, hit_dist, hit_cell, hit_dir, normal);

    total_dist += hit_dist;
    if(hit)
    {
        vec4 voxel = texelFetch(materials, hit_cell, 0);

        float roughness = 0.0f;
        vec3 emission = vec3(0.001f);
        if(voxel.r == 2)
        {
            // emission = vec3(0.4,0.4,0.8);
            emission = vec3(0.8,0.2,0.2)*100;
        }
        if(voxel.r == 1)
        {
            emission = vec3(0.54,0.44,0.21);
        }
        // if(voxel.r == 3)
        // {
        //     emission = vec3(0.5,0.125,0.5);
        // }

        // emission *= 1.0f-total_dist/(512.0f*sqrt(3));

        if(voxel.r == 1)
        {
            reflectivity *= vec3(0.54,0.44,0.21);
            roughness = 0.9;
        }
        else if(voxel.r == 3)
        {
            reflectivity *= vec3(0.5,0.5,1.0);
            roughness = 0.5;
        }
        else
        {
            reflectivity *= vec3(0.1,0.1,0.2);
            roughness = 0.1;
        }

        ivec3 ioutside = ipos - ivec3(hit_dir*ray_sign);
        normal = normalize(normal);

        // frag_color.rgb += reflectivity*emission*(0.9f+0.1f*normal)*dot(normal, gradient);
        // frag_color.rgb += reflectivity*emission*(0.9f+0.1f*normal);
        frag_color.rgb += emission;

        gl_FragDepth = 1.0f-total_dist/(512.0f*sqrt(3));

        reflectivity *= -dot(ray_dir, normal);

        float n_samples = 1;
        for(int samp = 0; samp < n_samples; samp++)
        {
            vec3 reflection_dir = ray_dir - 2*dot(ray_dir, normal)*normal;
            reflection_dir += roughness*(blue_noise(gl_FragCoord.xy/256.0+samp*vec2(0.82,0.34)).xyz-0.5f);
            reflection_dir = normalize(reflection_dir);
            // vec3 reflection_dir = normal;
            vec2 sample_depth;
            frag_color.rgb += (1.0f/n_samples)*reflectivity*sample_lightprobe_color(hit_pos, normal, vec_to_oct(reflection_dir), sample_depth);
        }
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
