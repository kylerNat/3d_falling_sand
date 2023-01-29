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

#include "include/body_data.glsl"
#define BODY_UPDATE_DATA_BINDING 1
#include "include/body_update_data.glsl"
#define BODY_JOINT_DATA_BINDING 2
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

    if(mat(v0_out) != 0)
    {
        int b0 = joints[j].body_index_0;
        int b1 = joints[j].body_index_1;
        ivec3 texture_lower0 = ivec3(bodies[b0].texture_lower_x, bodies[b0].texture_lower_y, bodies[b0].texture_lower_z);
        ivec3 texture_lower1 = ivec3(bodies[b1].texture_lower_x, bodies[b1].texture_lower_y, bodies[b1].texture_lower_z);
        ivec3 texture_upper0 = ivec3(bodies[b0].texture_upper_x, bodies[b0].texture_upper_y, bodies[b0].texture_upper_z);
        ivec3 texture_upper1 = ivec3(bodies[b1].texture_upper_x, bodies[b1].texture_upper_y, bodies[b1].texture_upper_z);
        ivec3 lower0 = ivec3(bodies[b0].lower_x, bodies[b0].lower_y, bodies[b0].lower_z);
        ivec3 lower1 = ivec3(bodies[b1].lower_x, bodies[b1].lower_y, bodies[b1].lower_z);
        ivec3 upper0 = ivec3(bodies[b0].upper_x, bodies[b0].upper_y, bodies[b0].upper_z);
        ivec3 upper1 = ivec3(bodies[b1].upper_x, bodies[b1].upper_y, bodies[b1].upper_z);

        if(all(equal(upper0-lower0, ivec3(0)))) lower0 = p0-texture_lower0;
        else                                    lower0 = min(lower0, p0-texture_lower0);
        upper0 = max(upper0, p0-texture_lower0+1);

        if(all(equal(upper1-lower1, ivec3(0)))) lower1 = p1-texture_lower1;
        else                                    lower1 = min(lower1, p1-texture_lower1);
        upper1 = max(upper1, p1-texture_lower1+1);

        bodies[b0].lower_x = lower0.x;
        bodies[b0].lower_y = lower0.y;
        bodies[b0].lower_z = lower0.z;
        bodies[b0].upper_x = upper0.x;
        bodies[b0].upper_y = upper0.y;
        bodies[b0].upper_z = upper0.z;

        bodies[b1].lower_x = lower1.x;
        bodies[b1].lower_y = lower1.y;
        bodies[b1].lower_z = lower1.z;
        bodies[b1].upper_x = upper1.x;
        bodies[b1].upper_y = upper1.y;
        bodies[b1].upper_z = upper1.z;
    }
}
