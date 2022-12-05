#program update_lightprobe_depth_program

/////////////////////////////////////////////////////////////////

#shader GL_COMPUTE_SHADER
#include "include/header.glsl"

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform sampler2D lightprobe_color;
layout(location = 2) uniform sampler2D lightprobe_depth;
layout(location = 3) uniform sampler2D lightprobe_x;
layout(location = 4, rg16f) uniform image2D lightprobe_depth_out;

#include "include/maths.glsl"
#include "include/lightprobe.glsl"
#include "include/probe_ray.glsl"

#define group_size lightprobe_depth_resolution
layout(local_size_x = group_size, local_size_y = group_size, local_size_z = 1) in;

void main()
{
    ivec2 probe_coord = ivec2(gl_WorkGroupID.xy);
    ivec2 depth_texel_coord = lightprobe_depth_padded_resolution*probe_coord+1;
    int probe_i = int(probe_coord.x+lightprobes_w*probe_coord.y);

    ivec2 o = ivec2(gl_LocalInvocationID.xy);
    vec2 oct = (2.0/lightprobe_depth_resolution)*(vec2(o)+0.5)-1.0;

    vec3 new_depth = vec3(0);

    for(int sample_i = 0; sample_i < rays_per_lightprobe; sample_i++)
    {
        int ray_i = probe_i*rays_per_lightprobe+sample_i;
        vec3 ray_dir = normalize(probe_rays[ray_i].rel_hit_pos);
        vec3 depth;
        depth.r = clamp(length(probe_rays[ray_i].rel_hit_pos), 0, 2*lightprobe_spacing);
        depth.g = sq(depth.r);
        depth.b = 1;

        vec3 probe_texel_dir = oct_to_vec(oct);
        float weight = max(dot(probe_texel_dir, ray_dir), 0);
        weight = pow(weight, 101.0);
        if(weight > 0.001)
        {
            new_depth += weight*depth;
        }
    }

    vec2 texel_depth = texelFetch(lightprobe_depth, depth_texel_coord+o, 0).rg;
    float decay_fraction = 0.05;
    if(new_depth.z > 0.001) texel_depth = mix(texel_depth, new_depth.xy/new_depth.z, decay_fraction*new_depth.z);
    imageStore(lightprobe_depth_out, depth_texel_coord+o, vec4(texel_depth, 0, 1));

    ivec2 o1 = ivec2(o.x, lightprobe_depth_resolution-1-o.y);
    ivec2 o2 = ivec2(lightprobe_depth_resolution-1-o.x, o.y);
    ivec2 o3 = ivec2(lightprobe_depth_resolution-1-o.x, lightprobe_depth_resolution-1-o.y);

    if(o.x == 0) {o1.x--; o3.x++;}
    if(o2.x == 0) {o1.x++; o3.x--;}
    if(o1.x != o.x) imageStore(lightprobe_depth_out, depth_texel_coord+o1, vec4(texel_depth, 0, 1));

    if(o.y == 0) {o2.y--; o3.y++;}
    if(o1.y == 0) {o2.y++; o3.y--;}
    if(o2.y != o.y) imageStore(lightprobe_depth_out, depth_texel_coord+o2, vec4(texel_depth, 0, 1));

    if(o1.x != o.x && o2.y != o.y) imageStore(lightprobe_depth_out, depth_texel_coord+o3, vec4(texel_depth, 0, 1));
}

/////////////////////////////////////////////////////////////////
