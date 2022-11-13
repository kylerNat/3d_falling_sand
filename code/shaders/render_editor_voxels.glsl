#program render_editor_voxels_program

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

layout(location = 0) uniform mat3 camera_axes;
layout(location = 1) uniform vec3 camera_pos;
layout(location = 2) uniform sampler2D material_visual_properties;
layout(location = 3) uniform isampler3D form_materials;
layout(location = 4) uniform sampler2D blue_noise_texture;
layout(location = 5) uniform int frame_number;
layout(location = 6) uniform int n_forms;
layout(location = 7) uniform int highlight_form;
layout(location = 8) uniform ivec3 highlight_cell;

// #define DEBUG_DOTS
#include "include/maths.glsl"
#include "include/blue_noise.glsl"
#include "include/form_data.glsl"
#include "include/materials.glsl"
#include "include/materials_physical.glsl"
#define ACTIVE_REGIONS
#define RAY_CAST_IGNORE_DEPTH
#include "include/raycast.glsl"

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
    int hit_form = -2;
    bool hit = false;

    for(int f = 0; f < n_forms; f++)
    {
        ivec3 form_x_origin = ivec3(forms[f].x_origin_x, forms[f].x_origin_y, forms[f].x_origin_z);
        vec3 form_x = vec3(forms[f].x_x, forms[f].x_y, forms[f].x_z);
        vec4 form_orientation = vec4(forms[f].orientation_r, forms[f].orientation_x, forms[f].orientation_y, forms[f].orientation_z);
        ivec3 form_lower = ivec3(forms[f].lower_x,
                                 forms[f].lower_y,
                                 forms[f].lower_z);
        ivec3 form_upper = ivec3(forms[f].upper_x,
                                 forms[f].upper_y,
                                 forms[f].upper_z);
        ivec3 form_size = form_upper-form_lower;
        ivec3 form_origin = ivec3(forms[f].materials_origin_x,
                                  forms[f].materials_origin_y,
                                  forms[f].materials_origin_z);

        //ray info in the forms frame
        vec3 form_pos = apply_rotation(conjugate(form_orientation), pos-form_x) + form_x_origin - form_lower;
        ivec3 iform_pos = ivec3(floor(form_pos));
        vec3 form_ray_dir = apply_rotation(conjugate(form_orientation), ray_dir);

        float form_jump_dist = 0.0;
        if(iform_pos.x < 0            && form_ray_dir.x > 0) form_jump_dist = max(form_jump_dist, epsilon+(-form_pos.x)/(form_ray_dir.x));
        if(iform_pos.x >= form_size.x && form_ray_dir.x < 0) form_jump_dist = max(form_jump_dist, epsilon+(form_size.x-form_pos.x)/(form_ray_dir.x));
        if(iform_pos.y < 0            && form_ray_dir.y > 0) form_jump_dist = max(form_jump_dist, epsilon+(-form_pos.y)/(form_ray_dir.y));
        if(iform_pos.y >= form_size.y && form_ray_dir.y < 0) form_jump_dist = max(form_jump_dist, epsilon+(form_size.y-form_pos.y)/(form_ray_dir.y));
        if(iform_pos.z < 0            && form_ray_dir.z > 0) form_jump_dist = max(form_jump_dist, epsilon+(-form_pos.z)/(form_ray_dir.z));
        if(iform_pos.z >= form_size.z && form_ray_dir.z < 0) form_jump_dist = max(form_jump_dist, epsilon+(form_size.z-form_pos.z)/(form_ray_dir.z));

        form_pos += form_jump_dist*form_ray_dir;

        vec3 form_hit_pos;
        float form_hit_dist;
        ivec3 form_hit_cell;
        vec3 form_hit_dir;
        vec3 form_normal;
        uvec4 form_voxel;
        bool form_hit = cast_ray(form_materials, form_ray_dir, form_pos, form_size, form_origin+form_lower, 0, form_hit_pos, form_hit_dist, form_hit_cell, form_hit_dir, form_normal, form_voxel, 100);
        // if(form_hit && (!hit || form_jump_dist+form_hit_dist < hit_dist) && b != 13 && b != 12)
        if(form_hit && (!hit || form_jump_dist+form_hit_dist < hit_dist))
        {
            hit = true;
            hit_pos = apply_rotation(form_orientation, form_hit_pos-form_x_origin)+form_x;
            hit_dist = form_jump_dist+form_hit_dist;
            hit_cell = form_hit_cell;
            hit_dir = form_hit_dir;
            hit_form = f;
            normal = apply_rotation(form_orientation, form_normal);
            voxel = form_voxel;
            if(voxel.r >= BASE_CELL_MAT) voxel.r += forms[f].cell_material_id;
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

        float light_value = 0.5*dot(reflection_dir, spotlight_dir)+0.5+0.2;
        light_value *= 0.5*dot(camera_pos, camera_pos)/sq(total_dist);

        frag_color.rgb += fr(material_id, reflection_dir, -ray_dir, normal)*light_value;
    }
    else
    {
        discard;
    }

    if(hit_form == highlight_form && hit_cell == highlight_cell)
    {
        frag_color.rgb = clamp(frag_color.rgb, 0, 1);
        frag_color.rgb += vec3(0.5,0.5,0.5);
    }

    frag_color.rgb = clamp(frag_color.rgb, 0, 1);
}
