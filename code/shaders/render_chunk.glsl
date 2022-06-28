#program render_chunk_program

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
layout(location = 2) uniform isampler3D materials;
layout(location = 3) uniform usampler3D occupied_regions;
layout(location = 4) uniform ivec3 size;
layout(location = 5) uniform ivec3 origin;
layout(location = 6) uniform sampler2D prepass_depth;

#include "include/raycast.glsl"

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

    frag_color = vec4(0,0,0,1);
    vec3 color_multiplier = vec3(1,1,1);

    vec3 pos = camera_pos;
    vec3 ray_sign = sign(ray_dir);

    vec3 invabs_ray_dir = ray_sign/ray_dir;

    float epsilon = 0.02;
    int i = 0;
    float total_dist = 0;

    // // float prepass_jump_dist = max(texture(prepass_depth, 0.5f*screen_pos.xy+0.5f, 0).a-2.0f, 0.0f);
    // float prepass_jump_dist = 10*max_iterations;
    // for(int x = -1; x < 1; x++)
    // for(int y = -1; y < 1; y++)
    // {
    //     prepass_jump_dist = clamp(
    //         texture(prepass_depth, 0.5f*screen_pos.xy+0.5f+0.5f*vec2((x)/360.0f,(y)/180.0f), 0).a-4.0f,
    //         0.0f, prepass_jump_dist);
    // }
    // // {
    // //     frag_color.rgb = vec3(1.0f-prepass_jump_dist/200.0f);
    // //     return;
    // // }
    // pos += prepass_jump_dist*ray_dir;
    // total_dist += prepass_jump_dist;

    float bounding_jump_dist = 0.0;
    if(pos.x > size.x && ray_dir.x < 0) bounding_jump_dist = max(bounding_jump_dist, -epsilon+(size.x-pos.x)/(ray_dir.x));
    if(pos.y < 0 && ray_dir.y > 0)      bounding_jump_dist = max(bounding_jump_dist, -epsilon+(-pos.y)/(ray_dir.y));
    if(pos.y > size.y && ray_dir.y < 0) bounding_jump_dist = max(bounding_jump_dist, -epsilon+(size.y-pos.y)/(ray_dir.y));
    if(pos.z < 0 && ray_dir.z > 0)      bounding_jump_dist = max(bounding_jump_dist, -epsilon+(-pos.z)/(ray_dir.z));
    if(pos.z > size.z && ray_dir.z < 0) bounding_jump_dist = max(bounding_jump_dist, -epsilon+(size.z-pos.z)/(ray_dir.z));

    pos += bounding_jump_dist*ray_dir;
    total_dist += bounding_jump_dist;

    ivec3 ipos = ivec3(floor(pos));

    vec3 hit_pos;
    float hit_dist;
    ivec3 hit_cell;
    vec3 hit_dir;
    vec3 normal;
    cast_ray(ray_dir, pos, hit_pos, hit_dist, hit_cell, hit_dir, normal);
    total_dist += hit_dist;
    if(hit_cell.x != -1)
    {
        vec4 voxel = texelFetch(materials, hit_cell, 0);

        float roughness = 0.0f;
        vec3 emission = vec3(0.02f);
        if(voxel.r == 2)
        {
            emission = vec3(0.05,0.05,0.1);
        }
        if(voxel.r == 4)
        {
            emission = vec3(1.0,1.0,1.0);
        }

        // emission *= 1.0f-total_dist/(512.0f*sqrt(3));

        if(voxel.r == 1)
        {
            color_multiplier *= vec3(0.54,0.44,0.21);
            roughness = 0.9;
        }
        else if(voxel.r == 3)
        {
            color_multiplier *= vec3(0.5,0.5,1.0);
            roughness = 0.1;
        }
        else
        {
            color_multiplier *= vec3(0.1,0.1,0.2);
            roughness = 0.1;
        }

        ivec3 ioutside = ipos - ivec3(hit_dir*ray_sign);
        normal = normalize(normal);

        // frag_color.rgb += color_multiplier*emission*(0.9f+0.1f*normal)*dot(normal, gradient);
        frag_color.rgb += color_multiplier*emission*(0.9f+0.1f*normal);

        gl_FragDepth = 1.0f-total_dist/(512.0f*sqrt(3));

        color_multiplier *= -dot(ray_dir, normal);

        float center_depth = texture(prepass_depth, 0.5f*screen_pos.xy+0.5f, 0).a;

        // vec3 prepass_color = vec3(0);
        // float prepass_total = 0.0f;
        // for(int x = -5; x <= 5; x++)
        //     for(int y = -5; y <= 5; y++)
        //     {
        //         vec4 prepass_value = texture(prepass_depth, 0.5f*screen_pos.xy+0.5f+vec2(x/360.0f,y/180.0f), 0);
        //         vec2 sample_pos = -fract(gl_FragCoord.xy*vec2(1.0f/360.0f,1.0f/180.0f))+vec2(x,y);
        //         float weight = step(-2.0f, -abs(center_depth-prepass_value.a))
        //             *exp(-0.3f*dot(sample_pos, sample_pos));
        //         prepass_color += prepass_value.rgb*weight;
        //         prepass_total += weight;
        //     }
        // prepass_color /= prepass_total;
        // frag_color.rgb += color_multiplier*prepass_color;

        // frag_color.rgb += color_multiplier*texture(prepass_depth, 0.5f*screen_pos.xy+0.5f, 0).rgb;
    }

    // frag_color.rgb = mix(vec3(1,0,0), vec3(0,0,1), i*1.0/20);
    // frag_color.rgb *= clamp(1.0-1.0*total_dist/chunk_size, 0, 1);
}
