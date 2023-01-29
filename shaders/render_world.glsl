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
layout(location = 8) uniform vec2 prepass_resolution;
layout(location = 9) uniform sampler2D lightprobe_color;
layout(location = 10) uniform sampler2D lightprobe_depth;
layout(location = 11) uniform sampler2D lightprobe_x;
layout(location = 12) uniform sampler2D blue_noise_texture;

#include "include/maths.glsl"
#include "include/blue_noise.glsl"
#include "include/materials_visual.glsl"
#include "include/materials_physical.glsl"
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

    ivec2 prepass_coord = ivec2((0.5*screen_pos+0.5)*prepass_resolution-0.5);

    uvec4 voxel;
    vec3 hit_pos;

    float transmission = 1;

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

            if(all(greaterThan(hit_x, vec3(-1.0) +face_dist/((270.0/pi)*dot(cube_ray_dir, max_dir)))) &&
               face_dist < total_dist && face_dist > 0)
            // if(all(greaterThan(hit_x, vec3(-1.0))) &&
            //    face_dist < total_dist && face_dist > 0)
            {
                total_dist = face_dist;
                vec4 transmitted_color = texelFetch(prepass_color, prepass_coord+d, 0);
                transmission = transmitted_color.a;
                frag_color.rgb = transmitted_color.rgb;
                voxel = texelFetch(prepass_voxel, prepass_coord+d, 0);
                normal = apply_rotation(orientation, -ray_sign*max_dir);
                hit_pos = camera_pos+face_dist*ray_dir;
                // frag_color.rgb = max_dir;
            }
        }

    // {
    //     ivec2 prepass_coord = ivec2((0.5*screen_pos+0.5)*prepass_resolution);

    //     vec3 x = texelFetch(prepass_x, prepass_coord, 0).xyz;
    //     vec4 orientation = texelFetch(prepass_orientation, prepass_coord+d, 0);
    //     vec4 inv_orientation = conjugate(orientation);

    //     vec3 cube_ray_dir = apply_rotation(inv_orientation, ray_dir);
    //     vec3 cube_ray_pos = apply_rotation(inv_orientation, camera_pos-x);

    //     vec3 ray_sign = sign_not_zero(cube_ray_dir);
    //     cube_ray_dir *= -ray_sign;
    //     cube_ray_pos *= -ray_sign;
    //     cube_ray_pos -= 0.5; //origin placed at front-most corner
    //     vec3 dist = -cube_ray_pos/cube_ray_dir;
    //     vec3 max_dir = step(dist.zxy, dist.xyz)*step(dist.yzx, dist.xyz);
    //     float face_dist = dot(max_dir, dist);
    //     vec3 hit_x = cube_ray_pos+face_dist*cube_ray_dir;
    //     vec3 outmost_edge = step(hit_x.zxy, hit_x.xyz)*step(hit_x.yzx, hit_x.xyz);

    //     total_dist = face_dist;
    //     vec4 transmitted_color = texelFetch(prepass_color, prepass_coord, 0);
    //     reflectance = 1-transmitted_color.a;
    //     frag_color.rgb = transmitted_color.rgb;
    //     voxel = texelFetch(prepass_voxel, prepass_coord, 0);
    //     normal = apply_rotation(orientation, -ray_sign*max_dir);
    //     hit_pos = camera_pos+face_dist*ray_dir;
    //     // frag_color.rgb = max_dir;
    // }

    {
        uint material_id = voxel.r;
        float roughness = get_roughness(material_id);
        vec3 emission = get_emission(material_id);

        // frag_color.rgb += -(emission)*dot(normal, ray_dir);
        frag_color.rgb += emission;

        //TODO: actual blackbody color
        frag_color.rgb += vec3(1,0.05,0.1)*clamp((1.0/127.0)*(float(temp(voxel))-128), 0.0, 1.0);

        frag_color.rgb += vec3(0.7,0.3,1.0)*clamp((1.0/15.0)*(float(volt(voxel))), 0.0, 1.0);

        vec3 reflection_dir = normal;
        vec2 sample_depth;
        frag_color.rgb += fr(material_id, reflection_dir, -ray_dir, normal)
            *sample_lightprobe_color(hit_pos, normal, vec_to_oct(reflection_dir), sample_depth);
        // frag_color.rgb += sample_lightprobe_color(hit_pos, normal, vec_to_oct(reflection_dir), sample_depth); //no materials
    }
    frag_color.rgb *= transmission;

    frag_color.rgb = clamp(frag_color.rgb, 0, 1);
    // frag_color.rgb = normal;
    gl_FragDepth = 1.0f/total_dist;
}
