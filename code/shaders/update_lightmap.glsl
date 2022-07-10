#program update_lightmap_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"

layout(location = 0) in vec2 x;
layout(location = 1) in vec2 X;

layout(location = 0) uniform int frame_number;
layout(location = 3) uniform sampler2D lightprobe_color;
layout(location = 4) uniform sampler2D lightprobe_depth;
layout(location = 5) uniform sampler2D lightprobe_x;
layout(location = 6) uniform sampler2D blue_noise_texture;

#include "include/blue_noise.glsl"
#include "include/lightprobe.glsl"

smooth out vec2 sample_oct;

void main()
{
    // vec2 sample_coord = blue_noise(vec2(gl_InstanceID*PHI/256.0f, 0)).xy;
    // gl_Position.xy = 2.0f*sample_coord-1.0f;
    // gl_Position.z = 0.0f;

    // sample_oct = 2.0f*fract(gl_Position.xy*vec2(256, 128))-1.0f;

    vec2 scale = 2.0f/vec2(lightprobes_w, lightprobes_h);
    vec2 scale2 = (0.5f*scale*lightprobe_resolution)/lightprobe_padded_resolution;

    gl_Position.xy = scale*(X+0.5)-1+scale2*x;
    gl_Position.z = 0.0f;
    gl_Position.w = 1.0f;
    sample_oct = x;
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

layout(pixel_center_integer) in vec4 gl_FragCoord;

void main()
{
    // vec2 sample_oct = 2*fract((gl_FragCoord.xy-1)*(1.0f/lightprobe_padded_resolution)
    //                           +(blue_noise(gl_FragCoord.xy/256.0).xy)*(1.0f/lightprobe_resolution))-1;
    // vec2 sample_oct = 2*fract((gl_FragCoord.xy+0.5f)*(1.0f/lightprobe_resolution))-1;

    //really all this ray casting could happen in the vertex shader, not sure if there's an advantage either way
    ivec2 probe_coord = ivec2(gl_FragCoord.xy/lightprobe_padded_resolution);
    int probe_index = probe_coord.x+probe_coord.y*lightprobes_w;
    ivec3 probe_pos = ivec3(probe_index%lightprobes_per_axis, (probe_index/lightprobes_per_axis)%lightprobes_per_axis, probe_index/(lightprobes_per_axis*lightprobes_per_axis));

    vec3 ray_dir = oct_to_vec(sample_oct+(blue_noise(gl_FragCoord.xy/256.0).xy-0.5f)*(1.0/lightprobe_resolution));
    vec3 pos = texelFetch(lightprobe_x, probe_coord, 0).xyz;
    // vec3 pos = (vec3(probe_pos)+blue_noise(gl_FragCoord.xy/256.0f+vec2(0.8f,0.2f)).xyz)*lightprobe_spacing;

    vec3 hit_pos;
    float hit_dist;
    ivec3 hit_cell;
    vec3 hit_dir;
    vec3 normal;
    bool hit = coarse_cast_ray(ray_dir, pos, hit_pos, hit_dist, hit_cell, hit_dir, normal);
    // bool hit = cast_ray(ray_dir, pos, hit_pos, hit_dist, hit_cell, hit_dir, normal);

    const float decay_fraction = 0.01;

    if(hit)
    {
        ivec4 voxel = texelFetch(materials, hit_cell, 0);

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
            roughness = 0.5;
        }
        else
        {
            reflectivity = vec3(0.1,0.1,0.2);
            roughness = 0.1;
        }

        color.rgb = vec3(0);
        // color.rgb = pos/512.0;
        color.rgb += emission;

        // float total_weight = 0.0;

        vec3 reflection_dir = ray_dir-(2*dot(ray_dir, normal))*normal;
        reflection_dir += roughness*blue_noise(gl_FragCoord.xy/256.0f+vec2(0.4f,0.6f)).xyz;
        reflection_dir = normalize(reflection_dir);

        vec2 sample_depth;
        color.rgb += reflectivity*sample_lightprobe_color(hit_pos, normal, vec_to_oct(reflection_dir), sample_depth);
    }
    else
    {
        vec2 sample_depth;
        vec3 sample_color = sample_lightprobe_color(hit_pos, ray_dir, vec_to_oct(ray_dir), sample_depth);
        // hit_dist += sample_depth.r;
        color.rgb = sample_color;
    }

    depth.r = clamp(hit_dist, 0, 1*lightprobe_spacing);
    depth.g = sq(depth.r);

    color.a = decay_fraction;
    depth.a = decay_fraction;
    //TODO: this arbitrarily gives later samples in the same frame more weight

    // depth.rg = vec2(512.0,sq(512.0));
    // color.rgb = 0.5+0.5*ray_dir;
    // color.rgb = clamp(color.rgb, 0.0f, 1.0f);
    color.rgb = max(color.rgb, 0);
}
