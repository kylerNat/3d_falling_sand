#program simulate_particles_program

/////////////////////////////////////////////////////////////////

#shader GL_COMPUTE_SHADER
#include "include/header.glsl"

layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform sampler2D material_physical_properties;
layout(location = 2) uniform usampler3D materials;
layout(location = 3) uniform writeonly uimage3D materials_out;
layout(location = 4) uniform writeonly uimage3D active_regions_out;
layout(location = 5) uniform writeonly uimage3D occupied_regions_out;

#define MATERIAL_PHYSICAL_PROPERTIES
#include "include/materials_physical.glsl"

#include "include/particle_data.glsl"

uint rand(uint seed)
{
    seed ^= seed<<13;
    seed ^= seed>>17;
    seed ^= seed<<5;
    return seed;
}

float float_noise(uint seed)
{
    return float(int(seed))/1.0e10;
}

const int chunk_size = 256;

void main()
{
    uint p = gl_GlobalInvocationID.x;
    if(!particles[p].alive) return;
    vec3 x = vec3(particles[p].x, particles[p].y, particles[p].z);
    vec3 x_dot = vec3(particles[p].x_dot, particles[p].y_dot, particles[p].z_dot);

    x += x_dot;
    x_dot.z -= 0.1;
    int i = 0;
    uint transient = 1;
    while(mat(texelFetch(materials, ivec3(x), 0)) != 0)
    {
        vec3 normal = normalize(unnormalized_gradient(materials, ivec3(x)));
        // x += max(0.1f, float(SURF_DEPTH-depth(voxel)))*normal;
        x += 0.05f*normal;
        x_dot -= dot(x_dot, normal)*normal;

        transient = 0;
        if(i++ > 50)
        {
            break;
        }
    }

    particles[p].x = x.x;
    particles[p].y = x.y;
    particles[p].z = x.z;

    particles[p].x_dot = x_dot.x;
    particles[p].y_dot = x_dot.y;
    particles[p].z_dot = x_dot.z;

    if(transient == 0 && mat(texelFetch(materials, ivec3(x), 0)) == 0)
    {
        uint v = particles[p].voxel_data;
        uvec4 particle_voxel_data = uvec4((v&0xF), ((v>>8)&0x7)|(transient<<7), ((v>>16)&0xF), ((v>>24)&0xF)|7);
        imageStore(materials_out, ivec3(x), particle_voxel_data);
        imageStore(active_regions_out, ivec3(x.x/16, x.y/16, x.z/16), uvec4(1,0,0,0));
        ivec3 cell_x = ivec3(x)%16;
        if(cell_x.x==15) imageStore(active_regions_out, ivec3(x.x/16+1, x.y/16, x.z/16), uvec4(1,0,0,0));
        if(cell_x.x== 0) imageStore(active_regions_out, ivec3(x.x/16-1, x.y/16, x.z/16), uvec4(1,0,0,0));
        if(cell_x.y==15) imageStore(active_regions_out, ivec3(x.x/16, x.y/16+1, x.z/16), uvec4(1,0,0,0));
        if(cell_x.y== 0) imageStore(active_regions_out, ivec3(x.x/16, x.y/16-1, x.z/16), uvec4(1,0,0,0));
        if(cell_x.z==15) imageStore(active_regions_out, ivec3(x.x/16, x.y/16, x.z/16+1), uvec4(1,0,0,0));
        if(cell_x.z== 0) imageStore(active_regions_out, ivec3(x.x/16, x.y/16, x.z/16-1), uvec4(1,0,0,0));
        imageStore(occupied_regions_out, ivec3(x.x/16, x.y/16, x.z/16), uvec4(1,0,0,0));

        particles[p].alive = false;
        int dead_index = atomicAdd(n_dead_particles, 1);
        dead_particles[dead_index] = p;
    }
}
