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

layout(location = 0) out vec3 p1;
layout(location = 1) out vec3 p2;
layout(location = 2) out vec3 p3;
layout(location = 3) out vec3 n1;
layout(location = 4) out vec3 n2;
layout(location = 5) out vec3 n3;
layout(location = 6) out uvec3 contact_materials;

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform usampler3D materials;
layout(location = 2) uniform usampler3D body_materials;
layout(location = 3) uniform sampler3D body_forces;
layout(location = 4) uniform sampler3D body_shifts;

#include "include/body_data.glsl"
#include "include/materials_physical.glsl"
#include "include/maths.glsl"

smooth in vec2 uv;

const int chunk_size = 256;

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

                uvec4 body_voxel = texelFetch(body_materials, body_materials_origin+body_coord, 0);

                // if(body_voxel.g == 0 && body_voxel.r > 0)
                if(depth(body_voxel) == SURF_DEPTH)
                {
                    ivec3 wvc = ivec3(world_coord); //world_voxel_coord
                    uvec4 world_voxel = texelFetch(materials, wvc, 0);
                    vec3 rel_pos = vec3(wvc)+0.5-world_coord;
                    // if(world_voxel.r > 0 && dot(rel_pos, rel_pos) <= 1.0)
                    if(depth(world_voxel) >= SURF_DEPTH-1)
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

                test_x += int(max(abs(depth(body_voxel)-SURF_DEPTH)-1, 0));
            }
}

//TODO: have better method of choosing points
void find_surface_points(out vec3 world_collision_points[N_MAX_COLLISION_POINTS], out ivec3 world_collision_coord[N_MAX_COLLISION_POINTS], out int n_collision_points)
{
    n_collision_points = 0;

    //TODO: maybe use space filling curves
    for(int test_z = 0; test_z < body_size.z; test_z+=1)
        for(int test_y = 0; test_y < body_size.y; test_y+=1)
            for(int test_x = 0; test_x < body_size.x; test_x+=1)
            {

                ivec3 body_coord = ivec3(test_x, test_y, test_z);
                vec3 world_coord = apply_rotation(body_orientation, vec3(body_coord)+0.5-body_x_cm)+body_x;

                uvec4 body_voxel = texelFetch(body_materials, body_materials_origin+body_coord, 0);

                // if(body_voxel.g == 0 && body_voxel.r > 0)
                if(depth(body_voxel) == SURF_DEPTH)
                {
                    ivec3 wvc = ivec3(world_coord); //world_voxel_coord

                    world_collision_points[n_collision_points] = world_coord;
                    world_collision_coord[n_collision_points] = wvc;
                    n_collision_points++;
                    if(n_collision_points >= N_MAX_COLLISION_POINTS) return;
                }

                test_x += int(max(abs(depth(body_voxel)-SURF_DEPTH)-1, 0));
            }
}

