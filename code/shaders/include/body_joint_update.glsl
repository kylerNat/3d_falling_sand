#ifndef JOINT_UPDATE_BINDING
#define JOINT_UPDATE_BINDING 0
#endif

struct joint_update
{
    int fragment_id0;
    int fragment_id1;
};

layout(std430, binding = JOINT_UPDATE_BINDING) buffer joint_update_data
{
    joint_update joint_updates[];
};
