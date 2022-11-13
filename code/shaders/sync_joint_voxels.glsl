#program sync_joint_voxels_program

/////////////////////////////////////////////////////////////////

#shader GL_COMPUTE_SHADER
#include "include/header.glsl"
#include "include/maths.glsl"

#define localgroup_size 4
#define subgroup_size 1
layout(local_size_x = localgroup_size, local_size_y = 1, local_size_z = 1) in;

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform sampler2D material_physical_properties;
layout(location = 2, rgba8ui) uniform uimage3D body_materials;
layout(location = 3) uniform int n_joints;

#define MATERIAL_PHYSICAL_PROPERTIES
#include "include/materials_physical.glsl"

#include "include/body_joint_data.glsl"

void main()
{
    uint j = gl_GlobalInvocationID.x;
    if(j >= n_joints) return;

    ivec3 p0 = ivec3(joints[j].texture_pos_0_x,joints[j].texture_pos_0_y,joints[j].texture_pos_0_z);
    ivec3 p1 = ivec3(joints[j].texture_pos_1_x,joints[j].texture_pos_1_y,joints[j].texture_pos_1_z);
    uvec4 v0 = imageLoad(body_materials, p0);
    uvec4 v1 = imageLoad(body_materials, p1);

    uvec4 v0_out;
    v0_out.r = max(mat(v0), mat(v1));
    v0_out.g = (max(phase(v0), phase(v1))<<5);
    v0_out.b = max(temp(v0), temp(v1));
    v0_out.a = (max(volt(v0), volt(v1)));

    uvec4 v1_out = v0_out;

    v0_out.g |= depth(v0);
    v1_out.g |= depth(v1);

    imageStore(body_materials, p0, v0_out);
    imageStore(body_materials, p1, v1_out);
}
