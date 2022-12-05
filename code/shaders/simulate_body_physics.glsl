#program simulate_body_physics_program

#shader GL_COMPUTE_SHADER
#include "include/header.glsl"

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform sampler2D material_physical_properties;
layout(location = 2) uniform usampler3D materials;
layout(location = 3, rgba8ui) restrict uniform uimage3D body_materials;

#include "include/body_data.glsl"
#define COLLISION_GRID_BINDING 1
#include "include/collision_grid.glsl"
#define BODY_UPDATE_DATA_BINDING 2
#include "include/body_update_data.glsl"

#define MATERIAL_PHYSICAL_PROPERTIES
#include "include/materials_physical.glsl"
#include "include/maths.glsl"

ivec3 find_unfilled(int b, ivec3 lower, ivec3 upper)
{
    for(int z = lower.z; z < upper.z; z+=1)
        for(int y = lower.y; y < upper.y; y+=1)
            for(int x = lower.x; x < upper.x; x+=1)
            {
                ivec3 body_coord = ivec3(x, y, z);
                ivec3 pos = body_coord+body_texture_lower(b);
                uvec4 body_voxel = imageLoad(body_materials, pos);
                if(floodfill(body_voxel) == 0 && mat(body_voxel) != 0)
                {
                    return body_coord;
                }
            }
    return ivec3(-1,-1,-1);
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

    const int padding = 1;

    ivec3 lower = max(body_lower(b)-padding, 0);
    ivec3 upper = min(body_upper(b)+padding, body_texture_upper(b)-body_texture_lower(b));

    int total_voxels = 0;

    ivec3 new_body_lower = ivec3(1000);
    ivec3 new_body_upper = ivec3(0);

    for(int test_z = lower.z; test_z < upper.z; test_z+=1)
        for(int test_y = lower.y; test_y < upper.y; test_y+=1)
            for(int test_x = lower.x; test_x < upper.x; test_x+=1)
            {
                ivec3 body_coord = ivec3(test_x, test_y, test_z);
                vec3 r = (vec3(body_coord)+0.5);
                ivec3 pos = body_coord+body_texture_lower(b);

                uvec4 body_voxel = imageLoad(body_materials, pos);
                if(mat(body_voxel) != 0)
                {
                    total_voxels++;

                    float rho = density(mat(body_voxel));
                    m += rho;
                    x_cm += rho*r;

                    float diag = (1.0/6.0)+dot(r, r);
                    I_xx += rho*(diag-r.x*r.x);
                    I_yy += rho*(diag-r.y*r.y);
                    I_zz += rho*(diag-r.z*r.z);
                    I_xy += -rho*(r.x*r.y);
                    I_xz += -rho*(r.x*r.z);
                    I_yz += -rho*(r.y*r.z);

                    new_body_lower = min(new_body_lower, body_coord);
                    new_body_upper = max(new_body_upper, body_coord+1);
                }
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

    #define max_stack_size 1024
    ivec3 search_stack[max_stack_size];
    int stack_size = 0;

    int floodfilled_voxels = 0;

    int current_fragment_id = 0;

    int n_new_fragments = 0;

    #define N_MAX_FRAGMENTS 16
    for(int i = 0; i < N_MAX_FRAGMENTS; i++)
    {
        current_fragment_id = n_new_fragments+1;

        ivec3 new_seed = find_unfilled(b, lower, upper);
        if(new_seed.x < 0) break;
        search_stack[stack_size++] = new_seed;

        ivec3 fragment_lower = ivec3(1000);
        ivec3 fragment_upper = ivec3(0);

        while(stack_size > 0)
        {
            ivec3 body_coord = search_stack[--stack_size];
            ivec3 pos = body_coord+body_texture_lower(b);
            uvec4 body_voxel = imageLoad(body_materials, pos);
            body_voxel.g = current_fragment_id;
            imageStore(body_materials, pos, body_voxel);

            fragment_lower = min(fragment_lower, body_coord);
            fragment_upper = max(fragment_upper, body_coord+1);

            floodfilled_voxels++;

            ivec3 r_coord = body_coord+ivec3(+1,0,0);
            ivec3 l_coord = body_coord+ivec3(-1,0,0);
            ivec3 f_coord = body_coord+ivec3(0,+1,0);
            ivec3 b_coord = body_coord+ivec3(0,-1,0);
            ivec3 u_coord = body_coord+ivec3(0,0,+1);
            ivec3 d_coord = body_coord+ivec3(0,0,-1);

            ivec3 r_pos = pos+ivec3(+1,0,0);
            ivec3 l_pos = pos+ivec3(-1,0,0);
            ivec3 f_pos = pos+ivec3(0,+1,0);
            ivec3 b_pos = pos+ivec3(0,-1,0);
            ivec3 u_pos = pos+ivec3(0,0,+1);
            ivec3 d_pos = pos+ivec3(0,0,-1);

            uvec4 r_vox = imageLoad(body_materials, r_pos);
            uvec4 l_vox = imageLoad(body_materials, l_pos);
            uvec4 f_vox = imageLoad(body_materials, f_pos);
            uvec4 b_vox = imageLoad(body_materials, b_pos);
            uvec4 u_vox = imageLoad(body_materials, u_pos);
            uvec4 d_vox = imageLoad(body_materials, d_pos);

            if(floodfill(r_vox) == 0 && mat(r_vox) != 0 && stack_size < max_stack_size) search_stack[stack_size++] = r_coord;
            if(floodfill(l_vox) == 0 && mat(l_vox) != 0 && stack_size < max_stack_size) search_stack[stack_size++] = l_coord;
            if(floodfill(f_vox) == 0 && mat(f_vox) != 0 && stack_size < max_stack_size) search_stack[stack_size++] = f_coord;
            if(floodfill(b_vox) == 0 && mat(b_vox) != 0 && stack_size < max_stack_size) search_stack[stack_size++] = b_coord;
            if(floodfill(u_vox) == 0 && mat(u_vox) != 0 && stack_size < max_stack_size) search_stack[stack_size++] = u_coord;
            if(floodfill(d_vox) == 0 && mat(d_vox) != 0 && stack_size < max_stack_size) search_stack[stack_size++] = d_coord;
        }

        if(fragment_lower.x != 1000)
        {
            n_new_fragments++;
        }
    }

    body_updates[b].n_fragments = n_new_fragments;

    // if(n_new_fragments > 0)
    // {
    //     int fragment_index = atomicAdd(n_fragments, n_new_fragments);

    //     body_updates[b].fragment_index = fragment_index;

    //     for(int f = 0; f < n_new_fragments; f++)
    //     {
    //         fragments[fragment_index+f] = new_fragments[f];
    //     }
    // }
}
