#program render_prepass_program

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

layout(location = 0) out vec3 voxel_x;
layout(location = 1) out vec4 voxel_orientation;
layout(location = 2) out uvec4 voxel_data;
layout(location = 3) out vec4 voxel_color;

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

    vec3 pos = camera_pos;

    uint medium = texelFetch(materials, ivec3(pos), 0).r;

    voxel_data = uvec4(0,0,0,0);
    vec3 reflectivity = vec3(1,1,1);

    vec3 ray_sign = sign(ray_dir);

    vec3 invabs_ray_dir = ray_sign/ray_dir;

    float epsilon = 0.02;
    int i = 0;
    float total_dist = 0;

    voxel_orientation = vec4(1,0,0,0);

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
    bool hit = cast_ray(materials, ray_dir, pos, size, origin, medium, hit_pos, hit_dist, hit_cell, hit_dir, normal, voxel, 200);
    // bool hit = coarse_cast_ray(ray_dir, pos, hit_pos, hit_dist, hit_cell, hit_dir, normal);
    // voxel = texelFetch(materials, hit_cell, 0);

    if(hit)
    {
        // normal = normalize(unnormalized_gradient(materials, hit_cell));
        voxel_x = vec3(hit_cell)+0.5;
    }

    for(int b = 0; b < n_bodies; b++)
    {
        if(bodies[b].substantial == 0) continue;
        ivec3 body_x_origin = ivec3(bodies[b].x_origin_x, bodies[b].x_origin_y, bodies[b].x_origin_z);
        vec3 body_x_cm = vec3(bodies[b].x_cm_x, bodies[b].x_cm_y, bodies[b].x_cm_z)+vec3(body_x_origin);
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
        ivec3 ibody_pos = ivec3(floor(body_pos));
        vec3 body_ray_dir = apply_rotation(conjugate(body_orientation), ray_dir);

        float body_jump_dist = 0.0;
        if(ibody_pos.x < 0            && body_ray_dir.x > 0) body_jump_dist = max(body_jump_dist, epsilon+(-body_pos.x)/(body_ray_dir.x));
        if(ibody_pos.x >= body_size.x && body_ray_dir.x < 0) body_jump_dist = max(body_jump_dist, epsilon+(body_size.x-body_pos.x)/(body_ray_dir.x));
        if(ibody_pos.y < 0            && body_ray_dir.y > 0) body_jump_dist = max(body_jump_dist, epsilon+(-body_pos.y)/(body_ray_dir.y));
        if(ibody_pos.y >= body_size.y && body_ray_dir.y < 0) body_jump_dist = max(body_jump_dist, epsilon+(body_size.y-body_pos.y)/(body_ray_dir.y));
        if(ibody_pos.z < 0            && body_ray_dir.z > 0) body_jump_dist = max(body_jump_dist, epsilon+(-body_pos.z)/(body_ray_dir.z));
        if(ibody_pos.z >= body_size.z && body_ray_dir.z < 0) body_jump_dist = max(body_jump_dist, epsilon+(body_size.z-body_pos.z)/(body_ray_dir.z));

        body_pos += body_jump_dist*body_ray_dir;

        vec3 body_hit_pos;
        float body_hit_dist;
        ivec3 body_hit_cell;
        vec3 body_hit_dir;
        vec3 body_normal;
        uvec4 body_voxel;
        bool body_hit = cast_ray(body_materials, body_ray_dir, body_pos, body_size, body_origin, 0, body_hit_pos, body_hit_dist, body_hit_cell, body_hit_dir, body_normal, body_voxel, 100);
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
            if(voxel.r >= BASE_CELL_MAT) voxel.r += bodies[b].cell_material_id;
            voxel_orientation = body_orientation;
            voxel_x = apply_rotation(body_orientation, vec3(body_hit_cell)-body_x_cm+0.5)+body_x;
        }
    }

    vec4 transmission = vec4(1);

    voxel_color = vec4(0);

    total_dist += hit_dist;
    if(hit)
    {
        float initial_transmission = exp(-opacity(mat(medium))*hit_dist);

        voxel_data = voxel;
        gl_FragDepth = 1.0f/total_dist;

        for(int i = 0; i < 5; i++)
        {
            if(opacity(mat(voxel)) >= 1) break;
            vec3 ray_pos = hit_pos;
            if(ivec3(ray_pos) != hit_cell) ray_pos += 0.001*ray_dir;
            float c = dot(ray_dir, normal);
            float r = refractive_index(medium)/refractive_index(mat(voxel));
            float square = 1.0-sq(r)*(1.0-sq(c));
            if(square > 0) ray_dir = r*ray_dir+(-r*c+sign(c)*sqrt(square))*normal;
            else ray_dir = ray_dir - 2*c*normal; //total internal reflection
            // ray_dir = normalize(ray_dir);
            medium = mat(voxel);
            bool hit = cast_ray(materials, ray_dir, ray_pos, size, origin, medium, hit_pos, hit_dist, hit_cell, hit_dir, normal, voxel, 200);

            if(hit)
            {
                transmission *= exp(-opacity(mat(medium))*hit_dist);
            }
            else
            {
                break;
            }

            if(dot(transmission, transmission) < 0.001) break;

            uint material_id = voxel.r;
            float roughness = get_roughness(material_id);
            vec3 emission = get_emission(material_id);

            // voxel_color.rgb += -(emission)*dot(normal, ray_dir);
            voxel_color.rgb += emission;

            //TODO: actual blackbody color
            voxel_color.rgb += vec3(1,0.05,0.1)*clamp((1.0/127.0)*(float(temp(voxel))-128), 0.0, 1.0);

            voxel_color.rgb += vec3(0.7,0.3,1.0)*clamp((1.0/15.0)*(float(volt(voxel))), 0.0, 1.0);

            vec3 reflection_dir = normal;
            vec2 sample_depth;
            voxel_color.rgb += fr(material_id, reflection_dir, -ray_dir, normal)
                *sample_lightprobe_color(hit_pos, normal, vec_to_oct(reflection_dir), sample_depth);
            voxel_color *= transmission;
        }
        voxel_color.a = initial_transmission; //pretty hacky, pretty sure I should restructure rendering stuff later
    }
}
