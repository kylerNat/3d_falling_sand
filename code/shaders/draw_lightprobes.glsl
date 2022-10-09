#program draw_lightprobes_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"
layout(location = 0) in vec3 x;

layout(location = 0) uniform mat4 t;
layout(location = 3) uniform sampler2D lightprobe_x;

#include "include/lightprobe_header.glsl"

smooth out vec3 r;
flat out ivec2 probe_coord;

void main()
{
    probe_coord = ivec2(gl_InstanceID%lightprobes_w, gl_InstanceID/lightprobes_w);
    gl_Position.xyz = x+texelFetch(lightprobe_x, probe_coord, 0).xyz;
    gl_Position.w = 1.0;
    gl_Position = t*gl_Position;
    r = x;
}

/////////////////////////////////////////////////////////////////

#shader GL_FRAGMENT_SHADER
#include "include/header.glsl"

layout(location = 0) out vec4 frag_color;

layout(location = 1) uniform sampler2D lightprobe_color;
layout(location = 2) uniform sampler2D lightprobe_depth;
layout(location = 3) uniform sampler2D lightprobe_x;

#include "include/lightprobe_header.glsl"

vec3 sign_not_zero(vec3 p) {
    return 2*step(0, p)-1;
}

vec2 vec_to_oct(vec3 p)
{
    vec3 sign_p = sign_not_zero(p);
    vec2 oct = p.xy * (1.0f/dot(p, sign_p));
    return (p.z > 0) ? oct : (1.0f-abs(oct.yx))*sign_p.xy;
}

vec3 oct_to_vec(vec2 oct)
{
    vec2 sign_oct = sign(oct);
    vec3 p = vec3(oct.xy, 1.0-dot(oct, sign_oct));
    if(p.z < 0) p.xy = (1.0f-abs(p.yx))*sign_oct.xy;
    return normalize(p);
}

smooth in vec3 r;
flat in ivec2 probe_coord;

void main()
{
    vec2 sample_oct = vec_to_oct(normalize(r));

    vec2 sample_coord = vec2(lightprobe_padded_resolution*probe_coord+1.5)+lightprobe_resolution*clamp(0.5f*sample_oct+0.5f,0,1);
    vec2 t = trunc(sample_coord);
    vec2 f = fract(sample_coord);
    f = f*f*f*(f*(f*6.0-15.0)+10.0);
    // f = f*f*(-2*f+3);
    float l = 1-sq(f.x-0.5)*sq(f.y-0.5);
    sample_coord = t+f-0.5;
    sample_coord *= vec2(1.0f/lightprobe_resolution_x, 1.0f/lightprobe_resolution_y);
    frag_color = textureLod(lightprobe_color, sample_coord, l);
    // frag_color.rgb = vec3(textureLod(lightprobe_depth, sample_coord, l).r/16.0);
    frag_color.a = 1.0;
}