void main()
{
    int b = int(gl_FragCoord.y);
    // int b = 0;

    get_body_data(b);

    p1 = vec3(0);
    p2 = vec3(0);
    p3 = vec3(0);
    n1 = vec3(0,0,1);
    n2 = vec3(0,0,1);
    n3 = vec3(0,0,1);
    contact_materials = uvec3(0);

    vec3 world_collision_points[N_MAX_COLLISION_POINTS];
    ivec3 world_collision_coord[N_MAX_COLLISION_POINTS];
    int n_collision_points = 0;

    find_collision_points(world_collision_points, world_collision_coord, n_collision_points);

    if(n_collision_points == 0)
        find_surface_points(world_collision_points, world_collision_coord, n_collision_points);

    vec3 impulses[N_MAX_COLLISION_POINTS] = vec3[N_MAX_COLLISION_POINTS](0);

    float d1 = 16;
    float d2 = 16;
    float d3 = 16;

    uint best_depth = MAX_DEPTH;
    float best_rsq = 16;
    for(int cp = 0; cp < n_collision_points; cp++)
    {
        vec3 world_coord = world_collision_points[cp];
        ivec3 wvc = world_collision_coord[cp]; //world_voxel_coord
        uvec4 world_voxel = texelFetch(materials, wvc, 0);
        vec3 r = world_coord-body_x;
        float rsq = dot(r, r);
        if(depth(world_voxel) < best_depth || (depth(world_voxel) == best_depth && rsq < best_rsq))
        {
            best_depth = depth(world_voxel);
            best_rsq = rsq;
            p1 = world_coord;
            d1 = depth(world_voxel);
            contact_materials.r = world_voxel.r;
        }
    }

    float best_asq = 0;
    for(int cp = 0; cp < n_collision_points; cp++)
    {
        vec3 world_coord = world_collision_points[cp];
        ivec3 wvc = world_collision_coord[cp]; //world_voxel_coord
        uvec4 world_voxel = texelFetch(materials, wvc, 0);
        float asq = dot(world_coord-p1, world_coord-p1);
        if(asq > best_asq)
        {
            best_asq = asq;
            p2 = world_coord;
            d2 = depth(world_voxel);
            contact_materials.g = world_voxel.r;
        }
    }

    float best_bsq = -1;
    for(int cp = 0; cp < n_collision_points; cp++)
    {
        vec3 world_coord = world_collision_points[cp];
        ivec3 wvc = world_collision_coord[cp]; //world_voxel_coord
        uvec4 world_voxel = texelFetch(materials, wvc, 0);
        vec3 ab = normalize(p2-p1);
        vec3 ac = world_coord-p1;
        float bsq = dot(ac, ac) - sq(dot(ac, ab));
        if(bsq > best_bsq)
        {
            best_bsq = bsq;
            p3 = world_coord;
            d3 = depth(world_voxel);
            contact_materials.b = world_voxel.r;
        }
    }

    d1 += 1;
    d2 += 1;
    d3 += 1;

    ivec3 ip1 = ivec3(p1);
    n1 = vec3(
        float(depth(texelFetch(materials, ip1+ivec3(-1,0,0), 0)))-float(depth(texelFetch(materials, ip1+ivec3(+1,0,0), 0))),
        float(depth(texelFetch(materials, ip1+ivec3(0,-1,0), 0)))-float(depth(texelFetch(materials, ip1+ivec3(0,+1,0), 0))),
        float(depth(texelFetch(materials, ip1+ivec3(0,0,-1), 0)))-float(depth(texelFetch(materials, ip1+ivec3(0,0,+1), 0)))+0.001);
    n1 = d1*normalize(n1);

    ivec3 ip2 = ivec3(p2);
    n2 = vec3(
        float(depth(texelFetch(materials, ip2+ivec3(-1,0,0), 0)))-float(depth(texelFetch(materials, ip2+ivec3(+1,0,0), 0))),
        float(depth(texelFetch(materials, ip2+ivec3(0,-1,0), 0)))-float(depth(texelFetch(materials, ip2+ivec3(0,+1,0), 0))),
        float(depth(texelFetch(materials, ip2+ivec3(0,0,-1), 0)))-float(depth(texelFetch(materials, ip2+ivec3(0,0,+1), 0)))+0.001);
    n2 = d2*normalize(n1);

    ivec3 ip3 = ivec3(p3);
    n3 = vec3(
        float(depth(texelFetch(materials, ip3+ivec3(-1,0,0), 0)))-float(depth(texelFetch(materials, ip3+ivec3(+1,0,0), 0))),
        float(depth(texelFetch(materials, ip3+ivec3(0,-1,0), 0)))-float(depth(texelFetch(materials, ip3+ivec3(0,+1,0), 0))),
        float(depth(texelFetch(materials, ip3+ivec3(0,0,-1), 0)))-float(depth(texelFetch(materials, ip3+ivec3(0,0,+1), 0)))+0.001);
    n3 = d3*normalize(n1);
}
