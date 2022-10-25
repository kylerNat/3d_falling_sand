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
    if(signed_depth(voxel) < 4)
    {
        vec3 gradient = unnormalized_gradient(materials, ix);
        gradient = normalize(gradient);

        x += 0.1*gradient*(5-signed_depth(voxel));
    }
    else if(signed_depth(voxel) > 5)
    {
        x = mix(x, center_x, 0.1);
    }

    new_x = x;
}
