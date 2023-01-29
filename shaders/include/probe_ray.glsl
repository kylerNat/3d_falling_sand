#ifndef PROBE_RAY_DATA_BINDING
#define PROBE_RAY_DATA_BINDING 0
#endif

struct probe_ray
{
    vec3 rel_hit_pos;
    vec3 hit_color;
};

layout(std430, binding = PROBE_RAY_DATA_BINDING) buffer probe_ray_data
{
    probe_ray probe_rays[];
};
