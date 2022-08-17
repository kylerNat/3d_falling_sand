#program blur_lightmap_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"

layout(location = 0) in vec2 x;
layout(location = 1) in vec2 X;

layout(location = 0) uniform sampler2D lightprobe_color;
layout(location = 1) uniform sampler2D lightprobe_depth;

#include "include/lightprobe_header.glsl"

void main()
{
    vec2 scale = 2.0f/vec2(lightprobes_w, lightprobes_h);
    vec2 scale2 = (0.5f*scale*lightprobe_resolution)/lightprobe_padded_resolution;

    gl_Position.xy = scale*(X+0.5)-1+scale2*x;
    gl_Position.z = 0.0f;
    gl_Position.w = 1.0f;
}

/////////////////////////////////////////////////////////////////

#shader GL_FRAGMENT_SHADER
#include "include/header.glsl"

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 depth;

layout(location = 0) uniform sampler2D lightprobe_color;
layout(location = 1) uniform sampler2D lightprobe_depth;
layout(location = 2) uniform sampler2D blue_noise_texture;

int frame_number = 0;
#include "include/blue_noise.glsl"
#include "include/lightprobe_header.glsl"
#include "include/maths.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;

void main()
{
    vec2 raw_coord = (gl_FragCoord.xy-1)/lightprobe_padded_resolution;
    vec2 oct = (lightprobe_padded_resolution*1.0/lightprobe_resolution)*fract(raw_coord);
    ivec2 probe_coord = lightprobe_raw_resolution*ivec2(trunc(raw_coord));
    vec3 center_vec = oct_to_vec(2*oct-1);
    float weight;
    color = vec4(0);
    depth = vec4(0);
    // int blur_radius = 2;
    // for(int y = -blur_radius; y <= blur_radius; y++)
    //     for(int x = -blur_radius; x <= blur_radius; x++)
    for(int i = 0; i < 4; i++)
    {
        // vec2 sample_oct = fract(oct+vec2(x,y)/lightprobe_resolution);
        vec2 sample_oct = fract(oct+0.5*(blue_noise(gl_FragCoord.xy/lightprobe_resolution+vec2(0.31,0.78)*i).xy-0.5)+1);
        ivec2 sample_coord = probe_coord + ivec2(sample_oct*lightprobe_raw_resolution);
        vec3 sample_vec = oct_to_vec(2*sample_oct-1);
        float w = clamp(dot(sample_vec, center_vec), 0, 1);
        color += w*texelFetch(lightprobe_color, sample_coord, 0);
        depth += w*texelFetch(lightprobe_depth, sample_coord, 0);
        weight += w;
    }
    color /= weight;
    depth /= weight;
    color.a = 1;
    depth.a = 1;
}
