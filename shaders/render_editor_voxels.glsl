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
layout(location = 2) uniform sampler2D material_visual_properties;
layout(location = 3) uniform isampler3D object_materials;
layout(location = 4) uniform sampler2D blue_noise_texture;
layout(location = 5) uniform int frame_number;
layout(location = 6) uniform int n_objects;
layout(location = 7) uniform float fov;
layout(location = 8) uniform sampler2D lightprobe_color;
layout(location = 9) uniform sampler2D lightprobe_depth;
layout(location = 10) uniform sampler2D lightprobe_x;


// #define DEBUG_DOTS
#include "include/maths.glsl"
#include "include/blue_noise.glsl"
#include "include/object_data.glsl"
#include "include/materials_visual.glsl"
#include "include/materials_physical.glsl"
#define ACTIVE_REGIONS
#define RAY_CAST_IGNORE_DEPTH
#include "include/raycast.glsl"
#include "include/lightprobe.glsl"

smooth in vec2 screen_pos;

void main()
{
    float screen_dist = 1.0/tan(fov/2);
    vec3 ray_dir = (16.0/9.0*screen_pos.x*camera_axes[0]
                    +        screen_pos.y*camera_axes[1]
                    -        screen_dist *camera_axes[2]);
    // vec3 ray_dir = (16.0/9.0*sin(screen_pos.x)*camera_axes[0]
    //                 +        sin(screen_pos.y)*camera_axes[1]
    //                 -        cos(screen_pos.x)*cos(screen_pos.y)*screen_dist *camera_axes[2]);
    ray_dir = normalize(ray_dir);

    frag_color = vec4(0,0,0,1);

    vec3 pos = camera_pos;
    vec3 ray_sign = sign(ray_dir);

    vec3 invabs_ray_dir = ray_sign/ray_dir;

    float epsilon = 0.02;
    int i = 0;
    float total_dist = 0;

    vec3 hit_pos;
    float hit_dist;
    ivec3 hit_cell;
    vec3 hit_dir;
    vec3 normal;
    uvec4 voxel;
    int hit_object = -2;
    bool hit = false;

    vec4 tint = vec4(1);
    vec4 highlight = vec4(0);

    for(int o = 0; o < n_objects; o++)
    {
        vec3 object_x = vec3(objects[o].x_x, objects[o].x_y, objects[o].x_z);
        vec4 object_orientation = vec4(objects[o].orientation_r, objects[o].orientation_x, objects[o].orientation_y, objects[o].orientation_z);
        ivec3 object_texture_lower = ivec3(objects[o].lower_x,
                                 objects[o].lower_y,
                                 objects[o].lower_z);
        ivec3 object_texture_upper = ivec3(objects[o].upper_x,
                                 objects[o].upper_y,
                                 objects[o].upper_z);
        ivec3 object_origin_to_lower = ivec3(objects[o].origin_to_lower_x,
                                           objects[o].origin_to_lower_y,
                                           objects[o].origin_to_lower_z);
        ivec3 object_size = object_texture_upper-object_texture_lower;
        ivec3 object_origin = object_texture_lower;

        //ray info in the objects frame
        vec3 object_pos = apply_rotation(conjugate(object_orientation), pos-object_x)-object_origin_to_lower;
        ivec3 iobject_pos = ivec3(floor(object_pos));
        vec3 object_ray_dir = apply_rotation(conjugate(object_orientation), ray_dir);

        vec3 object_hit_dir = vec3(0,0,0);
        float object_jump_dist = 0.0;
        if(iobject_pos.x < 0              && object_ray_dir.x > 0) {
            float new_jump_dist = epsilon+(-object_pos.x)/(object_ray_dir.x);
            if(new_jump_dist > object_jump_dist){
                object_hit_dir = vec3(1,0,0);
                object_jump_dist = new_jump_dist;
            }
        }
        if(iobject_pos.x >= object_size.x && object_ray_dir.x < 0) {
            float new_jump_dist = epsilon+(object_size.x-object_pos.x)/(object_ray_dir.x);
            if(new_jump_dist > object_jump_dist){
                object_hit_dir = vec3(1,0,0);
                object_jump_dist = new_jump_dist;
            }
        }
        if(iobject_pos.y < 0              && object_ray_dir.y > 0) {
            float new_jump_dist = epsilon+(-object_pos.y)/(object_ray_dir.y);
            if(new_jump_dist > object_jump_dist){
                object_hit_dir = vec3(0,1,0);
                object_jump_dist = new_jump_dist;
            }
        }
        if(iobject_pos.y >= object_size.y && object_ray_dir.y < 0) {
            float new_jump_dist = epsilon+(object_size.y-object_pos.y)/(object_ray_dir.y);
            if(new_jump_dist > object_jump_dist){
                object_hit_dir = vec3(0,1,0);
                object_jump_dist = new_jump_dist;
            }
        }
        if(iobject_pos.z < 0              && object_ray_dir.z > 0) {
            float new_jump_dist = epsilon+(-object_pos.z)/(object_ray_dir.z);
            if(new_jump_dist > object_jump_dist){
                object_hit_dir = vec3(0,0,1);
                object_jump_dist = new_jump_dist;
            }
        }
        if(iobject_pos.z >= object_size.z && object_ray_dir.z < 0) {
            float new_jump_dist = epsilon+(object_size.z-object_pos.z)/(object_ray_dir.z);
            if(new_jump_dist > object_jump_dist){
                object_hit_dir = vec3(0,0,1);
                object_jump_dist = new_jump_dist;
            }
        }

        object_pos += object_jump_dist*object_ray_dir;

        vec3 object_hit_pos;
        float object_hit_dist;
        ivec3 object_hit_cell;
        vec3 object_normal;
        uvec4 object_voxel;
        bool object_hit = cast_ray(object_materials, object_ray_dir, object_pos, object_size, object_origin, 0, false, object_hit_pos, object_hit_dist, object_hit_cell, object_hit_dir, object_normal, object_voxel, 100);
        if(object_hit && (!hit || object_jump_dist+object_hit_dist < hit_dist))
        {
            hit = true;
            hit_pos = apply_rotation(object_orientation, object_hit_pos)+object_x;
            hit_dist = object_jump_dist+object_hit_dist;
            hit_cell = object_hit_cell+object_origin_to_lower;
            hit_dir = object_hit_dir;
            hit_object = o;
            normal = apply_rotation(object_orientation, object_normal);
            voxel = object_voxel;
            // if(voxel.r >= BASE_CELL_MAT) voxel.r += objects[o].cell_material_id;

            tint = vec4(objects[o].tint_r, objects[o].tint_g, objects[o].tint_b, objects[o].tint_a);
            highlight = vec4(objects[o].highlight_r, objects[o].highlight_g, objects[o].highlight_b, objects[o].highlight_a);
        }
    }

    total_dist += hit_dist;
    if(hit)
    {
        gl_FragDepth = 1.0f/total_dist;

        uint material_id = mat(voxel);
        // float roughness = get_roughness(material_id);
        vec3 emission = get_emission(material_id);

        frag_color.rgb += emission;

        vec3 spotlight_dir = normalize(vec3(-1.4,-0.8,1));

        vec3 reflection_dir = normal;

        vec2 sample_depth;
        vec3 light_value = sample_lightprobe_color(hit_pos, normal, vec_to_oct(reflection_dir), sample_depth);
        // light_value *= 1.0f-0.05*total_dist;
        // light_value = max(light_value, 0);

        frag_color.rgb += fr(material_id, reflection_dir, -ray_dir, normal)*light_value;

        frag_color.rgb *= 1.0-0.2*(int(dot(hit_cell, ivec3(1,1,1)))%2);
    }
    else
    {
        discard;
    }

    frag_color *= tint;
    frag_color += highlight;

    frag_color = clamp(frag_color, 0, 1);
}
