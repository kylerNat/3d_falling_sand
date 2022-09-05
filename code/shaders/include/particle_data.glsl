#ifndef PARTICLE_DATA_BINDING
#define PARTICLE_DATA_BINDING 0
#endif

struct particle
{
    uint voxel_data;
    float x;
    float y;
    float z;
    float x_dot;
    float y_dot;
    float z_dot;

    bool alive;
};

#define N_MAX_PARTICLES 4096

layout(std430, binding = PARTICLE_DATA_BINDING) buffer particle_data
{
    int n_dead_particles;
    particle particles[N_MAX_PARTICLES];
    uint dead_particles[N_MAX_PARTICLES];
};

//buffer for particles
//imageStore to put particles back into materials texture
//write to particle buffer in chunk simulation, atomically index particle count
//dead and alive lists
