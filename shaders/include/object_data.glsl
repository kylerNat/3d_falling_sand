#ifndef OBJECT_DATA_BINDING
#define OBJECT_DATA_BINDING 0
#endif

struct object
{
    int lower_x; int lower_y; int lower_z;
    int upper_x; int upper_y; int upper_z;

    int origin_to_lower_x; int origin_to_lower_y; int origin_to_lower_z;

    float x_x; float x_y; float x_z;
    float orientation_r; float orientation_x; float orientation_y; float orientation_z;

    float tint_r; float tint_g; float tint_b; float tint_a;
    float highlight_r; float highlight_g; float highlight_b; float highlight_a;
};

layout(std430, binding = OBJECT_DATA_BINDING) buffer object_data
{
    object objects[];
};
