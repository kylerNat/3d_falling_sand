#ifndef BODY_JOINT_DATA_BINDING
#define BODY_JOINT_DATA_BINDING 0
#endif

struct body_joint
{
    int type;
    int texture_pos_0_x; int texture_pos_0_y; int texture_pos_0_z;
    int texture_pos_1_x; int texture_pos_1_y; int texture_pos_1_z;
    int axis_0;
    int axis_1;
};

layout(std430, binding = BODY_JOINT_DATA_BINDING) buffer body_joint_data
{
    body_joint joints[];
};
