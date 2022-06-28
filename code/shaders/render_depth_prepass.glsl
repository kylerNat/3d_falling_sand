#program render_depth_prepass_program

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

layout(location = 0) out vec4 depth;

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform mat3 camera_axes;
layout(location = 2) uniform vec3 camera_pos;
layout(location = 3) uniform isampler3D materials;
layout(location = 4) uniform usampler3D occupied_regions;
layout(location = 5) uniform ivec3 size;
layout(location = 6) uniform ivec3 origin;
layout(location = 7) uniform sampler2D blue_noise_texture;

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

vec4 blue_noise(vec2 co)
{
    return fract(texture(blue_noise_texture, co)+PHI*frame_number);
}

void main()
{
    float fov = pi*120.0/180.0;
    float screen_dist = 1.0/tan(fov/2);
    vec2 sample_pos = screen_pos.xy;
    sample_pos += (blue_noise(gl_FragCoord.xy/256.0f+vec2(0.319f, 0.712f)).xy-0.5f)/vec2(360, 180);
    vec3 ray_dir = (16.0/9.0*sample_pos.x*camera_axes[0]
                    +        sample_pos.y*camera_axes[1]
                    -        screen_dist *camera_axes[2]);
    ray_dir = normalize(ray_dir);

    vec3 pos = camera_pos;
    vec3 ray_sign = sign(ray_dir);

    vec3 invabs_ray_dir = ray_sign/ray_dir;

    vec3 hit_dir = vec3(0);

    float epsilon = 0.02;
    int max_iterations = 200;
    int i = 0;
    float total_dist = 0;

    if(pos.x < 0 && ray_dir.x > 0)      total_dist = max(total_dist, -epsilon+(-pos.x)/(ray_dir.x));
    if(pos.x > size.x && ray_dir.x < 0) total_dist = max(total_dist, -epsilon+(size.x-pos.x)/(ray_dir.x));
    if(pos.y < 0 && ray_dir.y > 0)      total_dist = max(total_dist, -epsilon+(-pos.y)/(ray_dir.y));
    if(pos.y > size.y && ray_dir.y < 0) total_dist = max(total_dist, -epsilon+(size.y-pos.y)/(ray_dir.y));
    if(pos.z < 0 && ray_dir.z > 0)      total_dist = max(total_dist, -epsilon+(-pos.z)/(ray_dir.z));
    if(pos.z > size.z && ray_dir.z < 0) total_dist = max(total_dist, -epsilon+(size.z-pos.z)/(ray_dir.z));

    pos += total_dist*ray_dir;

    ivec3 ipos = ivec3(floor(pos));

    int bounces_remaining = 5;
    bool first_hit = true;
    vec3 color_multiplier = vec3(1,1,1);

    depth = vec4(0.0);

    // {
    //     vec3 dist = ((0.5*ray_sign+0.5)*size-ray_sign*pos)*invabs_ray_dir;
    //     float min_dist = dist.x;
    //     int min_dir = 0;
    //     if(dist.y < min_dist) {
    //         min_dist = dist.y;
    //         min_dir = 1;
    //     }
    //     if(dist.z < min_dist) {
    //         min_dist = dist.z;
    //         min_dir = 2;
    //     }
    //     ivec3 max_displacement = ivec3(ceil(abs(min_dist*ray_dir)));
    //     max_iterations = max_displacement.x+max_displacement.y+max_displacement.z;
    // }
    // max_iterations = min(max_iterations, 100);
    // max_iterations = max(max_iterations, 100);

    for(;;)
    {
        if(ipos.x < -1 || ipos.y < -1 || ipos.z < -1
           || ipos.x > size.x || ipos.y > size.y || ipos.z > size.z)
        {
            return;
        }
        while(texelFetch(occupied_regions, ipos/16, 0).r == 0)
        {
            vec3 dist = ((0.5f*ray_sign+0.5f)*16.0f+ray_sign*(16.0f*floor(pos/16.0f)-pos))*invabs_ray_dir;
            vec3 min_dir = step(dist.xyz, dist.zxy)*step(dist.xyz, dist.yzx);
            float min_dist = dot(dist, min_dir);
            hit_dir = min_dir;

            float step_dist = min_dist+epsilon;
            pos += step_dist*ray_dir;
            total_dist += step_dist;
            // hit_dir = min_dir;

            ipos = ivec3(floor(pos));

            if(ipos.x < -1 || ipos.y < -1 || ipos.z < -1
               || ipos.x > size.x || ipos.y > size.y || ipos.z > size.z)
            {
                return;
            }

            if(++i >= max_iterations)
            {
                return;
            }
        }

        ivec4 voxel = texelFetch(materials, ipos+origin, 0);
        if(voxel.r != 0)
        {
            float roughness = 0.0f;
            vec3 emission = vec3(0.0f);
            if(voxel.r == 2)
            {
                emission = vec3(0.05,0.05,0.1);
            }
            if(voxel.r == 4)
            {
                emission = vec3(1.0,1.0,1.0);
            }

            if(voxel.r > 1)
            {
                roughness = 0.8;
            }
            else
            {
                roughness = 0.9;
            }

            ivec3 ioutside = ipos - ivec3(hit_dir*ray_sign);
            vec3 gradient = vec3(
                texelFetch(materials, ioutside+ivec3(1,0,0),0).g-texelFetch(materials, ioutside+ivec3(-1,0,0),0).g,
                texelFetch(materials, ioutside+ivec3(0,1,0),0).g-texelFetch(materials, ioutside+ivec3(0,-1,0),0).g,
                texelFetch(materials, ioutside+ivec3(0,0,1),0).g-texelFetch(materials, ioutside+ivec3(0,0,-1),0).g+0.001f
                );
            vec3 normal = gradient;
            normal += roughness*(blue_noise(gl_FragCoord.xy/256.0).rgb-0.5f);
            normal = normalize(normal);
            gradient = normalize(gradient);

            depth.rgb += color_multiplier*emission;

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

            color_multiplier *= -dot(ray_dir, normal);

            if(first_hit)
            {
                color_multiplier = vec3(1,1,1);
                depth.rgb = vec3(0,0,0);
                depth.a = total_dist;
                first_hit = false;
            }
            if(bounces_remaining-- <= 0)
            {
                return;
            }
            else
            {
                ray_dir -= 2*dot(ray_dir, normal)*normal;
                ray_sign = sign(ray_dir);

                invabs_ray_dir = ray_sign/ray_dir;
            }
        }

        if(voxel.g >= 3)
        {
            float skip_dist = (voxel.g-2)/dot(ray_dir,ray_sign);
            pos += ray_dir*skip_dist;
            total_dist += skip_dist;
            ipos = ivec3(floor(pos));
        }

        vec3 dist = (0.5f*ray_sign+0.5f+ray_sign*(vec3(ipos)-pos))*invabs_ray_dir;

        vec3 min_dir = step(dist.xyz, dist.zxy)*step(dist.xyz, dist.yzx);
        float min_dist = dot(dist, min_dir);
        pos += min_dist*ray_dir;
        ipos += ivec3(min_dir*ray_sign);
        total_dist += min_dist;
        hit_dir = min_dir;

        // if(first_hit)
        // {^all that stuff}
        // else
        // {
        //     pos += ray_dir*coarse_step;
        //     total_dist += coarse_step;
        //     ipos = ivec3(floor(pos));
        //     coarse_step += 1.0f*coarse_step*(float_noise(pos.xy+pos.zz));
        // }

        if(++i >= max_iterations)
        {
            return;
        }
    }
}
