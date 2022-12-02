#ifndef EXPLOSION_DATA_BINDING
#define EXPLOSION_DATA_BINDING 0
#endif

struct explosion
{
    float x; float y; float z;
    float r;
    float strength;
};

layout(std430, binding = EXPLOSION_DATA_BINDING) buffer explosion_data
{
    explosion explosions[];
};

vec3 explosion_x(int e)
{
    return vec3(explosions[e].x, explosions[e].y, explosions[e].z);
}
