#ifndef BODY_UPDATE_DATA_BINDING
#define BODY_UPDATE_DATA_BINDING 0
#endif

struct body_update
{
    float m;
    float I_xx;
    float I_xy; float I_yy;
    float I_xz; float I_yz; float I_zz;
    float x_cm_x; float x_cm_y; float x_cm_z;

    int lower_x; int lower_y; int lower_z;
    int upper_x; int upper_y; int upper_z;
};

layout(std430, binding = BODY_UPDATE_DATA_BINDING) buffer body_update_data
{
    body_update body_updates[];
};
