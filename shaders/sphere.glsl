#shader GL_VERTEX_SHADER

#include "include/header.glsl"

layout(location = 0) in vec3 x;
layout(location = 1) in vec3 X;
layout(location = 2) in float r;
layout(location = 3) in vec4 c;

smooth out vec3 center;
smooth out float radius;
smooth out vec4 color;

layout(location = 0) uniform mat4 t;

void main()
{
    gl_Position.xyz = (r*x+X);
    gl_Position.w = 1.0;
    gl_Position = t*gl_Position;
    center = X;
    radius = r;
    color = c;
}

///////////////////////////////////////////////////
#shader GL_FRAGMENT_SHADER

#include "include/header.glsl"

layout(location = 0) out vec4 frag_color;

layout(location = 1) uniform mat3 camera_axes;
layout(location = 2) uniform vec3 camera_pos;
layout(location = 3) uniform float fov;
layout(location = 4) uniform vec2 resolution;


smooth in vec3 center;
smooth in float radius;
smooth in vec4 color;

void main()
{
    float screen_dist = 1.0/tan(fov/2);
    vec2 screen_pos = (2.0f*gl_FragCoord.xy-resolution)/resolution.y;
    vec3 ray_dir = camera_axes*vec3(screen_pos, -screen_dist);
    ray_dir = normalize(ray_dir);

    vec3 d = camera_pos-center;
    d += max(0, -dot(ray_dir, d))*ray_dir;

    float distsq = dot(d, d);
    if(distsq > sq(radius))
    {
        discard;
        return;
    }
    float ray_dist = dot(center-camera_pos, ray_dir)-sqrt(sq(radius)-distsq);
    ray_dist = max(0, ray_dist);

    frag_color = color;
    // frag_color.rgb *= 5.0f/ray_dist;
    frag_color.rgb = clamp(frag_color.rgb, 0, 1);
    gl_FragDepth = 1.0/ray_dist;
}
