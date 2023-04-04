#program simulate_body_program

/////////////////////////////////////////////////////////////////

#shader GL_COMPUTE_SHADER
#include "include/header.glsl"
#include "include/maths.glsl"

#define localgroup_size 4
#define subgroup_size 1
layout(local_size_x = localgroup_size, local_size_y = localgroup_size, local_size_z = localgroup_size) in;

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform sampler2D material_physical_properties;
layout(location = 2) uniform usampler3D materials;
layout(location = 3) uniform usampler3D body_materials;
layout(location = 4, rgba8ui) uniform uimage3D body_materials_out;
layout(location = 5) uniform usampler3D form_materials;
layout(location = 6) uniform int n_bodies;
layout(location = 7) uniform int n_beams;
layout(location = 8) uniform int n_body_chunks;

#define MATERIAL_PHYSICAL_PROPERTIES
#include "include/materials_physical.glsl"

#include "include/body_data.glsl"
#define BODY_UPDATE_DATA_BINDING 1
#include "include/body_update_data.glsl"
#define PARTICLE_DATA_BINDING 2
#include "include/particle_data.glsl"
#define BEAM_DATA_BINDING 3
#include "include/beam_data.glsl"
#define CONTACT_DATA_BINDING 4
#include "include/contact_data.glsl"
#define COLLISION_GRID_BINDING 5
#include "include/collision_grid.glsl"
#define BODY_CHUNKS_BINDING 6

struct body_chunk
{
    uint body_id;
    uint origin_x;
    uint origin_y;
    uint origin_z;
};

layout(std430, binding = BODY_CHUNKS_BINDING) buffer body_chunk_data
{
    body_chunk body_chunks[];
};

uint rand(uint seed)
{
    seed ^= seed<<13;
    seed ^= seed>>17;
    seed ^= seed<<5;
    return seed;
}

void add_contact(int bi0, int bi1, uvec4 voxel0, uvec4 voxel1, vec3 body0_coord, vec3 body1_coord, vec3 normal, uint phase)
{
    int collision_index = atomicAdd(n_contacts, 1);

    contact_point new_contact;

    new_contact.b0 = bi0;
    new_contact.b1 = bi1;

    new_contact.material0 = mat(voxel0);
    new_contact.material1 = mat(voxel1);

    new_contact.phase = phase;

    new_contact.p0_x = body0_coord.x;
    new_contact.p0_y = body0_coord.y;
    new_contact.p0_z = body0_coord.z;

    new_contact.p1_x = body1_coord.x;
    new_contact.p1_y = body1_coord.y;
    new_contact.p1_z = body1_coord.z;

    new_contact.n_x = normal.x;
    new_contact.n_y = normal.y;
    new_contact.n_z = normal.z;
    new_contact.depth0 = signed_depth(voxel0);
    new_contact.depth1 = signed_depth(voxel1);

    new_contact.f_x = 0;
    new_contact.f_y = 0;
    new_contact.f_z = 0;

    contacts[collision_index++] = new_contact;
}

