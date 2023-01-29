#program sprite_program

///////////////////////////////////////////////////
#shader GL_VERTEX_SHADER

#include "include/header.glsl"

layout(location = 0) in vec3 x;
layout(location = 1) in vec3 X;
layout(location = 2) in vec2 r;
layout(location = 3) in vec4 tint;
layout(location = 4) in vec2 l;
layout(location = 5) in vec2 u;

smooth out vec2 uv;
smooth out vec4 color;

layout(location = 0) uniform mat4 t;

void main()
{
    gl_Position.xyz = (x*vec3(r,1.0)+X);
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
    frag_color = color*texture(spritesheet, uv);
}

///////////////////////////////////////////////////
