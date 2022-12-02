#program simulate_body_physics_program

#shader GL_COMPUTE_SHADER
#include "include/header.glsl"

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform sampler2D material_physical_properties;
layout(location = 2) uniform usampler3D materials;
layout(location = 3) uniform usampler3D body_materials;

#include "include/body_data.glsl"
#define COLLISION_GRID_BINDING 1
#include "include/collision_grid.glsl"
#define BODY_UPDATE_DATA_BINDING 2
#include "include/body_update_data.glsl"
#define CONTACT_DATA_BINDING 3
#include "include/contact_data.glsl"

#define MATERIAL_PHYSICAL_PROPERTIES
#include "include/materials_physical.glsl"
#include "include/maths.glsl"

const int chunk_size = 256;

//TODO: use old collision points and try to keep collision points near old ones for stability

#define N_MAX_COLLISION_POINTS 256
void find_collision_points(int bi, out float collision_time[N_MAX_COLLISION_POINTS], out ivec3 body_collision_coord[N_MAX_COLLISION_POINTS], out ivec3 world_collision_coord[N_MAX_COLLISION_POINTS], out int n_collision_points, out int min_depth)
{
    n_collision_points = 0;
    min_depth = 0;

    //TODO: maybe use space filling curves
    for(int test_z = body_lower(bi).z; test_z <= body_upper(bi).z; test_z+=1)
        for(int test_y = body_lower(bi).y; test_y <= body_upper(bi).y; test_y+=1)
            for(int test_x = body_lower(bi).x; test_x <= body_upper(bi).x; test_x+=1)
            {
                ivec3 body_coord = ivec3(test_x, test_y, test_z);
                uvec4 body_voxel = texelFetch(body_materials, body_texture_lower(bi)+body_coord, 0);

                // if(body_voxel.g == 0 && body_voxel.r > 0)
                if(corner(body_voxel) == 1)
                {
                    // int n_subdivisions = max(int(length(body_x(bi)-body_old_x(bi))*5.0), 1);
                    int n_subdivisions = 1;
                    float scale = 1.0/max(float(n_subdivisions-1), 1.0);
                    vec3 r = vec3(body_coord)-body_x_cm(bi);

                    for(int i = n_subdivisions-1; i >= 0; i--)
                    {
                        float t = 1-i*scale;
                        vec4 orientation = mix(body_old_orientation(bi), body_orientation(bi), t); //who needs slerp
                        orientation = normalize(orientation);
                        vec3 x = mix(body_old_x(bi), body_x(bi), t);
                        vec3 world_coord = apply_rotation(orientation, r)+x;

                        ivec3 wvc = ivec3(world_coord); //world_voxel_coord
                        uvec4 world_voxel = texelFetch(materials, wvc, 0);
                        vec3 rel_pos = vec3(wvc)+0.5-world_coord;
                        // if(world_voxel.r > 0 && dot(rel_pos, rel_pos) <= 1.0)
                        int depth = signed_depth(world_voxel);
                        if(depth <= 0)
                        {
                            min_depth = min(min_depth, depth);
                            collision_time[n_collision_points] = t;
                            body_collision_coord[n_collision_points] = body_coord;
                            world_collision_coord[n_collision_points] = wvc;
                            n_collision_points++;
                            if(n_collision_points >= N_MAX_COLLISION_POINTS) return;
                            break;
                        }
                    }
                }

                // test_x += max(abs(signed_depth(body_voxel))-1, 0);
            }
}

// void find_collision_points(int ai, int bi)
// {
//     n_collision_points = 0;

//     vec3 al = body_boxl(ai);
//     vec3 au = body_boxu(ai);
//     vec3 bl = body_boxl(bi);
//     vec3 bu = body_boxu(bi);

//     vec4 aorientation = body_orientation(ai);
//     vec4 borientation = body_orientation(bi);

//     vec4 rel_orientation = qmult(conjugate(aorientation), borientation);

//     vec3 a_to_borigin = apply_rotation(conjugate(aorientation), body_x(bi)-body_x(ai)+apply_rotation(borientation, -body_x_cm(bi)));

