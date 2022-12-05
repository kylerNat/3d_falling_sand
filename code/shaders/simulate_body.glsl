#program simulate_body_program

/////////////////////////////////////////////////////////////////

#shader GL_COMPUTE_SHADER
#include "include/header.glsl"
#include "include/maths.glsl"

#define localgroup_size 8
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
#define BODY_CHUNKS_BINDING 5

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

void add_world_contact(int bi, uvec4 world_voxel, vec3 world_coord, vec3 body_coord, vec3 normal, float world_depth)
{
    int collision_index = atomicAdd(n_contacts, 1);

    contact_point new_contact;
    new_contact.b0 = bi;
    new_contact.b1 = -1;
    new_contact.material1 = world_voxel.r;
    new_contact.x_x = world_coord.x;
    new_contact.x_y = world_coord.y;
    new_contact.x_z = world_coord.z;

    new_contact.p0_x = body_coord.x;
    new_contact.p0_y = body_coord.y;
    new_contact.p0_z = body_coord.z;

    new_contact.n_x = normal.x;
    new_contact.n_y = normal.y;
    new_contact.n_z = normal.z;
    new_contact.depth1 = world_depth;

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

    int pad = 1;
    ivec3 pos = ivec3(origin+gl_LocalInvocationID);
    // if(any(greaterThan(pos, body_texture_lower(bi)+body_upper(bi)))) return;
    if(any(greaterThan(pos, body_texture_upper(bi)))) return;

    // if(bi == 0 && layer == 0 && gl_VertexID == 0)
    // {
    //     int dead_index = atomicAdd(n_dead_particles, -1)-1;
    //     //this assumes particle creation and destruction never happen simutaneously
    //     uint p = dead_particles[dead_index];
    //     particles[p].voxel_data = 3|(2<<(6+8))|(8<<(2+16))|(15<<24);
    //     particles[p].x = bodies[bi].x_x;
    //     particles[p].y = bodies[bi].x_y;
    //     particles[p].z = bodies[bi].x_z;
    //     particles[p].x_dot = bodies[bi].x_dot_x;
    //     particles[p].y_dot = bodies[bi].x_dot_y;
    //     particles[p].z_dot = bodies[bi].x_dot_z+5;
    //     particles[p].alive = true;
    // }

    //+,0,-,0
    //0,+,0,-
    // int rot = (frame_number+layer)%4;
    uint rot = rand(rand(rand(frame_number)))%4;
    int i = 0;
    ivec2 dir = ivec2(((rot&1)*(2-rot)), (1-(rot&1))*(1-rot));

    uvec4 c  = texelFetch(body_materials, ivec3(pos.x, pos.y, pos.z), 0);
    uvec4 u  = texelFetch(body_materials, ivec3(pos.x, pos.y, pos.z+1), 0);
    uvec4 d  = texelFetch(body_materials, ivec3(pos.x, pos.y, pos.z-1), 0);
    uvec4 r  = texelFetch(body_materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z), 0);
    uvec4 l  = texelFetch(body_materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z), 0);
    uvec4 f  = texelFetch(body_materials, ivec3(pos.x-dir.y, pos.y+dir.x, pos.z), 0);
    uvec4 b  = texelFetch(body_materials, ivec3(pos.x+dir.y, pos.y-dir.x, pos.z), 0);

    if(body_fragment_id(bi) > 0)
    {
        if(floodfill(c) != body_fragment_id(bi)) c = uvec4(0);
        if(floodfill(u) != body_fragment_id(bi)) u = uvec4(0);
        if(floodfill(d) != body_fragment_id(bi)) d = uvec4(0);
        if(floodfill(r) != body_fragment_id(bi)) r = uvec4(0);
        if(floodfill(l) != body_fragment_id(bi)) l = uvec4(0);
        if(floodfill(f) != body_fragment_id(bi)) f = uvec4(0);
        if(floodfill(b) != body_fragment_id(bi)) b = uvec4(0);
    }

    ivec3 form_pos = pos+body_form_origin(bi);
    uint form_voxel = 0;
    if(all(greaterThanEqual(form_pos, body_form_lower(bi))) && all(lessThan(form_pos, body_form_upper(bi))))
        form_voxel = texelFetch(form_materials, form_pos, 0).r;

    int spawned_cell = 0;

    uint temp = temp(c);
    uint volt = 0;
    uint trig = 0;

    uint mid = mat(c); //material id
    bool is_cell = mid >= BASE_CELL_MAT;
    if(is_cell) mid += body_cell_material_id(bi);

    for(int i = 0; i < n_triggers(mid); i++)
    {
        uint trigger_data = trigger_info(mid, i);
        uint condition_type = trigger_data&0xFF;
        uint action_type = (trigger_data>>8)&0xFF;
        uint trigger_material = trigger_data>>16;
        bool condition = false;
        switch(condition_type)
        {
            case trig_always:
                condition = true;
                break;
            case trig_hot:
                condition = temp > 12;
                break;
            case trig_cold:
                condition = temp <= 4;
                break;
            case trig_electric:
                condition = volt > 0;
                break;
            case trig_contact:
                condition = trig(c) == i;
                break;
            default:
                condition = false;
        }

        if(condition)
        {
            switch(action_type)
            {
                case act_grow: {
                    trig = i+1;
                    break;
                }
                case act_die: {
                    c = uvec4(0);
                    break;
                }
                case act_heat: {
                    temp++;
                    break;
                }
                case act_chill: {
                    temp--;
                    break;
                }
                case act_electrify: {
                    volt = 3;
                    break;
                }
                case act_explode: {
                    //TODO: EXPLOSIONS!
                    break;
                }
                case act_spray: {
                    //create particle of type child_material_id, with velocity in the normal direction
                    break;
                }
                default:
            }
            break;
        }
    }

    //check if this cell is empty, on the surface, and is filled in the body map
    //then check for neighbors that are trying to grow
    if(body_is_mutating(bi)==1 && mid != form_voxel)
    {
        c = uvec4(0);
    }

    vec3 voxel_pos = pos-body_texture_lower(bi)-body_x_cm(bi)+0.5;

    //TODO: seed this in a way that is independent of internal engine variables, use displacement from nucleus or something
    // int grow_phase = int(length(voxel_pos));
    uint grow_phase = rand(rand(rand(pos.z)+pos.y)+pos.x);

    if(mid != form_voxel && form_voxel != 0 && depth(c) == 0 && (frame_number+grow_phase)%growth_time(form_voxel) == 0)
    {
        c.r = form_voxel;
    }
    else if(mid == 0 && all(greaterThan(pos-origin, ivec3(0))) && all(lessThan(pos-origin, ivec3(32-1))))
    {
        uvec4 growing_cell = uvec4(0);
        if(trig(l) != 0)      growing_cell = l;
        else if(trig(r) != 0) growing_cell = r;
        else if(trig(u) != 0) growing_cell = u;
        else if(trig(d) != 0) growing_cell = d;
        else if(trig(f) != 0) growing_cell = f;
        else if(trig(b) != 0) growing_cell = b;

        uint growing_mat = mat(growing_cell); //material id
        if(growing_mat != 0)
        {
            bool is_cell = growing_mat >= BASE_CELL_MAT;
            if(is_cell) growing_mat += body_cell_material_id(bi);

            uint trigger_data = trigger_info(growing_mat, trig(growing_cell)-1);
            uint condition_type = trigger_data&0xFF;
            uint action_type = (trigger_data>>8)&0xFF;
            uint trigger_material = trigger_data>>16;

            if(action_type == act_grow)
                c.r = mat(growing_cell)+1;
            else
                c.r = trigger_material;
        }
    }

    vec3 voxel_x = apply_rotation(body_orientation(bi), voxel_pos)+body_x(bi);
    //TODO: apply pushback force
    for(int be = 0; be < n_beams; be++)
    {
        vec3 delta = voxel_x-beam_x(be);
        vec3 dhat = normalize(beam_d(be));
        float d = clamp(dot(dhat, delta), 0.0, length(beam_d(be)));
        vec3 nearest_x = d*dhat+beam_x(be);
        vec3 r = voxel_x-nearest_x;
        if(dot(r, r) <= sq(beams[be].r))
        {
            temp = clamp(temp+100, 0u, 255u);
        }
    }

    if(float(temp) > melting_point(mat(c))) c.r = 0;
    if(float(temp) > boiling_point(mat(c))) c.r = 0;

    if(mat(c) == 0) temp = room_temp;

    uvec4 out_voxel = c;

    int depth = MAX_DEPTH-1;
    bool filledness = mat(c) != 0;
    if(((mat(u) != 0) != filledness) ||
       ((mat(d) != 0) != filledness) ||
       ((mat(r) != 0) != filledness) ||
       ((mat(l) != 0) != filledness) ||
       ((mat(f) != 0) != filledness) ||
       ((mat(b) != 0) != filledness)) depth = 0;
    else
    {
        depth = min(depth, depth(u)+1);
        depth = min(depth, depth(d)+1);
        depth = min(depth, depth(r)+1);
        depth = min(depth, depth(l)+1);
        depth = min(depth, depth(f)+1);
        depth = min(depth, depth(b)+1);
    }

    bool inside = false;
    bool outside = false;
    for(int z = 0; z < 2; z++)
        for(int y = 0; y < 2; y++)
            for(int x = 0; x < 2; x++)
            {
                ivec3 p = ivec3(pos.x-x, pos.y-y, pos.z-z);
                uvec4 vox;
                if(any(lessThan(p, body_texture_lower(bi))) || any(greaterThan(p, body_texture_upper(bi))))
                    vox = uvec4(0);
                else
                    vox = texelFetch(body_materials, p, 0);
                if(mat(vox) != 0 && (body_fragment_id(bi) == 0 || floodfill(vox) == body_fragment_id(bi)))
                {
                    inside = true;
                }
                else
                {
                    outside = true;
                }
            }

    if(inside && outside)
    {
        vec3 body_coord = pos-body_texture_lower(bi);

        // int n_subdivisions = max(int(length(body_x(bi)-body_old_x(bi))*5.0), 1);
        int n_subdivisions = 1;
        float scale = 1.0/max(float(n_subdivisions-1), 1.0);
        vec3 r = body_coord-body_x_cm(bi);

        for(int i = n_subdivisions-1; i >= 0; i--)
        {
            float t = 1-i*scale;
            vec4 orientation = mix(body_old_orientation(bi), body_orientation(bi), t); //who needs slerp
            orientation = normalize(orientation);
            vec3 x = mix(body_old_x(bi), body_x(bi), t);
            vec3 world_coord = apply_rotation(orientation, r)+x;

            ivec3 wvc = ivec3(world_coord); //world_voxel_coord
            uvec4 world_voxel = texelFetch(materials, wvc, 0);
            int world_depth = signed_depth(world_voxel);

            if(world_depth <= 0)
            {
                vec3 normal = unnormalized_gradient(materials, wvc);
                normal = normalize(normal);
                vec3 nsign = sign(normal);
                vec3 nabs = normal*nsign;

                vec3 main_dir = step(nabs.yzx, nabs.xyz)*step(nabs.zyx, nabs.xyz);
                vec3 adjusted_world_coord = mix(world_coord, nsign*ceil(nsign*world_coord), main_dir);

                add_world_contact(bi, world_voxel, adjusted_world_coord, body_coord, normal, world_depth);
                break;
            }
        }
    }

    out_voxel.g = 0;
    // out_voxel.g = uint(depth);
    // out_voxel.g = uint(depth) | (phase << 6) | (transient << 7);

    out_voxel.b = temp;

    out_voxel.a = volt | (trig << 4);

    imageStore(body_materials_out, pos, out_voxel);
}
