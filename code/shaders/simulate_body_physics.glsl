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

struct body
{
    int materials_origin_x; int materials_origin_y; int materials_origin_z;
    int size_x; int size_y; int size_z;
    float x_cm_x; float x_cm_y; float x_cm_z;
    float x_x; float x_y; float x_z;
    float x_dot_x; float x_dot_y; float x_dot_z;
    float orientation_r; float orientation_x; float orientation_y; float orientation_z;
    float omega_x; float omega_y; float omega_z;

    float m;

    float I_xx; float I_yx; float I_zx;
    float I_xy; float I_yy; float I_zy;
    float I_xz; float I_yz; float I_zz;

    float invI_xx; float invI_yx; float invI_zx;
    float invI_xy; float invI_yy; float invI_zy;
    float invI_xz; float invI_yz; float invI_zz;

    // ivec3 materials_origin;
    // ivec3 size;
    // vec3 x_cm;
    // vec3 x;
    // vec3 x_dot;
    // vec4 orientation;
    // vec3 omega;
};

ivec3 body_materials_origin;
ivec3 body_size;
vec3  body_x_cm;
vec3  body_x;
vec3  body_x_dot;
vec4  body_orientation;
vec3  body_omega;

layout(std430, binding = 0) buffer body_data
{
    body bodies[];
};

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

vec4 axis_to_quaternion(vec3 axis)
{
    float half_angle = length(axis)/2;
    if(half_angle <= 0.0001) return vec4(1,0,0,0);
    vec3 axis_hat = normalize(axis);
    float s = sin(half_angle);
    float c = cos(half_angle);
    return vec4(c, s*axis_hat.x, s*axis_hat.y, s*axis_hat.z);
}

vec4 qmult(vec4 a, vec4 b)
{
    return vec4(a.x*b.x-a.y*b.y-a.z*b.z-a.w*b.w,
                a.x*b.y+a.y*b.x+a.z*b.w-a.w*b.z,
                a.x*b.z+a.z*b.x+a.w*b.y-a.y*b.w,
                a.x*b.w+a.w*b.x+a.y*b.z-a.z*b.y);
}

vec3 apply_rotation(vec4 q, vec3 p)
{
    vec4 p_quat = vec4(0, p.x, p.y, p.z);
    vec4 q_out = qmult(qmult(q, p_quat), vec4(q.x, -q.y, -q.z, -q.w));
    return q_out.yzw;
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

    body_materials_origin = ivec3(bodies[b].materials_origin_x,
                                  bodies[b].materials_origin_y,
                                  bodies[b].materials_origin_z);
    body_size = ivec3(bodies[b].size_x,
                      bodies[b].size_y,
                      bodies[b].size_z);

    body_x_cm = vec3(bodies[b].x_cm_x, bodies[b].x_cm_y, bodies[b].x_cm_z);
    body_x = vec3(bodies[b].x_x, bodies[b].x_y, bodies[b].x_z);
    body_x_dot = vec3(bodies[b].x_dot_x, bodies[b].x_dot_y, bodies[b].x_dot_z);
    body_orientation = vec4(bodies[b].orientation_r, bodies[b].orientation_x, bodies[b].orientation_y, bodies[b].orientation_z);
    body_omega = vec3(bodies[b].omega_x, bodies[b].omega_y, bodies[b].omega_z);

    float m = bodies[b].m;
    mat3 I = mat3(bodies[b].I_xx, bodies[b].I_yx, bodies[b].I_zx,
                  bodies[b].I_xy, bodies[b].I_yy, bodies[b].I_zy,
                  bodies[b].I_xz, bodies[b].I_yz, bodies[b].I_zz);
    mat3 invI = mat3(bodies[b].invI_xx, bodies[b].invI_yx, bodies[b].invI_zx,
                     bodies[b].invI_xy, bodies[b].invI_yy, bodies[b].invI_zy,
                     bodies[b].invI_xz, bodies[b].invI_yz, bodies[b].invI_zz);

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

        // float old_E = m*dot(x_dot, x_dot)+I*dot(omega, omega);

        vec3 best_deltax_dot = vec3(0,0,0);
        vec3 best_deltax = vec3(0,0,0);
        vec3 best_r = vec3(0,0,0);
        int best_cp = -1;
        int n_collisions = 0;
        float best_E = 2*(m*dot(x_dot, x_dot)+dot(I*omega, omega));

        //TODO: build list of points to test that are non-empty and on the surface instead of checking all of them
        //      maybe make it a linked list to minimize texture lookups
        for(int cp = 0; cp < n_collision_points; cp++)
        {
            vec3 world_coord = world_collision_points[cp];
            ivec3 wvc = world_collision_coord[cp]; //world_voxel_coord
            ivec4 world_voxel = voxelFetch(materials, wvc);
            vec3 rel_pos = vec3(wvc)+0.5-world_coord;

            const float COR = 0.2;
            const float COF = 0.1;

            vec3 world_gradient = vec3(
                voxelFetch(materials, wvc+ivec3(1,0,0)).g-voxelFetch(materials, wvc+ivec3(-1,0,0)).g,
                voxelFetch(materials, wvc+ivec3(0,1,0)).g-voxelFetch(materials, wvc+ivec3(0,-1,0)).g,
                voxelFetch(materials, wvc+ivec3(0,0,1)).g-voxelFetch(materials, wvc+ivec3(0,0,-1)).g+0.001
                );

            //TODO: can probably reduce jitter by using more accurate collision_points, maybe add noise
            vec3 normal = normalize(world_gradient);
            // vec3 a = wvc-body_x;
            vec3 a = world_coord-body_x;

            float u = dot(x_dot+cross(omega, a), normal);
            float K = 1.0+m*dot(cross(invI*cross(a, normal), a), normal);
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

            vec3 test_deltaomega = m*invI*cross(a, test_deltax_dot);
            float E = (m*dot(x_dot+test_deltax_dot, x_dot+test_deltax_dot)
                       +dot(I*(omega+test_deltaomega), omega+test_deltaomega)); //TODO: need to adjust I as rotation changes
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
                pseudo_omega += m*invI*cross(best_r, deltax);
            }
        }

        best_deltax_dot *= 0.9;

        deltax_dot += best_deltax_dot;
        deltaomega += m*invI*cross(best_r, best_deltax_dot);

        impulses[best_cp] += deltax_dot;

        // pseudo_x_dot = best_deltax;
        // pseudo_omega = (m/I)*cross(best_r, best_deltax);
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
