#program render_world_program

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
layout(location = 1) out vec3 normal;

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform mat3 camera_axes;
layout(location = 2) uniform vec3 camera_pos;
layout(location = 3) uniform sampler2D material_visual_properties;
layout(location = 4) uniform sampler2D prepass_x;
layout(location = 5) uniform sampler2D prepass_orientation;
layout(location = 6) uniform usampler2D prepass_voxel;
layout(location = 7) uniform sampler2D prepass_color;
layout(location = 8) uniform sampler2D lightprobe_color;
layout(location = 9) uniform sampler2D lightprobe_depth;
layout(location = 10) uniform sampler2D lightprobe_x;
layout(location = 11) uniform sampler2D blue_noise_texture;

#include "include/maths.glsl"
#include "include/blue_noise.glsl"
#include "include/materials.glsl"
#include "include/lightprobe.glsl"

smooth in vec2 screen_pos;

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

    vec3 ray_sign = sign(ray_dir);

    vec3 invabs_ray_dir = ray_sign/ray_dir;

    float epsilon = 0.02;
    int i = 0;
    float total_dist = 10000;


    vec2 prepass_resolution = vec2(1280/2, 720/2);

    ivec2 prepass_coord = ivec2((0.5*screen_pos+0.5)*prepass_resolution-0.5);

    uvec4 voxel;
    vec3 hit_pos;

    ivec2 d = ivec2(0);
    for(d.y = 0; d.y <= 1; d.y++)
        for(d.x = 0; d.x <= 1; d.x++)
        {
            vec3 x = texelFetch(prepass_x, prepass_coord+d, 0).xyz;
            vec4 orientation = texelFetch(prepass_orientation, prepass_coord+d, 0);
            vec4 inv_orientation = conjugate(orientation);

            vec3 cube_ray_dir = apply_rotation(inv_orientation, ray_dir);
            vec3 cube_ray_pos = apply_rotation(inv_orientation, camera_pos-x);

            vec3 ray_sign = sign_not_zero(cube_ray_dir);
            cube_ray_dir *= -ray_sign;
            cube_ray_pos *= -ray_sign;
            cube_ray_pos -= 0.5; //origin placed at front-most corner
            vec3 dist = -cube_ray_pos/cube_ray_dir;
            vec3 max_dir = step(dist.zxy, dist.xyz)*step(dist.yzx, dist.xyz);
            float face_dist = dot(max_dir, dist);
            vec3 hit_x = cube_ray_pos+face_dist*cube_ray_dir;
            vec3 outmost_edge = step(hit_x.zxy, hit_x.xyz)*step(hit_x.yzx, hit_x.xyz);

            if(all(greaterThan(hit_x, vec3(-1.0)-face_dist/100)) &&
               face_dist < total_dist && face_dist > 0)
            {
                total_dist = face_dist;
                frag_color.rgb = texelFetch(prepass_color, prepass_coord+d, 0).rgb;
                voxel = texelFetch(prepass_voxel, prepass_coord+d, 0);
                normal = apply_rotation(orientation, -ray_sign*max_dir);
                hit_pos = camera_pos+face_dist*ray_dir;
                // frag_color.rgb = max_dir;
            }
        }

    //
    {
        uint material_id = voxel.r;
        float roughness = get_roughness(material_id);
        vec3 emission = get_emission(material_id);

        frag_color.rgb += -(emission)*dot(normal, ray_dir);

        int n_probe_samples = 1;
        int n_ray_samples = 0;
        int n_samples = n_probe_samples+n_ray_samples;
        for(int samp = 0; samp < n_probe_samples; samp++)
        {
            int noise_index = ((n_samples*samp+(frame_number%1))*256*256+(int(gl_FragCoord.x)%256)*256+(int(gl_FragCoord.y)%256));
            // vec3 reflection_normal = normal+0.5*roughness*(quasi_noise(noise_index).xyz-0.5f);
            // vec3 reflection_normal = normal+0.5*roughness*(static_blue_noise(gl_FragCoord.xy/256.0+samp*vec2(0.82,0.34)).xyz-0.5f);
            vec3 reflection_normal = normal;
            reflection_normal = normalize(reflection_normal);
            // vec3 reflection_dir = ray_dir - 2*dot(ray_dir, reflection_normal)*reflection_normal;
            vec3 reflection_dir = normal;
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

    frag_color.rgb = clamp(frag_color.rgb, 0, 1);
    // frag_color.rgb = normal;
    gl_FragDepth = 1.0f/total_dist;
}