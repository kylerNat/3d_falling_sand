#program update_lightmap_program

/////////////////////////////////////////////////////////////////

#shader GL_COMPUTE_SHADER
#include "include/header.glsl"

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform sampler2D lightprobe_color;
layout(location = 2) uniform sampler2D lightprobe_depth;
layout(location = 3) uniform sampler2D lightprobe_x;
layout(location = 4, rgba16f) uniform image2D lightprobe_color_out;
layout(location = 5, rg16f) uniform image2D lightprobe_depth_out;

#include "include/maths.glsl"
#include "include/lightprobe.glsl"
#include "include/probe_ray.glsl"

#define group_size 4
layout(local_size_x = group_size, local_size_y = group_size, local_size_z = 1) in;

void main()
{
    ivec2 probe_coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 texel_coord = lightprobe_padded_resolution*probe_coord+1;
    ivec2 depth_texel_coord = lightprobe_depth_padded_resolution*probe_coord+1;
    int probe_i = int(probe_coord.x+lightprobes_w*probe_coord.y);

    #define v vec3(0)
    vec3 new_colors[lightprobe_resolution*lightprobe_resolution]
        = vec3[lightprobe_resolution*lightprobe_resolution](v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v);
    #undef v
    #define v vec3(0)
    vec3 new_depths[lightprobe_depth_resolution*lightprobe_depth_resolution]
        = vec3[lightprobe_depth_resolution*lightprobe_depth_resolution](v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v);
    #undef v

    for(int sample_i = 0; sample_i < rays_per_lightprobe; sample_i++)
    {
        int ray_i = probe_i*rays_per_lightprobe+sample_i;
        vec3 ray_dir = normalize(probe_rays[ray_i].rel_hit_pos);
        vec3 color = probe_rays[ray_i].hit_color;
        vec3 depth;
        depth.r = clamp(length(probe_rays[ray_i].rel_hit_pos), 0, 2*lightprobe_spacing);
        depth.g = sq(depth.r);
        depth.b = 1;

        ivec2 o;
        for(o.y = 0; o.y < lightprobe_resolution; o.y++)
            for(o.x = 0; o.x < lightprobe_resolution; o.x++)
            {
                vec2 oct = (2.0/lightprobe_resolution)*(vec2(o)+0.5)-1.0;
                vec3 probe_texel_dir = oct_to_vec(oct);
                float weight = (1.0/rays_per_lightprobe)*max(0, dot(probe_texel_dir, ray_dir));

                if(weight > 0.001)
                {
                    new_colors[o.x+o.y*lightprobe_resolution] += weight*color;
                }
            }

        for(o.y = 0; o.y < lightprobe_depth_resolution; o.y++)
            for(o.x = 0; o.x < lightprobe_depth_resolution; o.x++)
            {
                vec2 oct = (2.0/lightprobe_depth_resolution)*(vec2(o)+0.5)-1.0;
                vec3 probe_texel_dir = oct_to_vec(oct);
                float weight = max(dot(probe_texel_dir, ray_dir), 0);
                weight = pow(weight, 101.0);
                if(weight > 0.001)
                {
                    new_depths[o.x+o.y*lightprobe_depth_resolution] += weight*depth;
                }
            }

        // ivec2 depth_oct = clamp(ivec2(lightprobe_depth_resolution*(0.5*vec_to_oct(ray_dir)+0.5)), 0, lightprobe_depth_resolution);
        // new_depths[depth_oct.x+depth_oct.y*lightprobe_depth_resolution] += depth;

        // vec2 texel_depth = texelFetch(lightprobe_depth, texel_coord+depth_oct, 0).rg;
        // texel_depth = mix(texel_depth, 0.8*depth, decay_fraction);
        // imageStore(lightprobe_depth_out, texel_coord+depth_oct, vec4(texel_depth, 0, 1));
    }

    ivec2 o;
    for(o.y = 0; o.y < lightprobe_resolution; o.y++)
        for(o.x = 0; o.x < lightprobe_resolution; o.x++)
        {
            vec3 texel_color = texelFetch(lightprobe_color, texel_coord+o, 0).rgb;
            vec3 new_color = new_colors[o.x+o.y*lightprobe_resolution];
            float decay_fraction = 0.05*dot(mix(texel_color, new_color, 0.02), vec3(1)); //update more slowly for low light levels
            texel_color = mix(texel_color, new_color, decay_fraction);
            imageStore(lightprobe_color_out, texel_coord+o, vec4(texel_color, 1));
        }

    for(int i = 0; i <= lightprobe_resolution; i++)
    {
        ivec2 o = ivec2(lightprobe_resolution-1-i, 0);
        ivec2 p = ivec2(i, -1);
        if(i == lightprobe_resolution) o = ivec2(0, lightprobe_resolution-1);

        for(int j = 0; j < 4; j++)
        {
            vec4 color = imageLoad(lightprobe_color_out, texel_coord+o);
            imageStore(lightprobe_color_out, texel_coord+p, color);

            o = ivec2(lightprobe_resolution-1-o.y, o.x);
            p = ivec2(lightprobe_resolution-1-p.y, p.x);
        }
    }

    for(o.y = 0; o.y < lightprobe_depth_resolution; o.y++)
        for(o.x = 0; o.x < lightprobe_depth_resolution; o.x++)
        {
            vec3 new_depth = new_depths[o.x+o.y*lightprobe_depth_resolution];
            vec2 texel_depth = texelFetch(lightprobe_depth, depth_texel_coord+o, 0).rg;
            if(new_depth.z > 0.001)
            {
                const float decay_fraction = 0.05;
                texel_depth = mix(texel_depth, new_depth.xy/new_depth.z, decay_fraction);
            }
            imageStore(lightprobe_depth_out, depth_texel_coord+o, vec4(texel_depth, 0, 1));
        }

    for(int i = 0; i <= lightprobe_depth_resolution; i++)
    {
        ivec2 o = ivec2(lightprobe_depth_resolution-1-i, 0);
        ivec2 p = ivec2(i, -1);
        if(i == lightprobe_depth_resolution) o = ivec2(0, lightprobe_depth_resolution-1);

        for(int j = 0; j < 4; j++)
        {
            vec4 depth = imageLoad(lightprobe_depth_out, depth_texel_coord+o);
            imageStore(lightprobe_depth_out, depth_texel_coord+p, depth);

            o = ivec2(lightprobe_depth_resolution-1-o.y, o.x);
            p = ivec2(lightprobe_depth_resolution-1-p.y, p.x);
        }
    }
}

/////////////////////////////////////////////////////////////////
