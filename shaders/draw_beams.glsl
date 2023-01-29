#program draw_beams_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"
layout(location = 0) in vec2 x;

layout(location = 0) uniform mat4 t;
layout(location = 1) uniform mat3 camera_axes;
layout(location = 2) uniform int n_beams;

#include "include/beam_data.glsl"

void main()
{
    int b = gl_InstanceID;

    vec3 i = beam_d(b);
    vec3 j = normalize(cross(i, camera_axes[2]+vec3(0,0,0.01)));
    j *= beams[b].r;

    vec3 p0 = beam_x(b);

    gl_Position.xyz = p0 + x.x*i + x.y*j;
    gl_Position.w = 1.0;
    gl_Position = t*gl_Position;
}

/////////////////////////////////////////////////////////////////

#shader GL_FRAGMENT_SHADER
#include "include/header.glsl"

layout(location = 0) out vec4 frag_color;

smooth in vec3 r;

void main()
{
    frag_color = vec4(1,0.5,0.5,0.5);
    // gl_FragDepth = ;
}
