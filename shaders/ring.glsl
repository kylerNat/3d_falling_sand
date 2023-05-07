#shader GL_VERTEX_SHADER

#include "include/header.glsl"

layout(location = 0) in vec3 x;
layout(location = 1) in vec3 X;
layout(location = 2) in float R;
layout(location = 3) in float r;
layout(location = 4) in vec3 a;
layout(location = 5) in vec4 c;

smooth out vec3 center;
smooth out float major_radius;
smooth out float minor_radius;
smooth out vec3 axis;
smooth out vec4 color;

layout(location = 0) uniform mat4 t;

void main()
{
    vec3 a1 = vec3(1,0,0);
    float epsilonsq = 0.000001;
    if(dot(a, a1) > 1.0-epsilonsq)
    {
        a1 = vec3(0,1,0);
    }
    a1 -= dot(a, a1)*a;
    a1 = normalize(a1);
    vec3 a2 = cross(a, a1);
    gl_Position.xyz = ((R+r)*(a1*x.x+a2*x.y)+r*a*x.z+X);
    gl_Position.w = 1.0;
    gl_Position = t*gl_Position;
    center = X;
    major_radius = R;
    minor_radius = r;
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


smooth in vec3 center;
smooth in float major_radius;
smooth in float minor_radius;
smooth in vec3 axis;
smooth in vec4 color;

void main()
{
    float screen_dist = 1.0/tan(fov/2);
    vec2 screen_pos = (2.0f*gl_FragCoord.xy-resolution)/resolution.y;
    vec3 ray_dir = camera_axes*vec3(screen_pos, -screen_dist);
    ray_dir = normalize(ray_dir);

    //start with a guess point on the line, then iteratively update the line point and circle point to be the closest point to each other on their respective curves
    //working in coordinates shifted so the ring center is at the origin

    //we can start circle_point with any arbitrary point, it will always be on the circle after it's updated
    //check both possible minima

    vec3 circle_point = -major_radius*ray_dir;
    vec3 initial_line_point = camera_pos-center;
    vec3 line_point = initial_line_point;
    vec3 d = vec3(0);

    for(int i = 0; i < 50; i++)
    {
        //update line_point to nearst point on the line to circle_point
        d = line_point-circle_point;
        line_point -= dot(ray_dir, d)*ray_dir;

        //update circle_point to nearst point on the circle to line_point
        d = line_point-circle_point;
        circle_point = major_radius*normalize((line_point-dot(line_point, axis)*axis));
    }
    //if the line_point traveled net backwards, bring it back to the initial point
    if(dot(line_point-initial_line_point, ray_dir) < 0) line_point = initial_line_point;
    d = line_point-circle_point;

    if(dot(d,d) > sq(minor_radius))
    {
        //check the minima that's away from the camera
        circle_point = major_radius*ray_dir;
        line_point = initial_line_point;
        d = vec3(0);

        for(int i = 0; i < 50; i++)
        {
            //update line_point to nearst point on the line to circle_point
            d = line_point-circle_point;
            line_point -= dot(ray_dir, d)*ray_dir;

            //update circle_point to nearst point on the circle to line_point
            d = line_point-circle_point;
            circle_point = major_radius*normalize((line_point-dot(line_point, axis)*axis));

        }

        //if neither minima matches then there is no intersection
        if(dot(d,d) > sq(minor_radius))
        {
            discard;
            return;
        }
    }
    //if the line_point traveled net backwards, bring it back to the initial point
    if(dot(line_point-initial_line_point, ray_dir) < 0) line_point = initial_line_point;
    d = line_point-circle_point;

    //iteratively find the surface of a sphere inside the torus centered at circle_point, this should converge to the torus surface
    //using an ellipsoid might converge faster
    for(int i = 0; i < 5; i++)
    {
        d = line_point-circle_point;
        if(dot(d,d) > sq(minor_radius)) break;
        line_point -= sqrt(sq(minor_radius)-dot(d,d))*ray_dir;

        d = line_point-circle_point;
        circle_point = major_radius*normalize((line_point-dot(line_point, axis)*axis));
    }
    float ray_dist = dot(line_point-initial_line_point, ray_dir);
    ray_dist = max(0, ray_dist);

    frag_color = color;
    // frag_color.rgb *= 5.0f/ray_dist;
    frag_color.rgb = clamp(frag_color.rgb, 0, 1);
    gl_FragDepth = 1.0/ray_dist;
}
