#program update_lightprobe_color_program

/////////////////////////////////////////////////////////////////

#shader GL_COMPUTE_SHADER
#include "include/header.glsl"

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform sampler2D lightprobe_color;
layout(location = 2) uniform sampler2D lightprobe_depth;
layout(location = 3) uniform sampler2D lightprobe_x;
layout(location = 4, rgba16f) uniform image2D lightprobe_color_out;

#include "include/maths.glsl"
#include "include/lightprobe.glsl"
#include "include/probe_ray.glsl"

#define group_size lightprobe_resolution
layout(local_size_x = group_size, local_size_y = group_size, local_size_z = 1) in;

void main()
{
    ivec2 probe_coord = ivec2(gl_WorkGroupID.xy);
    ivec2 texel_coord = lightprobe_padded_resolution*probe_coord+1;
    int probe_i = int(probe_coord.x+lightprobes_w*probe_coord.y);

    ivec2 o = ivec2(gl_LocalInvocationID.xy);
    vec2 oct = (2.0/lightprobe_resolution)*(vec2(o)+0.5)-1.0;

    vec4 new_color = vec4(0);

    for(int sample_i = 0; sample_i < rays_per_lightprobe; sample_i++)
    {
        int ray_i = probe_i*rays_per_lightprobe+sample_i;
        vec3 ray_dir = normalize(probe_rays[ray_i].rel_hit_pos);
        vec4 color = vec4(probe_rays[ray_i].hit_color, 1);

        vec3 probe_texel_dir = oct_to_vec(oct);
        float weight = (1.0/rays_per_lightprobe)*max(0, dot(probe_texel_dir, ray_dir));

        if(weight > 0.001)
        {
            new_color += weight*color;
        }
    }

    vec3 texel_color = texelFetch(lightprobe_color, texel_coord+o, 0).rgb;
    if(new_color.a > 0.001)
    {
        float decay_fraction = 0.05*dot(mix(texel_color, new_color.rgb/new_color.a, 0.02), vec3(1)); //update more slowly for low light levels
        texel_color = mix(texel_color, new_color.rgb/new_color.a, decay_fraction*new_color.a);
    }

    // //mix with neighboring probes
    // ivec3 probe_pos = ivec3(probe_i%lightprobes_per_axis,(probe_i/lightprobes_per_axis)%lightprobes_per_axis, probe_i/(lightprobes_per_axis*lightprobes_per_axis));

    // vec3 neighbor_texel_color = vec3(0.0);
    // ivec3 strides = ivec3(1,lightprobes_per_axis,lightprobes_per_axis*lightprobes_per_axis);

    // ivec3 e_pos = probe_pos+ivec3(+1,0,0);
    // int e_index = int(dot(e_pos, strides));
    // ivec2 e_coord = ivec2(e_index%lightprobes_w,e_index/lightprobes_w);
    // neighbor_texel_color += texelFetch(lightprobe_color, lightprobe_padded_resolution*e_coord+1+o, 0).rgb;
    // ivec3 w_pos =  probe_pos+ivec3(-1,0,0);
    // int w_index = int(dot(w_pos, strides));
    // ivec2 w_coord = ivec2(w_index%lightprobes_w,w_index/lightprobes_w);
    // neighbor_texel_color += texelFetch(lightprobe_color, lightprobe_padded_resolution*w_coord+1+o, 0).rgb;
    // ivec3 n_pos =  probe_pos+ivec3(0,+1,0);
    // int n_index = int(dot(n_pos, strides));
    // ivec2 n_coord = ivec2(n_index%lightprobes_w,n_index/lightprobes_w);
    // neighbor_texel_color += texelFetch(lightprobe_color, lightprobe_padded_resolution*n_coord+1+o, 0).rgb;
    // ivec3 s_pos =  probe_pos+ivec3(0,-1,0);
    // int s_index = int(dot(s_pos, strides));
    // ivec2 s_coord = ivec2(s_index%lightprobes_w,s_index/lightprobes_w);
    // neighbor_texel_color += texelFetch(lightprobe_color, lightprobe_padded_resolution*s_coord+1+o, 0).rgb;
    // ivec3 u_pos =  probe_pos+ivec3(0,0,+1);
    // int u_index = int(dot(u_pos, strides));
    // ivec2 u_coord = ivec2(u_index%lightprobes_w,u_index/lightprobes_w);
    // neighbor_texel_color += texelFetch(lightprobe_color, lightprobe_padded_resolution*u_coord+1+o, 0).rgb;
    // ivec3 d_pos =  probe_pos+ivec3(0,0,-1);
    // int d_index = int(dot(d_pos, strides));
    // ivec2 d_coord = ivec2(d_index%lightprobes_w,d_index/lightprobes_w);
    // neighbor_texel_color += texelFetch(lightprobe_color, lightprobe_padded_resolution*d_coord+1+o, 0).rgb;
    // texel_color = mix(texel_color, neighbor_texel_color/6.0, 0.001);

    imageStore(lightprobe_color_out, texel_coord+o, vec4(texel_color, 1));

    ivec2 o1 = ivec2(o.x, lightprobe_resolution-1-o.y);
    ivec2 o2 = ivec2(lightprobe_resolution-1-o.x, o.y);
    ivec2 o3 = ivec2(lightprobe_resolution-1-o.x, lightprobe_resolution-1-o.y);

    if(o.x == 0) {o1.x--; o3.x++;}
    if(o2.x == 0) {o1.x++; o3.x--;}
    if(o1.x != o.x) imageStore(lightprobe_color_out, texel_coord+o1, vec4(texel_color, 1));

    if(o.y == 0) {o2.y--; o3.y++;}
    if(o1.y == 0) {o2.y++; o3.y--;}
    if(o2.y != o.y) imageStore(lightprobe_color_out, texel_coord+o2, vec4(texel_color, 1));

    if(o1.x != o.x && o2.y != o.y) imageStore(lightprobe_color_out, texel_coord+o3, vec4(texel_color, 1));
}

/////////////////////////////////////////////////////////////////
