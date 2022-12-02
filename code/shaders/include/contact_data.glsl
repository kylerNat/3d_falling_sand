#ifndef CONTACT_DATA_BINDING
#define CONTACT_DATA_BINDING 0
#endif

struct contact_point
{
    int b0;
    int b1;
    uint material0;
    uint material1;
    float depth0;
    float depth1;
    float p0_x; float p0_y; float p0_z;
    float p1_x; float p1_y; float p1_z;
    float x_x; float x_y; float x_z;
    float n_x; float n_y; float n_z;
    float f_x; float f_y; float f_z;
};

layout(std430, binding = CONTACT_DATA_BINDING) buffer contact_data
{
    int n_contacts;
    contact_point contacts[];
};
