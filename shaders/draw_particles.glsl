#program draw_particles_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"
layout(location = 0) in vec3 x;

layout(location = 0) uniform mat4 t;

#include "include/particle_data.glsl"

smooth out vec3 r;

void main()
{
    int p = gl_InstanceID;
    if(!particles[p].alive)
    {
        gl_Position = vec4(0.0/0.0);
        return;
    }
    gl_Position.xyz = x+vec3(particles[p].x, particles[p].y, particles[p].z);
    gl_Position.w = 1.0;
    gl_Position = t*gl_Position;
    r = x;
}

/////////////////////////////////////////////////////////////////

#shader GL_FRAGMENT_SHADER
#include "include/header.glsl"

layout(location = 0) out vec4 frag_color;

smooth in vec3 r;

void main()
{
    frag_color = vec4(0,1,0,1);
}