//     //TODO: maybe use space filling curves
//     for(int test_z = body_lower(bi).z; test_z < body_upper(bi).z; test_z+=1)
//         for(int test_y = body_lower(bi).y; test_y < body_upper(bi).y; test_y+=1)
//             for(int test_x = body_lower(bi).x; test_x < body_upper(bi).x; test_x+=1)
//             {
//                 vec3 b_coord = vec3(test_x, test_y, test_z);
//                 vec3 a_coord = a_to_borigin+apply_rotation(rel_orientation, b_coord);

//                 ivec3 ib_coord = ivec3(test_x, text_y, test_z);
//                 ivec3 ia_coord = ivec3(a_coord);

//                 if(any(lessThan(ia_coord, ivec3(0)) || any greaterThanEqual(ia_coord, body_upper(ai)))) continue;

//                 uvec4 a_voxel = texelFetch(body_materials, body_texture_lower(ai)+a_coord, 0);
//                 uvec4 b_voxel = texelFetch(body_materials, body_texture_lower(bi)+b_coord, 0);

//                 if(signed_depth(a_voxel) == 0 && signed_depth(b_bvoxel))
//                 {
//                     ivec3 wvc = ivec3(world_coord); //world_voxel_coord
//                     uvec4 world_voxel = texelFetch(materials, wvc, 0);
//                     vec3 rel_pos = vec3(wvc)+0.5-world_coord;
//                     // if(world_voxel.r > 0 && dot(rel_pos, rel_pos) <= 1.0)
//                     if(signed_depth(world_voxel) <= 0)
//                     {
//                         world_collision_points[n_collision_points] = world_coord;
//                         world_collision_coord[n_collision_points] = wvc;
//                         n_collision_points++;
//                         if(n_collision_points >= N_MAX_COLLISION_POINTS) return;
//                     }

//                     // for(int wz = 0; wz <= 1; wz++)
//                     //     for(int wy = 0; wy <= 1; wy++)
//                     //         for(int wx = 0; wx <= 1; wx++)
//                     //         {
//                     //             ivec3 wvc = ivec3(world_coord-0.5)+ivec3(wx,wy,wz); //world_voxel_coord
//                     //             ivec4 world_voxel = voxelFetch(materials, wvc);
//                     //             vec3 rel_pos = vec3(wvc)+0.5-world_coord;
//                     //             // if(world_voxel.r > 0 && dot(rel_pos, rel_pos) <= 1.0)
//                     //             if(world_voxel.g <= 2)
//                     //             {
//                     //                 world_collision_points[n_collision_points] = world_coord;
//                     //                 world_collision_coord[n_collision_points] = wvc;
//                     //                 n_collision_points++;
//                     //                 if(n_collision_points >= N_MAX_COLLISION_POINTS) return;
//                     //             }
//                     //         }
//                 }

//                 test_x += max(abs(signed_depth(b_voxel))-1, 0);
//             }
// }

