#shader GL_VERTEX_SHADER

#include "include/header.glsl"

layout(location = 0) in vec3 X;

layout(location = 0) uniform mat4 t;

smooth out vec3 x;

void main()
{
    gl_Position.xyz = X;
    gl_Position.w = 1.0;
    gl_Position = t*gl_Position;
    x = X;
}

///////////////////////////////////////////////////
#shader GL_FRAGMENT_SHADER

#include "include/header.glsl"

layout(location = 0) out vec4 frag_color;

smooth in vec3 x;

layout(location = 1) uniform float smoothness;
layout(location = 2) uniform sampler2D points;
layout(location = 3) uniform int n_points;

const int N_MAX_POINTS = 4096;

vec3 dist_to_segment(int segment_id)
{
    vec4 a = texelFetch(points, ivec2(0, segment_id), 0); //xyz radius
    vec4 b = texelFetch(points, ivec2(0, segment_id+1), 0); //xyz radius
    vec3 v = b.xyz-a.xyz;

    //calculate nearest point to the central line
    float v_sq = dot(v, v);
    float t = dot(x-a.xyz, v)/v_sq;
    float r = mix(a.w, b.w, t);
    vec3 x_rel = x - mix(a.xyz, b.xyz, t);

    //adjust point to be nearest to the line accounting for the thickness
    float Deltar = b.w-a.w;
    if(v_sq < Deltar*Deltar) {
        t = (Deltar < 0.0) ? 0.0 : 1.0;
    } else {
        t += dot(x_rel, x_rel)*Deltar/(v_sq*r);
        t = clamp(t, 0.0, 1.0);
    }

    //recalculate for new point
    r = mix(a.w, b.w, t);
    x_rel = x - mix(a.xyz, b.xyz, t);
    float distance = length(x_rel);
    return vec3(r, t, distance);
}

void main()
{
    //find global min distance
    vec3 dist = dist_to_segment(0);
    int best_id = 0;
    vec3 test_dist;

    for(int test_id = 1; test_id <= N_MAX_POINTS-1; test_id++) {
        if(test_id >= n_points-1) break;
        test_dist = dist_to_segment(test_id);
        if(dist.r*test_dist.z <= test_dist.r*dist.z) {
            dist = test_dist;
            best_id = test_id;
        }
    }

    vec4 ca = texelFetch(points, ivec2(1, best_id), 0);
    vec4 cb = texelFetch(points, ivec2(1, best_id+1), 0);

    vec4 c = mix(ca, cb, dist.t); //NOTE: could probably get this automatically by sampling the texture at the right point with interpolation enabled

    float alpha = smoothstep(-dist.r, -dist.r+smoothness, -dist.z);
    frag_color = c;
    frag_color.a *= alpha;
    gl_FragDepth = gl_FragCoord.z*alpha;
}
