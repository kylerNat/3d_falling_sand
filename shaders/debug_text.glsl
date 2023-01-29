#program debug_text_program

///////////////////////////////////////////////////
#shader GL_VERTEX_SHADER

#include "include/header.glsl"

layout(location = 0) in vec3 x;
layout(location = 1) in vec4 c;

smooth out vec4 color;

void main()
{
    gl_Position.xyz = 5*x/vec3(1080, 720, 1);
    gl_Position.y = -gl_Position.y;
    gl_Position.w = 1.0;
    color = vec4(1);
}

///////////////////////////////////////////////////
#shader GL_FRAGMENT_SHADER

#include "include/header.glsl"

layout(location = 0) out vec4 frag_color;

smooth in vec4 color;

void main()
{
    frag_color = color;
}

///////////////////////////////////////////////////