void main()
{
    int b = int(gl_GlobalInvocationID.x);

    float m = 0;
    float I_xx = 0;
    float I_xy = 0; float I_yy = 0;
    float I_xz = 0; float I_yz = 0; float I_zz = 0;
    vec3 x_cm = vec3(0);

    float collision_time[N_MAX_COLLISION_POINTS];
    ivec3 body_collision_coord[N_MAX_COLLISION_POINTS];
    ivec3 world_collision_coord[N_MAX_COLLISION_POINTS];
    int n_collision_points = 0;
    int min_depth = 0;

    find_collision_points(b, collision_time, body_collision_coord, world_collision_coord, n_collision_points, min_depth);

    const int padding = 1;

    ivec3 lower = max(body_lower(b)-padding, 0);
    ivec3 upper = min(body_upper(b)+padding, body_texture_upper(b)-body_texture_lower(b));

    ivec3 new_body_lower = ivec3(1000);
    ivec3 new_body_upper = ivec3(0);

    for(int test_z = lower.z; test_z < upper.z; test_z+=1)
        for(int test_y = lower.y; test_y < upper.y; test_y+=1)
            for(int test_x = lower.x; test_x < upper.x; test_x+=1)
            {
                ivec3 body_coord = ivec3(test_x, test_y, test_z);
                vec3 r = (vec3(body_coord)+0.5); //TODO: subtract body_x_cm(b)?
                ivec3 pos = body_coord+body_texture_lower(b);

                uvec4 body_voxel = texelFetch(body_materials, pos, 0);
                if(mat(body_voxel) != 0)
                {
                    float rho = density(mat(body_voxel));
                    m += rho;
                    x_cm += rho*r;

                    float diag = 0.4*sq(0.5)+dot(r, r);
                    I_xx += rho*(diag-r.x*r.x);
                    I_yy += rho*(diag-r.y*r.y);
                    I_zz += rho*(diag-r.z*r.z);
                    I_xy += -rho*(r.x*r.y);
                    I_xz += -rho*(r.x*r.z);
                    I_yz += -rho*(r.y*r.z);

                    new_body_lower = min(new_body_lower, body_coord);
                    new_body_upper = max(new_body_upper, body_coord+1);
                }
                //TODO: I can skip some voxels to make this faster for bodies with large empty spaces
            }
    x_cm /= m;
    new_body_lower = min(new_body_lower, new_body_upper);

    body_updates[b].m    =    m;
    body_updates[b].I_xx = I_xx;
    body_updates[b].I_yy = I_yy;
    body_updates[b].I_zz = I_zz;
    body_updates[b].I_xy = I_xy;
    body_updates[b].I_xz = I_xz;
    body_updates[b].I_yz = I_yz;
    body_updates[b].x_cm_x = x_cm.x;
    body_updates[b].x_cm_y = x_cm.y;
    body_updates[b].x_cm_z = x_cm.z;

    body_updates[b].lower_x = new_body_lower.x;
    body_updates[b].lower_y = new_body_lower.y;
    body_updates[b].lower_z = new_body_lower.z;
    body_updates[b].upper_x = new_body_upper.x;
    body_updates[b].upper_y = new_body_upper.y;
    body_updates[b].upper_z = new_body_upper.z;

    for(int cp = n_collision_points-1; cp >= 0; cp--)
    {
        ivec3 wvc = world_collision_coord[cp];
        uvec4 world_voxel = texelFetch(materials, wvc, 0);
        if(signed_depth(world_voxel) > min_depth)
        {
          --n_collision_points;
          collision_time[cp] = collision_time[n_collision_points];
          world_collision_coord[cp] = world_collision_coord[n_collision_points];
          body_collision_coord[cp] = body_collision_coord[n_collision_points];
        }
    }

    int collision_index = atomicAdd(n_contacts, n_collision_points);

    for(int cp = 0; cp < n_collision_points; cp++)
    {
        float t = collision_time[cp];
        ivec3 wvc = world_collision_coord[cp];
        ivec3 bvc = body_collision_coord[cp];
        vec4 orientation = mix(body_old_orientation(b), body_orientation(b), t); //who needs slerp
        orientation = normalize(orientation);
        vec3 r = vec3(bvc)-body_x_cm(b);
        vec3 bx = mix(body_old_x(b), body_x(b), t);
        vec3 bvx = apply_rotation(orientation, r)+bx;
        vec3 wvx = vec3(wvc)+0.5;
        vec3 body_coord = vec3(bvc);
        uvec4 world_voxel = texelFetch(materials, wvc, 0);

        vec3 normal = unnormalized_gradient(materials, wvc);
        normal = normalize(normal);
        vec3 nsign = sign(normal);
        vec3 nabs = normal*nsign;

        vec3 main_dir = step(nabs.yzx, nabs.xyz)*step(nabs.zyx, nabs.xyz);
        vec3 world_coord = mix(bvx, nsign*ceil(nsign*bvx), main_dir);

        contact_point new_contact;
        new_contact.b0 = b;
        new_contact.b1 = -1;
        new_contact.material1 = world_voxel.r;
        new_contact.x_x = world_coord.x;
        new_contact.x_y = world_coord.y;
        new_contact.x_z = world_coord.z;

        new_contact.p0_x = body_coord.x;
        new_contact.p0_y = body_coord.y;
        new_contact.p0_z = body_coord.z;

        // vec3 x = floor(world_coord)+0.5;
        vec3 x = world_coord;

        new_contact.x_x = x.x;
        new_contact.x_y = x.y;
        new_contact.x_z = x.z;

        new_contact.n_x = normal.x;
        new_contact.n_y = normal.y;
        new_contact.n_z = normal.z;
        new_contact.depth1 = signed_depth(world_voxel);

        new_contact.f_x = 0;
        new_contact.f_y = 0;
        new_contact.f_z = 0;

        contacts[collision_index++] = new_contact;
    }
}
