#program rectangle_program

///////////////////////////////////////////////////
#shader GL_VERTEX_SHADER

#include "include/header.glsl"

layout(location = 0) in vec3 x;
layout(location = 1) in vec3 X;
layout(location = 2) in vec2 r;
layout(location = 3) in vec4 c;

layout(location = 0) uniform mat4 t;
layout(location = 1) uniform float smoothness;

smooth out vec4 color;
smooth out vec2 uv;
smooth out vec2 radius;

void main()
{
    uv = x.xy*(r+2.0*smoothness);
    gl_Position.xy = uv+X.xy;
    // gl_Position.z = x.z+X.z;
    gl_Position.z = 0;
    gl_Position.w = 1.0;
    gl_Position = t*gl_Position;
    radius = r;
    color = c;
}

///////////////////////////////////////////////////
#shader GL_FRAGMENT_SHADER

#include "include/header.glsl"

layout(location = 0) out vec4 frag_color;

layout(location = 1) uniform float smoothness;

smooth in vec4 color;
smooth in vec2 uv;
smooth in vec2 radius;

void main()
{
    frag_color = color;
    frag_color.a *= smoothstep(-radius.x, -radius.x+smoothness, -abs(uv.x));
    frag_color.a *= smoothstep(-radius.y, -radius.y+smoothness, -abs(uv.y));
}

///////////////////////////////////////////////////
