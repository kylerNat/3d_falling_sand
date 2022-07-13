#program simulate_body_physics_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"

layout(location = 0) in vec3 x;

smooth out vec2 uv;

void main()
{
    gl_Position.xyz = x;
    gl_Position.w = 1.0;
    uv = 0.5*x.xy+0.5;
}

/////////////////////////////////////////////////////////////////

#shader GL_FRAGMENT_SHADER
#include "include/header.glsl"

layout(location = 0) out vec3 deltax_dot;
layout(location = 1) out vec3 deltaomega;
layout(location = 2) out vec3 pseudo_x_dot;
layout(location = 3) out vec3 pseudo_omega;

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform isampler3D materials;
layout(location = 2) uniform isampler3D body_materials;
layout(location = 3) uniform sampler3D body_forces;
layout(location = 4) uniform sampler3D body_shifts;

#include "include/body_data.glsl"

#include "include/maths.glsl"

smooth in vec2 uv;

const int chunk_size = 256;

ivec4 voxelFetch(isampler3D tex, ivec3 coord)
{
    return texelFetch(materials, coord, 0);
}

ivec4 bodyVoxelFetch(int body_index, ivec3 coord)
{
    return texelFetch(body_materials, body_materials_origin+coord, 0);
}

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

#define N_MAX_COLLISION_POINTS 256
void find_collision_points(out vec3 world_collision_points[N_MAX_COLLISION_POINTS], out ivec3 world_collision_coord[N_MAX_COLLISION_POINTS], out int n_collision_points)
{
    n_collision_points = 0;

    //TODO: maybe use space filling curves
    for(int test_z = 0; test_z < body_size.z; test_z+=1)
        for(int test_y = 0; test_y < body_size.y; test_y+=1)
            for(int test_x = 0; test_x < body_size.x; test_x+=1)
            {

                ivec3 body_coord = ivec3(test_x, test_y, test_z);
                vec3 world_coord = apply_rotation(body_orientation, vec3(body_coord)+0.5-body_x_cm)+body_x;

                vec4 body_voxel = texelFetch(body_materials, body_materials_origin+body_coord, 0);

                // if(body_voxel.g == 0 && body_voxel.r > 0)
                if(body_voxel.g == 0)
                {
                    ivec3 wvc = ivec3(world_coord); //world_voxel_coord
                    ivec4 world_voxel = voxelFetch(materials, wvc);
                    vec3 rel_pos = vec3(wvc)+0.5-world_coord;
                    // if(world_voxel.r > 0 && dot(rel_pos, rel_pos) <= 1.0)
                    if(world_voxel.g <= 1)
                    {
                        world_collision_points[n_collision_points] = world_coord;
                        world_collision_coord[n_collision_points] = wvc;
                        n_collision_points++;
                        if(n_collision_points >= N_MAX_COLLISION_POINTS) return;
                    }

                    // for(int wz = 0; wz <= 1; wz++)
                    //     for(int wy = 0; wy <= 1; wy++)
                    //         for(int wx = 0; wx <= 1; wx++)
                    //         {
                    //             ivec3 wvc = ivec3(world_coord-0.5)+ivec3(wx,wy,wz); //world_voxel_coord
                    //             ivec4 world_voxel = voxelFetch(materials, wvc);
                    //             vec3 rel_pos = vec3(wvc)+0.5-world_coord;
                    //             // if(world_voxel.r > 0 && dot(rel_pos, rel_pos) <= 1.0)
                    //             if(world_voxel.g <= 2)
                    //             {
                    //                 world_collision_points[n_collision_points] = world_coord;
                    //                 world_collision_coord[n_collision_points] = wvc;
                    //                 n_collision_points++;
                    //                 if(n_collision_points >= N_MAX_COLLISION_POINTS) return;
                    //             }
                    //         }
                }

                test_x += int(max(abs(body_voxel.g)-1, 0));
            }
}

