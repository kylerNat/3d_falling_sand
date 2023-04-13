#ifndef RAY_IN_DATA_BINDING
#define RAY_IN_DATA_BINDING 0
#endif

#ifndef RAY_OUT_DATA_BINDING
#define RAY_OUT_DATA_BINDING (RAY_OUT_DATA_BINDING+1)
#endif

struct ray_in
{
    float x; float y; float z;
    float dx; float dy; float dz;
    float max_dist;
    int start_material;
};

struct ray_out
{
    int material;
    int px; int py; int pz;
    float dist;
};

layout(std430, binding = RAY_IN_DATA_BINDING) buffer ray_data_in
{
    ray_in rays_in[];
};

layout(std430, binding = RAY_OUT_DATA_BINDING) buffer ray_data_out
{
    ray_out rays_out[];
};

vec3 ray_x(int i)
{
    return vec3(rays_in[i].x, rays_in[i].y, rays_in[i].z);
}

vec3 ray_d(int i)
{
    return vec3(rays_in[i].dx, rays_in[i].dy, rays_in[i].dz);
}
