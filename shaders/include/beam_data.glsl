#ifndef BEAM_DATA_BINDING
#define BEAM_DATA_BINDING 0
#endif

struct beam
{
    float x; float y; float z;
    float dx; float dy; float dz;
    float r;
    float max_length;
    float strength;
};

layout(std430, binding = BEAM_DATA_BINDING) buffer beam_data
{
    beam beams[];
};

vec3 beam_x(int e)
{
    return vec3(beams[e].x, beams[e].y, beams[e].z);
}

vec3 beam_d(int e)
{
    return vec3(beams[e].dx, beams[e].dy, beams[e].dz);
}