void main()
{
    int b = int(gl_FragCoord.y);
    // int b = 0;

    get_body_data(b);

    deltax_dot = vec3(0,0,0);
    deltaomega = vec3(0,0,0);

    pseudo_x_dot = vec3(0,0,0);
    pseudo_omega = vec3(0,0,0);

    //gravity
    deltax_dot.z += -0.1;

    vec3 world_collision_points[N_MAX_COLLISION_POINTS];
    ivec3 world_collision_coord[N_MAX_COLLISION_POINTS];
    int n_collision_points = 0;

    find_collision_points(world_collision_points, world_collision_coord, n_collision_points);

    // if(n_collision_points > 0 && length(body_omega) < 0.01 && n_collision_points > 0 && length(body_x_dot) < 0.01)
    // {
    //     deltaomega = -0.05f*body_omega;
    //     deltax_dot = -0.05f*body_x_dot;
    //     // return;
    // }

    vec3 impulses[N_MAX_COLLISION_POINTS] = vec3[N_MAX_COLLISION_POINTS](0);

    for(int i = 0; i < 6; i++)
    {
        vec3 x_dot = body_x_dot+deltax_dot;
        vec3 omega = body_omega+deltaomega;

        const int n_test_points = 5;
        const int max_steps = 20;

        int deepest_x;
        int deepest_y;
        int deepest_z;
        int deepest_depth = 0;
        bool point_found = false;

        // float old_E = m*dot(x_dot, x_dot)+body_I*dot(omega, omega);

        vec3 best_deltax_dot = vec3(0,0,0);
        vec3 best_deltax = vec3(0,0,0);
        vec3 best_r = vec3(0,0,0);
        int best_cp = -1;
        int n_collisions = 0;
        float best_E = 2*(body_m*dot(x_dot, x_dot)+dot(body_I*omega, omega));

        float COR = 0.2;
        float COF = 0.1;
        for(int cp = 0; cp < n_collision_points; cp++)
        {
            vec3 world_coord = world_collision_points[cp];
            ivec3 wvc = world_collision_coord[cp]; //world_voxel_coord
            ivec4 world_voxel = voxelFetch(materials, wvc);
            vec3 rel_pos = vec3(wvc)+0.5-world_coord;

            vec3 world_gradient = vec3(
                voxelFetch(materials, wvc+ivec3(1,0,0)).g-voxelFetch(materials, wvc+ivec3(-1,0,0)).g,
                voxelFetch(materials, wvc+ivec3(0,1,0)).g-voxelFetch(materials, wvc+ivec3(0,-1,0)).g,
                voxelFetch(materials, wvc+ivec3(0,0,1)).g-voxelFetch(materials, wvc+ivec3(0,0,-1)).g+0.001
                );

            vec3 normal = normalize(world_gradient);
            // vec3 a = wvc-body_x;

            // if(length(omega) - sq(dot(omega, normal)) < 0.01
            //    && dot(x_dot, normal) < 0.01) COR = 0;

            vec3 a = world_coord-body_x;

            float u = dot(x_dot+cross(omega, a), normal);
            float K = 1.0+body_m*dot(cross(body_invI*cross(a, normal), a), normal);
            vec3 test_deltax_dot = vec3(0,0,0);
            // if(u < 0)
            {
                //normal force
                test_deltax_dot = (-(1.0+COR)*u/K)*normal;
                //friction
                test_deltax_dot += -COF*(-(1.0+COR)*u/K)*(x_dot+cross(omega, a)-u*normal);
            }
            if(dot(test_deltax_dot+impulses[cp], normal) < 0) test_deltax_dot = vec3(0);

            vec3 v = x_dot+cross(omega, a);

            vec3 test_deltaomega = body_m*body_invI*cross(a, test_deltax_dot);
            float E = (body_m*dot(x_dot+test_deltax_dot, x_dot+test_deltax_dot)
                       +dot(body_I*(omega+test_deltaomega), omega+test_deltaomega)); //TODO: need to adjust I as rotation changes
            if(E < best_E)
            {
                best_deltax_dot = test_deltax_dot;
                // best_deltax = deltax;
                best_r = a;
                best_E = E;
                best_cp = cp;
            }

            if(i == 0)
            {
                vec3 deltax = 0.01*max(-world_voxel.g-1, 0)*normal;
                pseudo_x_dot += deltax;
                pseudo_omega += body_m*body_invI*cross(best_r, deltax);
            }
        }

        best_deltax_dot *= 0.9;

        deltax_dot += best_deltax_dot;
        deltaomega += body_m*body_invI*cross(best_r, best_deltax_dot);

        impulses[best_cp] += deltax_dot;

        // pseudo_x_dot = best_deltax;
        // pseudo_omega = (body_m/body_I)*cross(best_r, best_deltax);
    }

    // if(dot(body_x_dot, body_x_dot+deltax_dot) < 0)
    // {
    //     deltax_dot *= -length(body_x_dot)/dot(body_x_dot, normalize(deltax_dot));
    // }

    // if(dot(body_omega, body_omega+deltaomega) < 0)
    // {
    //     deltaomega *= -length(body_omega)/dot(body_omega, normalize(deltaomega));
    // }
}
