#ifndef FRAGMENT_DATA_BINDING
#define FRAGMENT_DATA_BINDING 0
#endif

struct fragment
{
    int lower_x; int lower_y; int lower_z;
    int upper_x; int upper_y; int upper_z;
};

layout(std430, binding = FRAGMENT_DATA_BINDING) buffer fragment_data
{
    int n_fragments;
    fragment fragments[];
};
