#program fullscreen_texture_program

#shader GL_VERTEX_SHADER
#include "include/header.glsl"
layout(location = 0) in vec3 x;

layout(location = 0) uniform mat4 t;

smooth out vec2 uv;

void main()
{
    gl_Position.xyz = x;
    gl_Position.w = 1.0;
    uv = 0.5f*x.xy+0.5f;
}

#shader GL_FRAGMENT_SHADER
#include "include/header.glsl"
////////////////<2d circle fragment shader>////////////////
layout(location = 0) out vec4 frag_color;

layout(location = 1) uniform sampler2D color;

smooth in vec2 uv;

void main()
{
    frag_color = texture(color, uv);
}
