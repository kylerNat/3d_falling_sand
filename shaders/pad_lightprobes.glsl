#program pad_lightprobes_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"

layout(location = 0) in vec2 x;

layout(location = 0) uniform sampler2D lightprobe_color;
layout(location = 1) uniform sampler2D lightprobe_depth;

smooth out vec2 sample_oct;

void main()
{
    gl_Position.xy = x;
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

#include "include/lightprobe_header.glsl"

void main()
{
    vec2 probe_pos = floor(gl_FragCoord.xy*(1.0f/lightprobe_padded_resolution));
    vec2 oct = (gl_FragCoord.xy-probe_pos*lightprobe_padded_resolution-1.0f)/lightprobe_resolution-0.5f;
    if(abs(oct.x) > 0.5)
        oct.y = -oct.y;
    if(abs(oct.y) > 0.5)
        oct.x = -oct.x;
    ivec2 sample_coord = ivec2(probe_pos*lightprobe_padded_resolution+1.0f
                               +clamp((oct+0.5f)*lightprobe_resolution, 0, lightprobe_resolution-1));
    color = texelFetch(lightprobe_color, sample_coord, 0);
    depth = texelFetch(lightprobe_depth, sample_coord, 0);

    color.a = 1;
    depth.a = 1;
}
