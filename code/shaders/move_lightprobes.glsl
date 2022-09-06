#program move_lightprobes_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"
layout(location = 0) in vec3 x;

smooth out vec2 uv;

void main()
{
    gl_Position.xyz = x;
    gl_Position.w = 1.0;
}

/////////////////////////////////////////////////////////////////

#shader GL_FRAGMENT_SHADER
#include "include/header.glsl"
#include "include/lightprobe_header.glsl"

layout(location = 0) out vec3 new_x;

layout(location = 0) uniform isampler3D materials;
layout(location = 1) uniform sampler2D old_x;

#include "include/materials_physical.glsl"

void main()
{
    ivec2 probe_coord = ivec2(gl_FragCoord.xy);
    vec3 x = texelFetch(old_x, probe_coord, 0).rgb;

    ivec3 ix = ivec3(x);

    //the center of the current cell
    int j = probe_coord.x+lightprobes_w*probe_coord.y;
    vec3 center_x = vec3((lightprobe_spacing/2)+lightprobe_spacing*(j%lightprobes_per_axis),
                         (lightprobe_spacing/2)+lightprobe_spacing*((j/lightprobes_per_axis)%lightprobes_per_axis),
                         (lightprobe_spacing/2)+lightprobe_spacing*(j/sq(lightprobes_per_axis)));

    ivec4 voxel = texelFetch(materials, ix, 0);
    if(depth(voxel) > SURF_DEPTH-4)
    {
        vec3 gradient = vec3(
            depth(texelFetch(materials, ix+ivec3(-1,0,0), 0))-depth(texelFetch(materials, ix+ivec3(+1,0,0), 0)),
            depth(texelFetch(materials, ix+ivec3(0,-1,0), 0))-depth(texelFetch(materials, ix+ivec3(0,+1,0), 0)),
            depth(texelFetch(materials, ix+ivec3(0,0,-1), 0))-depth(texelFetch(materials, ix+ivec3(0,0,+1), 0))+0.001);
        gradient = normalize(gradient);

        x += 0.1*gradient*(5+depth(voxel)-SURF_DEPTH);
    }
    else if(depth(voxel) < SURF_DEPTH-5)
    {
        x = mix(x, center_x, 0.1);
    }

    new_x = x;
}
