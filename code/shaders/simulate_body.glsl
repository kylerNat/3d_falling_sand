#program simulate_body_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"

layout(location = 0) in vec3 x;

layout(location = 0) uniform int layer;
layout(location = 6) uniform int n_bodies;

#include "include/body_data.glsl"
#define FORM_DATA_BINDING 1
#include "include/form_data.glsl"
#define PARTICLE_DATA_BINDING 2
#include "include/particle_data.glsl"

flat out int bi;
flat out ivec3 origin;

void main()
{
    bi = gl_InstanceID;

    // get_body_data(b);
    body_materials_origin = ivec3(bodies[bi].materials_origin_x,
                                  bodies[bi].materials_origin_y,
                                  bodies[bi].materials_origin_z);
    body_size = ivec3(bodies[bi].size_x,
                      bodies[bi].size_y,
                      bodies[bi].size_z);

    float scale = 2.0/256.0;

    gl_Position = vec4(0.0/0.0, 0.0/0.0, 0, 1);

    const int padding = 1;

    origin = body_materials_origin;

    if(body_materials_origin.z-padding <= layer && layer < body_materials_origin.z+body_size.z+padding)
    {
        gl_Position.xy = scale*(body_materials_origin.xy-padding+(body_size.xy+2*padding)*x.xy)-1.0;
    }

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
}

/////////////////////////////////////////////////////////////////

#shader GL_FRAGMENT_SHADER
#include "include/header.glsl"

layout(location = 0) out uvec4 out_voxel;

layout(location = 0) uniform int layer;
layout(location = 1) uniform int frame_number;
layout(location = 2) uniform sampler2D material_physical_properties;
layout(location = 3) uniform usampler3D materials;
layout(location = 4) uniform usampler3D body_materials;
layout(location = 5) uniform usampler3D form_materials;

#define MATERIAL_PHYSICAL_PROPERTIES
#include "include/materials_physical.glsl"

#include "include/body_data.glsl"
#define FORM_DATA_BINDING 1
#include "include/form_data.glsl"
#define PARTICLE_DATA_BINDING 2
#include "include/particle_data.glsl"


flat in int bi;
flat in ivec3 origin;

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

void main()
{
    ivec3 pos;
    pos.xy = ivec2(gl_FragCoord.xy);
    pos.z = layer;

    get_body_data(bi);
    if(body_form_id != 0) get_form_data(body_form_id-1);

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

    ivec3 form_pos = pos-body_materials_origin+form_materials_origin;
    uint form_voxel = texelFetch(form_materials, form_pos, 0).r;

    int spawned_cell = 0;

    uint temp = temp(c);
    uint volt = volt(c);
    uint trig = 0;

    uint mid = mat(c); //material id
    bool is_cell = mid >= BASE_CELL_MAT;
    if(is_cell) mid += body_cell_material_id;

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
    if(form_is_mutating==1 && mid != form_voxel)
    {
        c = uvec4(0);
    }

    if(mid != form_voxel && form_voxel != 0 && depth(c) == 0 && frame_number%growth_time(form_voxel) == 0)
    {
        c.r = form_voxel;
    }
    else if(mid == 0 && all(greaterThan(pos-origin, ivec3(0))) && all(lessThan(pos-origin, ivec3(32-1))))
    {
        uvec4 growing_cell = uvec4(0);
        if(trig(l) != 0)       growing_cell = l;
        else if(trig(r) != 0)  growing_cell = r;
        else if(trig(u) != 0)  growing_cell = u;
        else if(trig(d) != 0)  growing_cell = d;
        else if(trig(f) != 0)  growing_cell = f;
        else if(trig(b) != 0) growing_cell = b;

        uint growing_mat = mat(growing_cell); //material id
        if(growing_mat != 0)
        {
            bool is_cell = growing_mat >= BASE_CELL_MAT;
            if(is_cell) growing_mat += body_cell_material_id;

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

    // c.r = 1;
    out_voxel = c;

    int depth = MAX_DEPTH-1;
    bool filledness = mat(c) != 0 && transient(c)==0;
    if(((mat(u) != 0 && transient(u) == 0) != filledness) ||
       ((mat(d) != 0 && transient(d) == 0) != filledness) ||
       ((mat(r) != 0 && transient(r) == 0) != filledness) ||
       ((mat(l) != 0 && transient(l) == 0) != filledness) ||
       ((mat(f) != 0 && transient(f) == 0) != filledness) ||
       ((mat(b) != 0 && transient(b) == 0) != filledness)) depth = 0;
    else
    {
        depth = min(depth, depth(u)+1);
        depth = min(depth, depth(d)+1);
        depth = min(depth, depth(r)+1);
        depth = min(depth, depth(l)+1);
        depth = min(depth, depth(f)+1);
        depth = min(depth, depth(b)+1);
    }

    out_voxel.g = uint(depth);
    // out_voxel.g = uint(depth) | (phase << 6) | (transient << 7);

    out_voxel.b = temp;

    out_voxel.a = volt | (trig << 4);
}
