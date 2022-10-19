#program draw_text_program

///////////////////////////////////////////////////
#shader GL_VERTEX_SHADER

#include "include/header.glsl"

layout(location = 0) in vec2 xy;
layout(location = 1) in vec2 st;

layout(location = 0) uniform vec2 resolution;

smooth out vec2 uv;

void main()
{
    gl_Position.xy = 2*xy/resolution;
    gl_Position.y = -gl_Position.y;
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
    uv = st;
}

///////////////////////////////////////////////////
#shader GL_FRAGMENT_SHADER

#include "include/header.glsl"

layout(location = 0) out vec4 frag_color;

layout(location = 1) uniform vec4 color;
layout(location = 2) uniform sampler2D packed_font;

smooth in vec2 uv;

void main()
{
    frag_color = color*texture(packed_font, uv).r;
}

///////////////////////////////////////////////////
