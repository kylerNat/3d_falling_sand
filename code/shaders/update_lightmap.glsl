#program update_lightmap_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"

layout(location = 0) in vec2 x;
layout(location = 1) in vec2 X;

layout(location = 0) uniform int frame_number;
layout(location = 4) uniform sampler2D lightprobe_color;
layout(location = 5) uniform sampler2D lightprobe_depth;
layout(location = 6) uniform sampler2D lightprobe_x;
layout(location = 7) uniform sampler2D blue_noise_texture;

#include "include/maths.glsl"
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
layout(location = 1) uniform sampler2D material_visual_properties;
layout(location = 2) uniform usampler3D materials;
layout(location = 3) uniform usampler3D occupied_regions;
layout(location = 4) uniform sampler2D lightprobe_color;
layout(location = 5) uniform sampler2D lightprobe_depth;
layout(location = 6) uniform sampler2D lightprobe_x;
layout(location = 7) uniform sampler2D blue_noise_texture;

ivec3 size = ivec3(512);
ivec3 origin = ivec3(0);

#include "include/maths.glsl"
#include "include/blue_noise.glsl"
#include "include/body_data.glsl"
#include "include/materials.glsl"
#include "include/materials_physical.glsl"
#include "include/raycast.glsl"
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

    vec2 jittered_oct = sample_oct+(blue_noise(gl_FragCoord.xy/256.0).xy-0.5f)*(2.0/lightprobe_resolution);
    jittered_oct = clamp(jittered_oct, -1, 1);
    vec3 ray_dir = oct_to_vec(jittered_oct);
    vec3 pos = texelFetch(lightprobe_x, probe_coord, 0).xyz;
    // vec3 pos = (vec3(probe_pos)+blue_noise(gl_FragCoord.xy/256.0f+vec2(0.8f,0.2f)).xyz)*lightprobe_spacing;

    vec3 hit_pos;
    float hit_dist;
    ivec3 hit_cell;
    vec3 hit_dir;
    vec3 normal;
    // bool hit = coarse_cast_ray(ray_dir, pos, hit_pos, hit_dist, hit_cell, hit_dir, normal);

    ivec3 origin = ivec3(0);
    ivec3 size = ivec3(512);
    uvec4 voxel;
    bool hit = cast_ray(materials, ray_dir, pos, size, origin, hit_pos, hit_dist, hit_cell, hit_dir, normal, voxel, 24);

    const float decay_fraction = 0.01;

    if(hit)
    {
        uvec4 voxel = texelFetch(materials, hit_cell, 0);

        uint material_id = voxel.r;

        float roughness = get_roughness(material_id);
        vec3 emission = get_emission(material_id);

        color.rgb = vec3(0);
        color.rgb += -(emission)*dot(normal, ray_dir);

        // float total_weight = 0.0;

        // vec3 reflection_normal = normal+0.5*roughness*(blue_noise(gl_FragCoord.xy/256.0+vec2(0.82,0.34)).xyz-0.5f);
        vec3 reflection_normal = normal;
        reflection_normal = normalize(reflection_normal);
        // vec3 reflection_dir = ray_dir-(2*dot(ray_dir, reflection_normal))*reflection_normal;
        vec3 reflection_dir = normal;
        // reflection_dir += roughness*blue_noise(gl_FragCoord.xy/256.0f+vec2(0.4f,0.6f)).xyz;

        vec2 sample_depth;
        color.rgb += fr(material_id, reflection_dir, -ray_dir, normal)*sample_lightprobe_color(hit_pos, normal, vec_to_oct(reflection_dir), sample_depth);
    }
    // else
    // {
    //     vec2 sample_depth;
    //     vec3 sample_color = sample_lightprobe_color(hit_pos, ray_dir, vec_to_oct(ray_dir), sample_depth);
    //     // hit_dist += sample_depth.r;
    //     color.rgb = sample_color;
    // }

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
