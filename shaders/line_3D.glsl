#shader GL_VERTEX_SHADER

#include "include/header.glsl"

layout(location = 0) in vec3 x;
layout(location = 1) in vec3 X;
layout(location = 2) in float l;
layout(location = 3) in float r;
layout(location = 4) in vec3 a;
layout(location = 5) in vec4 c;

smooth out vec3 start;
smooth out float len;
smooth out float radius;
smooth out vec3 axis;
smooth out vec4 color;

layout(location = 0) uniform mat4 t;

void main()
{
    vec3 a1 = vec3(1,0,0);
    float epsilonsq = 0.000001;
    if(abs(dot(a, a1)) > 1.0-epsilonsq)
    {
        a1 = vec3(0,1,0);
    }
    a1 -= dot(a, a1)*a;
    a1 = normalize(a1);
    vec3 a2 = cross(a, a1);
    vec3 center = X+0.5*l*a;
    gl_Position.xyz = (r*(a1*x.x+a2*x.y)+(0.5*l+r)*a*x.z+center);
    gl_Position.w = 1.0;
    gl_Position = t*gl_Position;
    start = X;
    len = l;
    radius = r;
    axis = a;
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

smooth in vec3 start;
smooth in float len;
smooth in float radius;
smooth in vec3 axis;
smooth in vec4 color;

void main()
{
    float screen_dist = 1.0/tan(fov/2);
    vec2 screen_pos = (2.0f*gl_FragCoord.xy-resolution)/resolution.y;
    vec3 ray_dir = camera_axes*vec3(screen_pos, -screen_dist);
    ray_dir = normalize(ray_dir);

    vec3 d = start-camera_pos;
    vec3 perp = normalize(cross(axis, ray_dir));
    vec3 dplane = d-dot(perp, d)*perp;
    vec3 dplaneperp = dplane-dot(dplane, axis)*axis;
    vec3 planeperpdir = normalize(dplaneperp);
    vec3 ray_point = camera_pos+max(0, dot(dplaneperp, planeperpdir)/dot(ray_dir, planeperpdir))*ray_dir;
    float line_dist = clamp(dot(ray_point-start, axis), 0, len);
    vec3 line_point = start+line_dist*axis;

    d = ray_point-line_point;

    if(dot(d, d) > sq(radius))
    {
        discard;
        return;
    }

    //iteratively find the surface of a sphere inside the torus centered at circle_point to converge to the surface of the torus
    for(int i = 0; i < 5; i++)
    {
        d = ray_point-line_point;
        if(dot(d,d) > sq(radius)) break;
        ray_point -= sqrt(sq(radius)-dot(d,d))*ray_dir;

        float added_dist = clamp(dot(d, axis), 0, len-line_dist);;
        line_point = line_point+added_dist*axis;
        line_dist += added_dist;
    }

    float ray_dist = dot(ray_point-camera_pos, ray_dir);
    ray_dist = max(0, ray_dist);

    frag_color = color;
    // frag_color.rgb *= 5.0f/ray_dist;
    frag_color.rgb = clamp(frag_color.rgb, 0, 1);
    gl_FragDepth = 1.0/ray_dist;
}
