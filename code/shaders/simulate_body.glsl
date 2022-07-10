#program simulate_body_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"

layout(location = 0) in vec3 x;

layout(location = 0) uniform int layer;
layout(location = 4) uniform int n_bodies;

struct body
{
    int materials_origin_x; int materials_origin_y; int materials_origin_z;
    int size_x; int size_y; int size_z;
    float x_cm_x; float x_cm_y; float x_cm_z;
    float x_x; float x_y; float x_z;
    float x_dot_x; float x_dot_y; float x_dot_z;
    float orientation_r; float orientation_x; float orientation_y; float orientation_z;
    float omega_x; float omega_y; float omega_z;

    float m;

    float I_xx; float I_yx; float I_zx;
    float I_xy; float I_yy; float I_zy;
    float I_xz; float I_yz; float I_zz;

    float invI_xx; float invI_yx; float invI_zx;
    float invI_xy; float invI_yy; float invI_zy;
    float invI_xz; float invI_yz; float invI_zz;
};

ivec3 body_materials_origin;
ivec3 body_size;
vec3  body_x_cm;
vec3  body_x;
vec3  body_x_dot;
vec4  body_orientation;
vec3  body_omega;

layout(std430, binding = 0) buffer body_data
{
    body bodies[];
};

flat out int b;

void main()
{
    b = gl_InstanceID;
    body_materials_origin = ivec3(bodies[b].materials_origin_x,
                                  bodies[b].materials_origin_y,
                                  bodies[b].materials_origin_z);
    body_size = ivec3(bodies[b].size_x,
                      bodies[b].size_y,
                      bodies[b].size_z);

    float scale = 2.0/128.0;

    gl_Position = vec4(0/0,0/0,0,1);

    if(body_materials_origin.z <= layer && layer < body_materials_origin.z+body_size.z)
    {
        gl_Position.xy = scale*(body_materials_origin.xy+body_size.xy*x.xy)-1.0;
    }
}

/////////////////////////////////////////////////////////////////

#shader GL_FRAGMENT_SHADER
#include "include/header.glsl"

layout(location = 0) out ivec4 frag_color;

layout(location = 0) uniform int layer;
layout(location = 1) uniform int frame_number;
layout(location = 2) uniform isampler3D materials;
layout(location = 3) uniform isampler3D body_materials;

struct body
{
    int materials_origin_x; int materials_origin_y; int materials_origin_z;
    int size_x; int size_y; int size_z;
    float x_cm_x; float x_cm_y; float x_cm_z;
    float x_x; float x_y; float x_z;
    float x_dot_x; float x_dot_y; float x_dot_z;
    float orientation_r; float orientation_x; float orientation_y; float orientation_z;
    float omega_x; float omega_y; float omega_z;

    float m;

    float I_xx; float I_yx; float I_zx;
    float I_xy; float I_yy; float I_zy;
    float I_xz; float I_yz; float I_zz;

    float invI_xx; float invI_yx; float invI_zx;
    float invI_xy; float invI_yy; float invI_zy;
    float invI_xz; float invI_yz; float invI_zz;

};

ivec3 body_materials_origin;
ivec3 body_size;
vec3  body_x_cm;
vec3  body_x;
vec3  body_x_dot;
vec4  body_orientation;
vec3  body_omega;

layout(std430, binding = 0) buffer body_data
{
    body bodies[];
};

flat in int b;

vec4 qmult(vec4 a, vec4 b)
{
    return vec4(a.x*b.x-a.y*b.y-a.z*b.z-a.w*b.w,
                a.x*b.y+a.y*b.x+a.z*b.w-a.w*b.z,
                a.x*b.z+a.z*b.x+a.w*b.y-a.y*b.w,
                a.x*b.w+a.w*b.x+a.y*b.z-a.z*b.y);
}

vec3 apply_rotation(vec4 q, vec3 p)
{
    vec4 p_quat = vec4(0, p.x, p.y, p.z);
    vec4 q_out = qmult(qmult(q, p_quat), vec4(q.x, -q.y, -q.z, -q.w));
    return q_out.yzw;
}

uint rand(uint seed)
{
    seed ^= seed<<13;
    seed ^= seed>>17;
    seed ^= seed<<5;
    return seed;
}

float float_noise(uint seed)
{
    return float(int(seed))/1.0e10;
}

void main()
{
    ivec3 pos;
    pos.xy = ivec2(gl_FragCoord.xy);
    pos.z = layer;

    //+,0,-,0
    //0,+,0,-
    // int rot = (frame_number+layer)%4;
    uint rot = rand(rand(rand(frame_number)))%4;
    int i = 0;
    ivec2 dir = ivec2(((rot&1)*(2-rot)), (1-(rot&1))*(1-rot));

    ivec4 c  = texelFetch(body_materials, ivec3(pos.x, pos.y, pos.z), 0);
    ivec4 u  = texelFetch(body_materials, ivec3(pos.x, pos.y, pos.z+1), 0);
    ivec4 d  = texelFetch(body_materials, ivec3(pos.x, pos.y, pos.z-1), 0);
    ivec4 r  = texelFetch(body_materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z), 0);
    ivec4 l  = texelFetch(body_materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z), 0);
    ivec4 f  = texelFetch(body_materials, ivec3(pos.x-dir.y, pos.y+dir.x, pos.z), 0);
    ivec4 ba  = texelFetch(body_materials, ivec3(pos.x+dir.y, pos.y-dir.x, pos.z), 0);

    // c.r = 1;
    frag_color = c;

    const int max_depth = 16;
    if(c.r > 0)
    {
        if(l.r == 0 ||
           r.r == 0 ||
           u.r == 0 ||
           d.r == 0 ||
           f.r == 0 ||
           ba.r == 0) frag_color.g = 0;
        else
        {
            frag_color.g = -max_depth;
            frag_color.g = max(frag_color.g, l.g-1);
            frag_color.g = max(frag_color.g, r.g-1);
            frag_color.g = max(frag_color.g, u.g-1);
            frag_color.g = max(frag_color.g, d.g-1);
            frag_color.g = max(frag_color.g, f.g-1);
            frag_color.g = max(frag_color.g, ba.g-1);
        }

        frag_color.b = c.b/2;
        if(frag_color.r == 3) frag_color.b = 1000;
        // frag_color.b = 0;
    }
    else
    {
        frag_color.g = max_depth;
        frag_color.g = min(frag_color.g, l.g+1);
        frag_color.g = min(frag_color.g, r.g+1);
        frag_color.g = min(frag_color.g, u.g+1);
        frag_color.g = min(frag_color.g, d.g+1);
        frag_color.g = min(frag_color.g, f.g+1);
        frag_color.g = min(frag_color.g, ba.g+1);
    }
    frag_color.g = clamp(frag_color.g,-max_depth,max_depth);

    frag_color.b = 1000;
}