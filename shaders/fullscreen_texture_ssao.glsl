#shader GL_VERTEX_SHADER
#include "include/header.glsl"
layout(location = 0) in vec3 x;

smooth out vec2 uv;

void main()
{
    gl_Position.xyz = x;
    gl_Position.w = 1.0;
    uv = 0.5f*x.xy+0.5f;
}

#shader GL_FRAGMENT_SHADER
#include "include/header.glsl"
///////////////////////////////////////////////////////////
layout(location = 0) out vec4 frag_color;

layout(location = 0) uniform float fov;
layout(location = 1) uniform vec2 resolution;
layout(location = 2) uniform sampler2D color_texture;
layout(location = 3) uniform sampler2D depth_texture;
layout(location = 4) uniform sampler2D normal_texture;
layout(location = 5) uniform sampler2D blue_noise_texture;
layout(location = 6) uniform int frame_number;

#include "include/blue_noise.glsl"

smooth in vec2 uv;

void main()
{
    float screen_dist = 1.0/tan(fov/2);
    vec2 screen_pos = 2*uv-1;
    vec3 ray_dir = vec3(16.0/9.0*screen_pos.x,
                        +        screen_pos.y,
                        -        screen_dist );
    ray_dir = normalize(ray_dir);

    float depth = 1.0/texture(depth_texture, uv).r;
    vec3 normal = texture(normal_texture, uv).rgb;

    vec3 pos = depth*ray_dir;

    float sample_r = 2;
    int n_samples = 4;
    float occlusion = 0.0;
    float max_dist = sample_r*1.5;
    for(int i = 0; i < n_samples; i++)
    {
        vec3 sample_rel = 2*sample_r*(blue_noise_3(uv*resolution/256.0+i*vec2(0.12,0.64))-0.5);
        if(dot(normal, sample_rel) < 0) sample_rel = -sample_rel;
        vec3 sample_pos = pos+sample_rel;
        vec2 sample_uv = 0.5*(-vec2(9.0/16.0, 1.0)*sample_pos.xy*(screen_dist/sample_pos.z))+0.5;
        float sample_depth = 1.0/texture(depth_texture, sample_uv).r;
        float test_depth = length(sample_pos);
        if(sample_depth > test_depth || sample_depth+max_dist < test_depth) occlusion += 1.0/n_samples;
    }

    frag_color = texture(color_texture, uv);
    frag_color.rgb *= occlusion;
}
