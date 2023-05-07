#shader GL_VERTEX_SHADER

#include "include/header.glsl"

layout(location = 0) in vec3 x;
layout(location = 1) in vec3 X;
layout(location = 2) in vec2 r;
layout(location = 3) in vec4 tint;
layout(location = 4) in float theta;
layout(location = 5) in vec2 l;
layout(location = 6) in vec2 u;

smooth out vec2 uv;
smooth out vec4 color;

layout(location = 0) uniform mat4 t;

vec2 rotate(vec2 a, vec2 b)
{
    return vec2(a.x*b.x-a.y*b.y, a.x*b.y+a.y*b.x);
}

void main()
{
    vec2 rot = vec2(cos(theta), sin(theta));
    gl_Position.xyz = (vec3(rotate(x.xy*r, rot), x.z)+X);
    gl_Position.w = 1.0;
    gl_Position = t*gl_Position;
    vec3 x_flipped = x;
    x_flipped.y = -x_flipped.y;
    uv = mix(l, u, 0.5*x_flipped.xy+0.5);
    color = tint;
}

///////////////////////////////////////////////////
#shader GL_FRAGMENT_SHADER

#include "include/header.glsl"

layout(location = 0) out vec4 frag_color;

layout(location = 1) uniform sampler2D spritesheet;

smooth in vec2 uv;
smooth in vec4 color;

void main()
{
    vec4 sprite_color = texture(spritesheet, uv);
    float gamma = 2.2f;
    sprite_color.rgb = pow(sprite_color.rgb, vec3(gamma));
    frag_color = color*sprite_color;
}
