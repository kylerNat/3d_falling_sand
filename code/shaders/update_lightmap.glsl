#program update_lightmap_program

/////////////////////////////////////////////////////////////////

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

layout(location = 9) uniform writeonly image2D lightprobe_color_out;
layout(location = 10) uniform writeonly image2D lightprobe_depth_out;

#include "include/maths.glsl"
#include "include/blue_noise.glsl"
#include "include/materials.glsl"
#include "include/materials_physical.glsl"
#define ACTIVE_REGIONS
#include "include/raycast.glsl"
#include "include/lightprobe.glsl"

#define group_size 4
layout(local_size_x = group_size, local_size_y = group_size, local_size_z = 1) in;

void main()
{
    ivec2 probe_coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 texel_coord = lightprobe_padded_resolution*probe_coord+1;

    #define v vec3(0)
    vec3 new_colors[lightprobe_resolution*lightprobe_resolution]
        = vec3[lightprobe_resolution*lightprobe_resolution](v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v);
    #undef v
    #define v vec2(0)
    vec2 new_depths[lightprobe_resolution*lightprobe_resolution]
        = vec2[lightprobe_resolution*lightprobe_resolution](v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v,v);
    #undef v

    const float decay_fraction = 0.005;

    const int n_samples = 4;
    for(int sample_i = 0; sample_i < n_samples; sample_i++)
    {
        // int probe_id = int(lightprobes_w*gl_GlobalInvocationID.y+gl_GlobalInvocationID.x);
        // vec3 ray_dir = quasinoise_3(probe_id+lightprobes_w*lightprobes_h*(frame_number%100))-0.5;
        // vec3 ray_dir = blue_noise_3(vec2(probe_coord)/256.0)-0.5;
        vec2 Omega = blue_noise_2(vec2(probe_coord)/256.0+sample_i*vec2(0.86, 0.24));
        float cosphi = cos((2*pi)*Omega.x);
        float sinphi = sin((2*pi)*Omega.x);
        float costheta = 2*Omega.y-1;
        float sintheta = sqrt(max(0, 1.0-sq(costheta)));
        vec3 ray_dir = vec3(sintheta*cosphi, sintheta*sinphi, costheta);
        ray_dir = normalize(ray_dir);

        vec3 probe_x = texelFetch(lightprobe_x, probe_coord, 0).xyz;

        vec3 hit_pos;
        float hit_dist;
        ivec3 hit_cell;
        vec3 hit_dir;
        vec3 normal;
        ivec3 origin = ivec3(0);
        ivec3 size = ivec3(512);
        uvec4 voxel;
        bool hit = cast_ray(materials, ray_dir, probe_x, size, origin, hit_pos, hit_dist, hit_cell, hit_dir, normal, voxel, 24);

        vec3 color = vec3(0);
        vec2 depth = vec2(0);

        if(hit)
        {
            uvec4 voxel = texelFetch(materials, hit_cell, 0);

            uint material_id = voxel.r;

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
            color += fr(material_id, reflection_dir, -ray_dir, normal)*sample_lightprobe_color(hit_pos, normal, vec_to_oct(reflection_dir), sample_depth);
        }
        else
        {
            vec2 sample_depth;
            vec3 sample_color = sample_lightprobe_color(hit_pos, ray_dir, vec_to_oct(ray_dir), sample_depth);
            // hit_dist += sample_depth.r;
            color.rgb = sample_color;
        }

        // color = clamp(color, 0, 1);
        color = max(color, 0);

        depth.r = clamp(hit_dist, 0, 16*lightprobe_spacing);
        // depth.r = hit_dist;
        depth.g = sq(depth.r);

        ivec2 o;
        for(o.y = 0; o.y < lightprobe_resolution; o.y++)
            for(o.x = 0; o.x < lightprobe_resolution; o.x++)
            {
                vec2 oct = (2.0/lightprobe_resolution)*(vec2(o)+0.5)-1.0;
                vec3 probe_texel_dir = oct_to_vec(oct);
                float weight = (5.0/n_samples)*max(0, dot(probe_texel_dir, ray_dir));

                if(weight > 0.001)
                {
                    new_colors[o.x+o.y*lightprobe_resolution] += weight*color;
                    // new_depths[o.x+o.y*lightprobe_resolution] += pow(weight, 20.0)*depth;
                }
            }
        ivec2 depth_oct = clamp(ivec2(lightprobe_resolution*(0.5*vec_to_oct(ray_dir)+0.5)), 0, lightprobe_resolution);
        // new_depths[depth_oct.x+depth_oct.y*lightprobe_resolution] += depth;

        vec2 texel_depth = texelFetch(lightprobe_depth, texel_coord+depth_oct, 0).rg;
        texel_depth = mix(texel_depth, 0.8*depth, decay_fraction);
        imageStore(lightprobe_depth_out, texel_coord+depth_oct, vec4(texel_depth, 0, 1));
    }

    ivec2 o;
    for(o.y = 0; o.y < lightprobe_resolution; o.y++)
        for(o.x = 0; o.x < lightprobe_resolution; o.x++)
        {
                vec3 texel_color = texelFetch(lightprobe_color, texel_coord+o, 0).rgb;
                texel_color = mix(texel_color, new_colors[o.x+o.y*lightprobe_resolution],
                                  decay_fraction);
                imageStore(lightprobe_color_out, texel_coord+o, vec4(texel_color, 1));

                // vec2 texel_depth = texelFetch(lightprobe_depth, texel_coord+o, 0).rg;
                // texel_depth = mix(texel_depth, new_depths[o.x+o.y*lightprobe_resolution],
                //                   decay_fraction);
                // imageStore(lightprobe_depth_out, texel_coord+o, vec4(texel_depth, 0, 1));
        }
}

/////////////////////////////////////////////////////////////////