void main()
{
    uint body_chunk_id = gl_WorkGroupID.x;
    int bi = int(body_chunks[body_chunk_id].body_id);
    uvec3 origin = uvec3(body_chunks[body_chunk_id].origin_x, body_chunks[body_chunk_id].origin_y, body_chunks[body_chunk_id].origin_z);

    ivec3 pos = ivec3(origin+gl_LocalInvocationID);
    // if(any(greaterThan(pos, body_texture_lower(bi)+body_upper(bi)))) return;
    if(any(greaterThan(pos, body_texture_upper(bi)))) return;

    //check 2x2x2 to determine if this corner is on the boundary
    bool inside = false;
    bool outside = false;
    uvec4 corner_voxel = uvec4(0);
    //TODO: better way of deciding the corner voxel when there are multiple
    for(int z = 0; z < 2; z++)
        for(int y = 0; y < 2; y++)
            for(int x = 0; x < 2; x++)
            {
                ivec3 p = ivec3(pos.x-x, pos.y-y, pos.z-z);
                uvec4 vox;
                if(any(lessThan(p, body_texture_lower(bi))) || any(greaterThanEqual(p, body_texture_upper(bi))))
                    vox = uvec4(0);
                else
                    vox = texelFetch(body_materials, p, 0);
                if(mat(vox) != 0)
                {
                    inside = true;
                    corner_voxel = vox;
                }
                else
                {
                    outside = true;
                }
            }

    if(inside && outside)
    {
        vec3 body_coord = pos-body_texture_lower(bi)+body_origin_to_lower(bi);
        vec3 r = body_coord-body_x_cm(bi);

        vec3 world_coord = apply_rotation(body_orientation(bi), r)+body_x(bi);

        ivec3 wvc = ivec3(world_coord); //world_voxel_coord
        uvec4 world_voxel = texelFetch(materials, wvc, 0);

        // if(mat(world_voxel) != 0 && (phase(world_voxel) == phase_solid || phase(world_voxel) == phase_sand))
        if(mat(world_voxel) != 0)
        {
            vec3 normal = unnormalized_gradient(materials, wvc);
            normal = normalize(normal);
            vec3 nsign = sign(normal);
            vec3 nabs = normal*nsign;

            vec3 main_dir = step(nabs.yzx, nabs.xyz)*step(nabs.zyx, nabs.xyz);
            vec3 adjusted_world_coord = mix(world_coord, nsign*ceil(nsign*world_coord), main_dir);

            add_contact(bi, -1, corner_voxel, world_voxel, body_coord, adjusted_world_coord, normal, phase(world_voxel));
        }

        ivec3 cell_coord = ivec3(world_coord/collision_cell_size);
        cell_coord = clamp(cell_coord, 0, collision_cells_per_axis);

        collision_cell cell = collision_grid[cell_coord.x+collision_cells_per_axis*(cell_coord.y+collision_cells_per_axis*cell_coord.z)];

        for(int ci = cell.first_index; ci < cell.first_index+cell.n_bodies; ci++)
        {
            int bi1 = collision_indices[ci];
        // for(int bi1 = 0; bi1 < n_bodies; bi1++)
        // {
            if(bi1 == bi || body_is_substantial(bi) == 0 || (body_brain_id(bi) != 0 && (body_brain_id(bi1) == body_brain_id(bi)))) continue;
            //working in the frame of body bi1
            vec4 rel_orientation = qmult(conjugate(body_orientation(bi1)), body_orientation(bi));
            vec3 rel_x = apply_rotation(conjugate(body_orientation(bi1)), body_x(bi)-body_x(bi1));
            vec3 body1_coord = apply_rotation(rel_orientation, r)+rel_x+body_x_cm(bi1);

            ivec3 body1_pos = ivec3(body1_coord);
            if(any(lessThan(body1_pos, vec3(0))) || any(greaterThanEqual(body1_pos, body_texture_upper(bi1)-body_texture_lower(bi1)))) continue;
            ivec3 pos1 = body1_pos+body_texture_lower(bi1);
            uvec4 body1_voxel = texelFetch(body_materials, pos1, 0);

            if(mat(body1_voxel) != 0)
            {
                vec3 normal = unnormalized_gradient(body_materials, pos1);
                normal = normalize(normal);
                vec3 nsign = sign(normal);
                vec3 nabs = normal*nsign;
                normal = apply_rotation(body_orientation(bi1), normal);

                vec3 main_dir = step(nabs.yzx, nabs.xyz)*step(nabs.zyx, nabs.xyz);
                vec3 adjusted_body1_coord = mix(body1_coord, nsign*ceil(nsign*body1_coord), main_dir);

                add_contact(bi, bi1, corner_voxel, body1_voxel, body_coord, adjusted_body1_coord, normal, phase_solid);
                break;
            }
        }
    }
}
