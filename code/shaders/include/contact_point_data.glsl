#ifndef CONTACT_DATA_BINDING
#define CONTACT_DATA_BINDING 0
#endif

struct contact
{
    vec3 x;
    vec3 normal;
    real depth;
    int material;
};

layout(std430, binding = CONTACT_DATA_BINDING) buffer contact_data
{
    int n_contacts;
    contact contacts[];
};
