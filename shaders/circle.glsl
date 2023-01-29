#program circle_program

///////////////////////////////////////////////////
#shader GL_VERTEX_SHADER

#include "include/header.glsl"

layout(location = 0) in vec3 x;
layout(location = 1) in vec3 X;
layout(location = 2) in float r;
layout(location = 3) in vec4 c;

smooth out vec4 color;
smooth out vec2 uv;
smooth out float radius;

layout(location = 0) uniform mat4 t;

void main()
{
    gl_Position.xyz = (x*r+X);
    gl_Position.w = 1.0;
    gl_Position = t*gl_Position;
    uv = x.xy*r;
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
smooth in float radius;

void main()
{
    frag_color = color;
    float alpha = smoothstep(-radius, -radius+smoothness, -length(uv));
    frag_color.a *= alpha;
    gl_FragDepth = gl_FragCoord.z*alpha;
}

///////////////////////////////////////////////////
