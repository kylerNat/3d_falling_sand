#ifndef GAME_COMMON
#define GAME_COMMON

#define N_SOLVER_ITERATIONS 20

#define room_size 512

size_t index_3D(int_3 pos, int_3 size)
{
    return pos.x+size.x*(pos.y+size.y*pos.z);
}

size_t index_3D(int_3 pos, int size)
{
    return pos.x+size*(pos.y+size*pos.z);
}

enum ui_type
{
    ui_none,
    ui_gene,
    ui_gizmo,
    ui_form,
    ui_form_voxel,
    ui_form_joint,
    ui_form_foot,
    ui_window,
    ui_button,
    n_ui_elements
};

struct ui_element
{
    int type;
    void* element;
};

struct user_input
{
    real_2 mouse;
    real_2 dmouse;
    int16 mouse_wheel;
    int16 mouse_hwheel;
    byte buttons[32];
    byte pressed_buttons[32];
    byte released_buttons[32];
    bool click_blocked;
    ui_element active_ui_element;
};

#define M1 0x01
#define M2 0x02
#define M3 0x04
#define M4 0x05
#define M5 0x06
#define M_WHEEL_DOWN 0x0A
#define M_WHEEL_UP 0x0B
#define M_WHEEL_LEFT 0x0E
#define M_WHEEL_RIGHT 0x0F

#define LARROW 0x25
#define UARROW 0x26
#define RARROW 0x27
#define DARROW 0x28

#define is_down(key_code, input) ((((input)->buttons[(key_code)/8]>>((key_code)%8)) | ((input)->pressed_buttons[(key_code)/8]>>((key_code)%8)))&1) //check pressed_buttons in case there was a press and release within a single frame
#define is_pressed(key_code, input) ((input)->pressed_buttons[(key_code)/8]>>((key_code)%8)&1)
#define is_released(key_code, input) ((input)->released_buttons[(key_code)/8]>>((key_code)%8)&1)

#define set_key_down(key_code, input) (((input).buttons[(key_code)/8] |= 1<<((key_code)%8)), ((input).pressed_buttons[(key_code)/8] |= 1<<((key_code)%8)))
#define set_key_up(key_code, input) (((input).buttons[(key_code)/8] &= ~(1<<((key_code)%8))), ((input).released_buttons[(key_code)/8] |= 1<<((key_code)%8)))

enum storage_mode
{
    sm_none,
    sm_disk,
    sm_compressed,
    sm_memory,
    sm_gpu,
};

enum joint_type
{
    joint_root,
    joint_ball,
    joint_condyloid,
    joint_hinge,
};

enum endpoint_type
{
    endpoint_eye,
    endpoint_hand,
    endpoint_foot,
    n_endpoint_types,
};

enum target_value_type
{
    tv_velocity,
    tv_angular_velocity,
};

enum foot_state
{
    foot_lift,
    foot_swing,
    foot_down,
    foot_push,
};

struct real_bounding_box
{
    real_3 l;
    real_3 u;
};

real_bounding_box expand_to(real_bounding_box b, real_3 p)
{
    b.l = min_per_axis(b.l, p);
    b.u = max_per_axis(b.u, p);
    return b;
}

#define max_bodies_per_cell 1024
#define collision_cell_size 32
#define collision_cells_per_axis 16
struct collision_cell
{
    int body_indices[max_bodies_per_cell];
    int n_bodies;
};

struct ray_hit
{
    bool hit;
    int_3 pos;
    int_3 dir;
    real dist;
};

ray_hit cast_ray(uint8* materials, real_3 ray_origin, real_3 ray_dir, int_3 size, int max_iterations)
{
    real_3 ray_sign = sign_not_zero_per_axis(ray_dir);
    real_3 inv_ray_dir = divide_components({1,1,1}, ray_dir);
    real_3 invabs_ray_dir = divide_components(ray_sign, ray_dir);

    int_3 dir = {};

    int_3 bounding_planes = {ray_dir.x < 0 ? size.x:0, ray_dir.y < 0 ? size.y:0, ray_dir.z < 0 ? size.z:0};
    real_3 bounding_dists = multiply_components((real_cast(bounding_planes)-ray_origin), inv_ray_dir);
    real skip_dist = 0;
    for(int i = 0; i < 3; i++)
    {
        if(bounding_dists[i] > skip_dist)
        {
            skip_dist = bounding_dists[i];
            dir = {};
            dir[i] = ray_sign[i];
        }
    }
    skip_dist += 0.001;
    real_3 pos = ray_origin + skip_dist*ray_dir;
    real total_dist = skip_dist;

    int_3 ipos = int_cast(floor_per_axis(pos));

    int i = 0;
    for ever
    {
        if(ipos.x < 0 || ipos.y < 0 || ipos.z < 0
           || ipos.x >= size.x || ipos.y >= size.y || ipos.z >= size.z)
        {
            return {false};
        }

        if(materials[index_3D(ipos, size)] != 0)
        {
            return {true, ipos, dir, total_dist};
        }

        real_3 dist = multiply_components((0.5f*ray_sign+(real_3){0.5f,0.5f,0.5f}
                                           +multiply_components(ray_sign, (real_cast(ipos)-pos))),
                                          invabs_ray_dir);

        int_3 min_dir = {};
        real min_dist = 0;
        if(dist.x < dist.y && dist.x < dist.z) {min_dist = dist.x; min_dir = {1,0,0};}
        if(dist.y < dist.z && dist.y < dist.x) {min_dist = dist.y; min_dir = {0,1,0};}
        if(dist.z < dist.x && dist.z < dist.y) {min_dist = dist.z; min_dir = {0,0,1};}
        pos += min_dist*ray_dir;
        dir = multiply_components(min_dir, int_cast(ray_sign));
        ipos += dir;
        total_dist += min_dist;

        if(++i > max_iterations)
        {
            return {false};
        }
    }
}

struct ring_hit
{
    bool hit;
    real_3 nearest; //nearest position on the ring, regardless if there is a hit or not
    real_3 pos; //position on the surface of the torus, if there was a hit
    real dist; //distance the ray traveled to the nearest point/torus surface
};

ring_hit project_to_ring(real_3 ray_origin, real_3 ray_dir, real_3 center, real major_radius, real minor_radius, real_3 axis)
{
    real_3 circle_point = -major_radius*ray_dir;
    real_3 initial_line_point = ray_origin-center;
    real_3 line_point = initial_line_point;
    real_3 d = {};

    for(int i = 0; i < 50; i++)
    {
        //update line_point to nearst point on the line to circle_point
        d = line_point-circle_point;
        line_point -= dot(ray_dir, d)*ray_dir;

        //update circle_point to nearst point on the circle to line_point
        d = line_point-circle_point;
        circle_point = major_radius*normalize_or_zero((line_point-dot(line_point, axis)*axis));
    }
    //if the line_point traveled net backwards, bring it back to the initial point
    if(dot(line_point-initial_line_point, ray_dir) < 0) line_point = initial_line_point;
    d = line_point-circle_point;

    real_3 nearest = circle_point;
    real dsq = normsq(d);
    real nearest_dsq = dsq;

    if(dsq > sq(minor_radius))
    {
        //check the minima that's away from the camera
        circle_point = 2*major_radius*ray_dir;
        line_point = initial_line_point;
        d = {};

        for(int i = 0; i < 50; i++)
        {
            //update line_point to nearst point on the line to circle_point
            d = line_point-circle_point;
            line_point -= dot(ray_dir, d)*ray_dir;

            //update circle_point to nearst point on the circle to line_point
            d = line_point-circle_point;
            circle_point = major_radius*normalize_or_zero((line_point-dot(line_point, axis)*axis));
        }
        dsq = normsq(d);
        if(dsq < nearest_dsq)
        {
            nearest = circle_point;
            nearest_dsq = dsq;
        }

        //if neither minima matches then there is no intersection
        if(dsq > sq(minor_radius))
        {
            return {false, nearest+center, nearest+center, max(0.0f, dot(nearest-initial_line_point, ray_dir))};
        }
    }
    //if the line_point traveled net backwards, bring it back to the initial point
    if(dot(line_point-initial_line_point, ray_dir) < 0) line_point = initial_line_point;
    d = line_point-circle_point;

    //iteratively find the surface of a sphere inside the torus centered at circle_point, this should converge to the torus surface
    //using an ellipsoid might converge faster
    for(int i = 0; i < 5; i++)
    {
        d = line_point-circle_point;
        if(dot(d,d) > sq(minor_radius)) break;
        line_point -= sqrt(sq(minor_radius)-dot(d,d))*ray_dir;

        d = line_point-circle_point;
        circle_point = major_radius*normalize_or_zero((line_point-dot(line_point, axis)*axis));
    }
    real ray_dist = dot(line_point-initial_line_point, ray_dir);
    ray_dist = max(0.0f, ray_dist);
    return {true, circle_point+center, line_point+center, ray_dist};
}

ring_hit project_to_line(real_3 ray_origin, real_3 ray_dir, real_3 start, real length, real radius, real_3 axis)
{
    real_3 d = start-ray_origin;
    real_3 perp = normalize(cross(axis, ray_dir));
    real_3 dplane = d-dot(perp, d)*perp;
    real_3 dplaneperp = dplane-dot(dplane, axis)*axis;
    real_3 planeperpdir = normalize(dplaneperp);
    real   ray_dist = max(0.0f, dot(dplaneperp, planeperpdir)/dot(ray_dir, planeperpdir));
    real_3 ray_point = ray_origin+ray_dist*ray_dir;
    real   line_dist = clamp(dot(ray_point-start, axis), 0.0f, length);
    real_3 line_point = start+line_dist*axis;

    d = ray_point-line_point;
    if(normsq(d) > sq(radius))
    {
        return {false, line_point, line_point, ray_dist};
    }

    //iteratively find the surface of a sphere inside the tube centered at line_point to converge to the surface of the tube
    for(int i = 0; i < 5; i++)
    {
        d = ray_point-line_point;
        if(normsq(d) > sq(radius)) break;
        ray_point -= sqrt(sq(radius)-dot(d,d))*ray_dir;

        real added_dist = clamp(dot(d, axis), 0.0f, length-line_dist);
        line_point = line_point+added_dist*axis;
        line_dist += added_dist;
    }

    ray_dist = dot(ray_point-ray_origin, ray_dir);
    ray_dist = max(0.0f, ray_dist);
    return {true, line_point, ray_point, ray_dist};
}

struct index_table_entry
{
    int id;
    int index; //TODO: make indices above the bodies_cpu list size refer to unloaded bodies
};

#include "genetics.h"
#include "editor.h"

struct rational
{
    int n;
    int d;
};

#pragma pack(push, 1)
struct particle_data
{
    uint8_4 voxel_data;
    real_3 x;
    real_3 x_dot;

    bool32 alive;
};

struct particle_data_list
{
    int n_particles;
    particle_data particles[4096];
};

struct explosion_data
{
    real_3 x;
    real r;
    real strength;
};

struct beam_data
{
    real_3 x;
    real_3 dir;
    real r;
    real max_length;
    real strength;
};
#pragma pack(pop)

struct voxel_data
{
    int material;
    int distance; //signed distance to the nearest surface
    real_3 normal;
};

#define N_MAX_CHILDREN 16
#define N_MAX_ENDPOINTS 16

struct body_joint
{
    int type;
    int body_id[2];
    int_3 pos[2];
    int axis[2];

    real_3 deltap;
    real_3 deltaL;
    real_3 pseudo_force;
    real_3 pseudo_torque;
};

#pragma pack(push, 1)
struct gpu_body_joint
{
    int body_index[2];
    int_3 texture_pos[2];
    int axis[2];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct gpu_body_data
{
    bounding_box texture_region;
    int_3 origin_to_lower;
    real_3 x_cm;
    real_3 x;
    real_3 x_dot;
    quaternion orientation;
    real_3 omega;
    real_3 old_x;
    quaternion old_orientation;
    real m;
    real_3x3 I;
    real_3x3 invI;

    real_bounding_box box; //world space bounding box

    int cell_material_id; //material_id of cell type 0 for this body
    int is_mutating;
    int substantial;

    int brain_id;
};
#pragma pack(pop)

#define MAX_DEPTH 32

struct world_cell
{
    union
    {
        struct
        {
            uint8 material;
            uint8 depth:5;
            uint8 temperature;
            uint8 voltage:4;
            uint8 flow:4;
        };
        uint8 data[4];
        uint32 data32;
    };
};

struct body_cell
{
    uint8 material;
    int8 depth;
    uint8 temperature;
    int8 voltage;
};

struct cpu_body_data
{
    int id;

    int genome_id;
    int form_id;

    body_cell* materials;
    //NOTE: I think I need to move region into gpu data
    bounding_box region; //the region that is stored in memory

    int_3 updated_region; //region that needs to get sent to the gpu //TODO: could make this multiple regions if necessary

    bool is_root;
    int root;

    //these are in body coordinates, while the gpu versions are in world coordinates
    real_3x3 I;

    bool has_contact;
    bool phasing;

    int first_fragment_index;
    int n_fragments;
};

void resize_body(cpu_body_data* body_cpu, bounding_box new_region)
{
    bounding_box old_region = body_cpu->region;
    if(old_region == new_region) return;

    int_3 new_size = dim(new_region);
    int_3 old_size = dim(old_region);

    int_3 offset = new_region.l-body_cpu->region.l;

    size_t new_size_total = new_size.x*new_size.y*new_size.z*sizeof(body_cell);
    body_cell* new_materials = (body_cell*) dynamic_alloc(new_size_total);

    //find the union between the old and new regions and copy from old to new within the union
    int_3 lower = max_per_axis(new_region.l, old_region.l);
    int_3 upper = min_per_axis(new_region.u, old_region.u);

    memset(new_materials, 0, new_size_total);

    for(int z = lower.z; z < upper.z; z++)
        for(int y = lower.y; y < upper.y; y++)
        {
            int x = lower.x;

            int nx = x-new_region.l.x;
            int ny = y-new_region.l.y;
            int nz = z-new_region.l.z;

            int ox = x-old_region.l.x;
            int oy = y-old_region.l.y;
            int oz = z-old_region.l.z;

            memcpy(new_materials+index_3D({nx,ny,nz}, new_size), body_cpu->materials+index_3D({ox, oy, oz}, old_size), (upper.x-lower.x)*sizeof(body_cell));
        }

    dynamic_free(body_cpu->materials);

    body_cpu->region = new_region;
    body_cpu->materials = new_materials;
}

body_cell get_cell(cpu_body_data* body_cpu, int_3 pos)
{
    if(all_less_than_eq(body_cpu->region.l, pos) && all_less_than(pos, body_cpu->region.u))
    {
        return body_cpu->materials[index_3D(pos-body_cpu->region.l, dim(body_cpu->region))];
    }
    return {};
}

void set_cell(cpu_body_data* body_cpu, int_3 pos, body_cell cell)
{
    if(!(all_less_than_eq(body_cpu->region.l, pos) && all_less_than(pos, body_cpu->region.u)))
    {
        bounding_box new_region = expand_to(body_cpu->region, pos);
        resize_body(body_cpu, new_region);
    }
    body_cpu->materials[index_3D(pos-body_cpu->region.l, dim(body_cpu->region))] = cell;
}

#define MAX_COYOTE_TIME 6.0f

#pragma pack(push, 1)
struct contact_point
{
    int body_index[2];
    uint material[2];
    uint phase;
    real depth[2];
    real_3 pos[2];
    real_3 normal;
    real_3 force;
};

struct body_update
{
    real m;
    real I_xx = 0;
    real I_xy = 0; real I_yy = 0;
    real I_xz = 0; real I_yz = 0; real I_zz = 0;
    real_3 x_cm;

    int_3 lower;
    int_3 upper;

    int fragment_id;
    int n_fragments;
};

struct joint_update
{
    int fragment_id[2];
};
#pragma pack(pop)

struct endpoint
{
    int type;
    int body_id;
    int_3 pos;

    bool is_grabbing;
    int grab_target;
    real_3 grab_point;
    real_3 grab_normal; //in body space of the grab target

    real coyote_time;

    real foot_phase;

    //+ means going away from the ground, - means going toward, and close to 0 means only moving forward
};

struct brain
{
    int id;
    int root_id;
    int* body_ids;
    int n_bodies;
    int n_max_bodies;
    endpoint* endpoints;
    int n_endpoints;
    real_3 look_dir;
    real_3 target_accel;
    real_3 stabilize_accel;
    real_3 kick;
    int kick_frames;
    bool is_moving;

    int genome_id;
};

struct entity
{
    real_3 x;
    real_3 x_dot;
};
#include "room.h"

struct debug_menu_t
{
    real plotline[1000];
    int n_plotline_points;
    char plotname[100];

    bool body_inspector_active;
    int active_body;
    int active_joint;
};

debug_menu_t debug_menu;

#define debug_menu_active (debug_menu.body_inspector_active)

enum brush_shape
{
    bs_cube,
    bs_sphere,
    n_brush_shapes
};

#pragma pack(push, 1)
struct brush_stroke
{
    int material;
    int shape;
    real_3 x;
    real r;
};

struct ray_in
{
    real_3 x;
    real_3 d;
    float max_dist;
    int start_material;
};

struct ray_out
{
    int material; //-1 for no hit
    int_3 pos;
    real dist;
};
#pragma pack(pop)

struct world
{
    uint32 seed;
    entity player;

    index_table_entry* body_table;
    int n_max_bodies;
    int next_body_id;
    cpu_body_data * bodies_cpu;
    gpu_body_data * bodies_gpu;
    int n_bodies;

    int* body_fragments; //contains a list of body ids for bodies that have just been fragmented off other bodies
    int n_body_fragments;

    body_joint* joints;
    int n_joints;

    contact_point* contacts;
    int n_contacts;

    index_table_entry* brain_table;
    int n_max_brains;
    int next_brain_id;
    brain* brains;
    int n_brains;

    index_table_entry* genome_table;
    int n_max_genomes;
    int next_genome_id;
    genome* genomes;
    int n_genomes;

    genedit_window gew;

    editor_data editor;

    cuboid_space form_space;

    cuboid_space body_space;

    bool edit_mode;
    bool world_edit_mode;

    explosion_data* explosions;
    int n_explosions;

    beam_data* beams;
    int n_beams;

    ray_in* rays_in;
    ray_out* rays_out;
    int n_rays;

    room * c;
    int8_2 chunk_lookup[2*2*2];

    collision_cell* collision_grid;

    int frame_number;
};

brain* create_brain(world* w)
{
    for(int i = 0; i < w->n_max_brains; i++)
    {
        int id = w->next_brain_id++;
        index_table_entry* entry = &w->brain_table[id % w->n_max_brains];
        if(entry->id == 0)
        {
            entry->id = id;
            entry->index = w->n_brains++;
            w->brains[entry->index].id = id;
            return &w->brains[entry->index];
        }
    }
    return 0;
}

brain* get_brain(world* w, int id)
{
    if(id == 0) return 0;
    index_table_entry entry = w->brain_table[id % w->n_max_brains];
    if(entry.id == id)
    {
        return &w->brains[entry.index];
    }
    return 0;
}

void delete_brain(world* w, int id)
{
    index_table_entry* entry = &w->brain_table[id % w->n_max_brains];
    if(entry->id == id)
    {
        entry->id = 0;
        w->brains[entry->index] = w->brains[--w->n_brains];
        int moved_id = w->brains[entry->index].id;
        if(w->brain_table[moved_id % w->n_max_brains].id == moved_id)
        {
            w->brain_table[moved_id % w->n_max_brains].index = entry->index;
        }
    }
}

genome* create_genome(world* w)
{
    for(int i = 0; i < w->n_max_genomes; i++)
    {
        int id = w->next_genome_id++;
        index_table_entry* entry = &w->genome_table[id % w->n_max_genomes];
        if(entry->id == 0)
        {
            entry->id = id;
            entry->index = w->n_genomes++;
            w->genomes[entry->index].id = id;
            return &w->genomes[entry->index];
        }
    }
    return 0;
}

genome* get_genome(world* w, int id)
{
    if(id == 0) return 0;
    index_table_entry entry = w->genome_table[id % w->n_max_genomes];
    if(entry.id == id)
    {
        return &w->genomes[entry.index];
    }
    return 0;
}

void delete_genome(world* w, int id)
{
    index_table_entry* entry = &w->genome_table[id % w->n_max_genomes];
    if(entry->id == id)
    {
        entry->id = 0;
        w->genomes[entry->index] = w->genomes[--w->n_genomes];
        int moved_id = w->genomes[entry->index].id;
        if(w->genome_table[moved_id % w->n_max_genomes].id == moved_id)
        {
            w->genome_table[moved_id % w->n_max_genomes].index = entry->index;
        }
    }
}

int new_body_index(world* w)
{
    for(int i = 0; i < w->n_max_bodies; i++)
    {
        int id = w->next_body_id++;
        index_table_entry* entry = &w->body_table[id % w->n_max_bodies];
        if(entry->id == 0)
        {
            entry->id = id;
            entry->index = w->n_bodies++;
            w->bodies_cpu[entry->index] = {};
            w->bodies_gpu[entry->index] = {};
            w->bodies_cpu[entry->index].id = id;
            return entry->index;
        }
    }
    return -1;
}

int get_body_index(world* w, int id)
{
    if(id == 0) return -1;
    index_table_entry entry = w->body_table[id % w->n_max_bodies];
    if(entry.id == id)
    {
        return entry.index;
    }
    return -1;
}

void delete_body(world* w, int id)
{
    index_table_entry* entry = &w->body_table[id % w->n_max_bodies];
    if(entry->id == id)
    {
        entry->id = 0;

        if(w->bodies_cpu[entry->index].materials) dynamic_free(w->bodies_cpu[entry->index].materials);
        w->bodies_cpu[entry->index] = w->bodies_cpu[--w->n_bodies];
        w->bodies_gpu[entry->index] = w->bodies_gpu[  w->n_bodies];

        int moved_id = w->bodies_cpu[entry->index].id;
        if(w->body_table[moved_id % w->n_max_bodies].id == moved_id)
        {
            w->body_table[moved_id % w->n_max_bodies].index = entry->index;
        }
    }
}

//NOTE: bounding boxes seem larger than necessary, not sure if it's a bug or just needs voxel-level detection
void update_bounding_box(cpu_body_data* body_cpu, gpu_body_data* body_gpu)
{
    for(int i = 0; i < 3; i++)
    {
        real_3 axis = {};
        axis[i] = 1;
        real_3 signs = apply_rotation(conjugate(body_gpu->orientation), axis);
        real_3 l;
        real_3 u;
        //find the corners in the directions of +- axis
        for(int j = 0; j < 3; j++)
        {
            if(signs[j] > 0)
            {
                l[j] = body_cpu->region.l[j];
                u[j] = body_cpu->region.u[j];
            }
            else
            {
                l[j] = body_cpu->region.u[j];
                u[j] = body_cpu->region.l[j];
            }
        }
        body_gpu->box.l[i] = apply_rotation(body_gpu->orientation, l-body_gpu->x_cm)[i];
        body_gpu->box.u[i] = apply_rotation(body_gpu->orientation, u-body_gpu->x_cm)[i];
    }
    body_gpu->box.l += body_gpu->x;
    body_gpu->box.u += body_gpu->x;
}

void update_inertia(cpu_body_data* body_cpu, gpu_body_data* body_gpu)
{
    //NOTE: techincally the innermost I should be transposed, but it should be a symmetric matrix
    body_gpu->I = apply_rotation(body_gpu->orientation, transpose(apply_rotation(body_gpu->orientation, body_cpu->I)));
    // log_output("I: ", body_gpu->I, "\n");
    body_gpu->invI = inverse(body_gpu->I);
    // body_gpu->invI = apply_rotation(body_gpu->orientation, transpose(apply_rotation(body_gpu->orientation, transpose(body_cpu->invI))));
}

voxel_data get_voxel_data(int_3 x);

// int get_body_material();

real_3 cm_to_joint(gpu_body_data* body_gpu, body_joint* joint, int a)
{
    return real_cast(joint->pos[a])+(real_3){0.5,0.5,0.5}-body_gpu->x_cm;
}

real_3 cm_to_endpoint(gpu_body_data* body_gpu, endpoint* ep)
{
    return real_cast(ep->pos)+(real_3){0.5,0.5,0.5}-body_gpu->x_cm;
}

//figure out stuff like which feet to raise or lower
void plan_brains(world* w, render_data* rd)
{
    for(int brain_id = 0; brain_id < w->n_brains; brain_id++)
    {
        brain* br = &w->brains[brain_id];
        int n_feet_down = 0;
        int n_feet = 0;

        br->stabilize_accel = {};

        endpoint* most_extended_foot = 0;
        real most_extension = -1000;

        endpoint* most_forward_foot = 0;
        real most_forwardness = -1000;

        for(int e = 0; e < br->n_endpoints; e++)
        {
            endpoint* ep = &br->endpoints[e];

            int b = get_body_index(w, ep->body_id);
            gpu_body_data* body_gpu = &w->bodies_gpu[b];
            cpu_body_data* body_cpu = &w->bodies_cpu[b];

            int b_root = get_body_index(w, body_cpu->root);
            gpu_body_data* root_gpu = &w->bodies_gpu[b_root];
            cpu_body_data* root_cpu = &w->bodies_cpu[b_root];

            real_3 r = apply_rotation(body_gpu->orientation, cm_to_endpoint(body_gpu, ep));

            if(ep->type == endpoint_foot)
            {
                n_feet++;

                real_3 foot_x = r+body_gpu->x;
                real_3 foot_x_dot = body_gpu->x_dot+cross(body_gpu->omega, r);

                // draw_circle(rd, foot_x, 0.1, {1,1,0,1});

                real_3 root_r = foot_x - root_gpu->x;

                if(ep->is_grabbing)
                {
                    n_feet_down++;

                    // real extension = norm(root_r)/ep->root_dist;
                    real root_dist = 1.0;
                    //TODO: could calculate this from distance of the form, but probably want to rework this whole thing instead
                    real extension = -dot(root_r, br->target_accel)/root_dist;
                    if(extension > most_extension)
                    {
                        most_extended_foot = ep;
                        most_extension = extension;
                    }

                    if(br->is_moving)
                    {
                        //make sure there's actually a contact
                        if(!body_cpu->has_contact)
                        {
                            ep->is_grabbing = false;
                            ep->foot_phase = -0.0f;
                        }
                    }
                    else
                    {
                        // for(int i = 0; i < 3; i++)
                        // {
                        //     if(body_cpu->contact_depths[i] <= 1)
                        //     {
                        //         // body_cpu->contact_locked[i] = true;
                        //         ep->grab_point = body_cpu->contact_points[i];
                        //         ep->grab_normal = body_cpu->contact_normals[i];
                        //         break;
                        //     }
                        // }
                        br->stabilize_accel -= 0.005f*root_gpu->x_dot;
                    }

                    // int f = body_cpu->form_id-1;
                    // gpu_form_data* form_gpu = &forms_gpu[f];

                    // int fr = root_cpu->form_id-1;
                    // gpu_form_data* root_form_gpu = &forms_gpu[fr];

                    // real_3 form_r = apply_rotation(form_gpu->orientation, real_cast(ep->pos)+(real_3){0.5,0.5,0.5}-body_gpu->x_cm);
                    // real_3 form_foot_x = form_r+form_gpu->x;
                    // real_3 target_foot_displacement = root_gpu->x + form_foot_x - root_form_gpu->x - foot_x;

                    // br->stabilize_accel -= 0.002f*target_foot_displacement;
                    // br->stabilize_accel.z += 0.008f;
                    br->stabilize_accel.z += 0.01f;

                    // if(ep->foot_phase > 0.0f)
                    // {
                    //     ep->is_grabbing = false;
                    //     ep->foot_phase = 1.0;
                    // }
                    ep->coyote_time = MAX_COYOTE_TIME;

                    // for(int i = 0; i < 3; i++)
                    // {
                    //     if(body_cpu->contact_locked[i] && dot(body_cpu->contact_forces[i], br->target_accel) < -0.3f)
                    //     {
                    //         body_cpu->contact_locked[i] = false;
                    //         ep->is_grabbing = false;
                    //         ep->foot_phase = 1.0f;
                    //     }
                    // }

                    // real_3 grab_x = ep->grab_point;
                    // if(normsq(foot_x-grab_x) > sq(2.0))
                    // {
                    //     ep->is_grabbing = false;
                    //     ep->foot_phase = 1.0f;
                    // }
                }
                else
                {
                    ep->foot_phase = 0;
                    ep->coyote_time -= 1.0f;

                    // real forwardness = dot(root_r, br->target_accel)/ep->root_dist;
                    real forwardness = -ep->foot_phase;
                    if(forwardness > most_forwardness)
                    {
                        most_forward_foot = ep;
                        most_forwardness = forwardness;
                    }

                    if(body_cpu->has_contact)
                    {
                        ep->is_grabbing = true;
                        ep->grab_target = 0;
                    }
                }
            }
        }

        if(n_feet_down > 0)
        {
            br->stabilize_accel /= n_feet_down;
        }

        if(n_feet_down > 1 && br->is_moving)
        {
            if(most_extended_foot)
            {
                // // most_extended_foot->foot_phase = lerp(most_extended_foot->foot_phase, 1, 0.01);
                // most_extended_foot->foot_phase += 0.5;
                // most_extended_foot->foot_phase = clamp(most_extended_foot->foot_phase, -1.0f, 1.0f);

                // if(most_extended_foot->foot_phase > 0.0f)
                {
                    most_extended_foot->is_grabbing = false;
                    most_extended_foot->foot_phase = 1.0f;

                    int b = get_body_index(w, most_extended_foot->body_id);
                    cpu_body_data* body_cpu = &w->bodies_cpu[b];
                    gpu_body_data* body_gpu = &w->bodies_gpu[b];

                    int b_root = get_body_index(w, body_cpu->root);
                    gpu_body_data* root_gpu = &w->bodies_gpu[b_root];

                    body_cpu->phasing = true;

                    // real_3 impulse = {};
                    // impulse += 100.0f*br->target_accel;
                    // impulse.z += 1.0f;
                    // body_gpu->x_dot += impulse;
                    // real_3 r = apply_rotation(body_gpu->orientation, real_cast(most_extended_foot->pos)+(real_3){0.5,0.5,0.5}-body_gpu->x_cm);
                    // body_gpu->omega += body_gpu->invI*body_gpu->m*cross(r, impulse);
                    // root_gpu->x_dot -= (body_gpu->m/root_gpu->m)*impulse;
                }
            }
        }
        // else
        // {
        //     if(most_forward_foot)
        //     {
        //         // most_forward_foot->foot_phase = lerp(most_forward_foot->foot_phase, -1, 0.01);
        //         most_forward_foot->foot_phase -= 0.1*(n_feet-n_feet_down);
        //         most_forward_foot->foot_phase = clamp(most_forward_foot->foot_phase, -1.0f, 1.0f);

        //         gpu_body_data* body_gpu = &bodies_gpu[most_forward_foot->body_id];
        //         real_3 r = apply_rotation(body_gpu->orientation, real_cast(most_forward_foot->pos)+(real_3){0.5,0.5,0.5}-body_gpu->x_cm);
        //         real_3 foot_x = r+body_gpu->x;
        //         // draw_circle(rd, foot_x, 0.3, {1.0,0.8,0.8,1});
        //     }
        // }
    }
}

void iterate_brains(world* w, render_data* rd)
{
    for(int brain_id = 0; brain_id < w->n_brains; brain_id++)
    {
        brain* br = &w->brains[brain_id];

        {
            int b = get_body_index(w, br->root_id);
            gpu_body_data* body_gpu = &w->bodies_gpu[b];
            cpu_body_data* body_cpu = &w->bodies_cpu[b];

            if(body_gpu->substantial)
            {
                real_3 current_look_dir = apply_rotation(body_gpu->orientation, (real_3){1,0,0});
                real_3 current_up_dir = apply_rotation(body_gpu->orientation, (real_3){0,0,1});
                real_3 torque = 0.00*cross(current_up_dir, (real_3){0,0,1});
                torque.z = 0.05*cross(current_look_dir, br->look_dir).z;
                torque -= 0.1*body_gpu->omega;
                body_gpu->omega += (1.0/N_SOLVER_ITERATIONS)*torque;
            }
        }

        for(int e = 0; e < br->n_endpoints; e++)
        {
            endpoint* ep = &br->endpoints[e];
            int b = get_body_index(w, ep->body_id);
            gpu_body_data* body_gpu = &w->bodies_gpu[b];
            cpu_body_data* body_cpu = &w->bodies_cpu[b];

            int b_root = get_body_index(w, body_cpu->root);
            gpu_body_data* root_gpu = &w->bodies_gpu[b_root];
            cpu_body_data* root_cpu = &w->bodies_cpu[b_root];

            if(!root_gpu->substantial || !body_gpu->substantial) continue;

            real_3 r = apply_rotation(body_gpu->orientation, cm_to_endpoint(body_gpu, ep));

            real_3x3 r_dual = {
                0   ,+r.z,-r.y,
                -r.z, 0  ,+r.x,
                +r.y,-r.x, 0
            };

            real_3x3 K = real_identity_3(1.0) - body_gpu->m*r_dual*body_gpu->invI*r_dual;
            real_3x3 invK = inverse(K);

            real_3 force = {}; //force between the endpoint and the root body
            real_3 grab_force = {}; //force between the grabbed object and the root body
            real_3 endpoint_force = {}; //force directly applied to the endpoint

            // //air control
            // grab_force -= (0.04f/N_SOLVER_ITERATIONS)*br->target_accel;

            if(ep->type == endpoint_foot)
            {
                real_3 foot_x = r+body_gpu->x;
                real_3 foot_x_dot = body_gpu->x_dot+cross(body_gpu->omega, r);

                if(ep->coyote_time > 0) //the foot is on the ground
                {
                    real_3 foot_force = -1.0f*br->target_accel;
                    // quaternion ground_orientation = get_rotation_between(body_cpu->contact_normals[0], {0,0,1});
                    //TODO: re-add world gradient detection
                    quaternion ground_orientation = {1,0,0,0};
                    if(abs(ground_orientation.r) > cos(pi/6+0.001))
                    {
                        foot_force.z = 0;
                        foot_force = apply_rotation(ground_orientation, foot_force);
                        foot_force.z += -br->target_accel.z;
                    }
                    foot_force += -br->stabilize_accel;

                    // draw_circle(rd, root_gpu->x, 0.1, {1,1,0,1});
                    // draw_circle(rd, root_gpu->x+100*foot_force, 0.1, {1,1,1,1});

                    real min_normal_force = 1.0f;
                    real normal_force = max(-dot(foot_force, ep->grab_normal), -min_normal_force);
                    real_3 tangent_force = rej(foot_force, ep->grab_normal);

                    // rd->log_pos
                    //     += sprintf(rd->debug_log+rd->log_pos, "normal_force = %f, tangent_force = %f\n",
                    //                normal_force, norm(tangent_force));

                    real_3 grab_x = ep->grab_point;
                    real_3 grab_normal = ep->grab_normal;

                    // //constrain to friction cone
                    // if(normsq(tangent_force) > sq(max_tangent_force))
                    // {
                    //     tangent_force = max_tangent_force*normalize(tangent_force);
                    // }
                    // foot_force = tangent_force - normal_force*ep->grab_normal;

                    // if(body_cpu->contact_depths[0] <= 1)
                    // {
                    //     grab_force = (1.0f/N_SOLVER_ITERATIONS)*foot_force;
                    //     grab_force *= (1.0f/MAX_COYOTE_TIME)*ep->coyote_time;
                    //     grab_force *= 0.5;
                    //     // force = (1.0f/N_SOLVER_ITERATIONS)*foot_force;
                    //     // force *= (1.0f/MAX_COYOTE_TIME)*ep->coyote_time;
                    // }

                    force = (1.0f/N_SOLVER_ITERATIONS)*foot_force;
                    // force *= (1.0f/MAX_COYOTE_TIME)*ep->coyote_time;

                    if(ep->grab_target)
                    {
                        int b = get_body_index(w, ep->grab_target);
                        gpu_body_data* grabbed_gpu  = &w->bodies_gpu[b];
                        real_3 grabbed_r = apply_rotation(grabbed_gpu->orientation, ep->grab_point);
                        grab_x = grabbed_gpu->x+grabbed_r;

                        grab_normal = apply_rotation(grabbed_gpu->orientation, ep->grab_normal);
                    }

                    // force = (1.0f/N_SOLVER_ITERATIONS)*0.05f*(grab_x-foot_x);
                    // endpoint_force += (1.0f/N_SOLVER_ITERATIONS)*0.01f*dot(grab_x-foot_x, grab_normal)*(grab_normal);
                    // endpoint_force += (1.0f/N_SOLVER_ITERATIONS)*0.001f*(grab_x-foot_x);

                    // if(!br->is_moving)
                    // {
                    //     force += -(0.5f/N_SOLVER_ITERATIONS)*br->stabilize_accel;
                    //     // force += (1.0f/N_SOLVER_ITERATIONS)*0.001f*(grab_x-foot_x);
                    //     force += (1.0f/N_SOLVER_ITERATIONS)*(real_3){0,0,-0.012};
                    // }
                    // draw_circle(rd, grab_x, 0.3f, {0,1,1,1});
                }
                if(!ep->is_grabbing) //the foot is swinging forward
                {
                    real_3 forward_force = 1.0f*(br->target_accel);
                    // real_3 forward_force = 0.02f*(br->target_accel)-0.001*(foot_x_dot-root_gpu->x_dot);

                    // quaternion ground_orientation = get_rotation_between(body_cpu->contact_normals[0], {0,0,1});
                    // if(abs(ground_orientation.r) > cos(pi/6+0.001))
                    // {
                    //     forward_force.z = 0;
                    //     forward_force = apply_rotation(ground_orientation, forward_force);
                    //     forward_force.z += br->target_accel.z;
                    // }

                    real_3 ground_force = {}; //force to move toward/away from walls
                    real ground_distance = 0;
                    // for(int i = 0; i < 3; i++)
                    // {
                    //     ground_force += body_cpu->contact_normals[i];
                    //     ground_distance += body_cpu->contact_depths[i];
                    // }
                    real_3 world_gradient = {0,0,1}; //TODO
                    ground_force = world_gradient;
                    ground_force *= -0.01;
                    // ground_force = -0.05f*(1.0f/3.0f)*body_cpu->contact_normals[0];
                    ground_distance *= (1.0f/3.0f);

                    forward_force *= ep->foot_phase;

                    forward_force = rej(forward_force, world_gradient);

                    // log_output("depth: ", ground_distance, "\n");

                    // real mix_factor = clamp(1.0f-0.1f*ground_distance, 0.0f, 1.0f);
                    real mix_factor = clamp(sq(ep->foot_phase), 0.0f, 1.0f);
                    // if(body_cpu->contact_depths[0] > ep->root_dist/2) mix_factor = 0;
                    // force = (1.0f/N_SOLVER_ITERATIONS)*lerp(forward_force, ground_force, mix_factor);
                    force = (1.0f/N_SOLVER_ITERATIONS)*(forward_force+ground_force);
                    // force = (1.0f/N_SOLVER_ITERATIONS)*ground_force;

                    // draw_circle(rd, foot_x, 0.3f, {mix_factor, 0.5f+0.5f*ep->foot_phase, 1.0f-mix_factor, 1});
                    // draw_circle(rd, foot_x+100*force, 0.2f, {1, 1, 1, 1});
                }

                force += (1.0/N_SOLVER_ITERATIONS)*br->kick;

                genome* g = get_genome(w, body_cpu->genome_id);

                int f = body_cpu->form_id;
                form_t* form = &g->forms[f];

                genome* gr = get_genome(w, root_cpu->genome_id);

                int fr = root_cpu->form_id;
                form_t* root_form = &gr->forms[fr];

                real_3 form_r = apply_rotation(form->orientation, real_cast(ep->pos)+(real_3){0.5,0.5,0.5}); //from x_cm to foot
                real_3 form_foot_x = form_r+form->x;
                real_3 root_form_x = root_form->x+apply_rotation(root_form->orientation, root_gpu->x_cm);
                // real_2 forward_dir = apply_rotation(root_gpu->orientation, (real_3){1,0,0}).xy;
                real_2 forward_dir = br->look_dir.xy;
                forward_dir = normalize(forward_dir);
                if(forward_dir.x != forward_dir.x) forward_dir = {1,0};
                real_3 target_foot_x = root_gpu->x + complexx_xy(forward_dir, form_foot_x - root_form_x);
                real_3 target_foot_displacement = target_foot_x - foot_x;

                // draw_circle(rd, target_foot_x, 0.2, {1,0.5,0,1});

                force += (0.002f/N_SOLVER_ITERATIONS)*target_foot_displacement;
            }

            // if(ep->is_grabbing)
            {
                if(ep->grab_target) //0 means grabbed to the world, so nothing else to apply a grab_force to
                {
                    int b = get_body_index(w, ep->grab_target);
                    gpu_body_data* grabbed_gpu  = &w->bodies_gpu[b];
                    cpu_body_data* grabbed_cpu  = &w->bodies_cpu[b];

                    real_3 grabbed_r = apply_rotation(grabbed_gpu->orientation, ep->grab_point);

                    grabbed_gpu->x_dot += (1.0f/grabbed_gpu->m)*grab_force;
                    grabbed_gpu->omega += root_gpu->invI*cross(grabbed_r, grab_force);
                }

                root_gpu->x_dot -= (1.0f/root_gpu->m)*grab_force;
                // root_gpu->omega -= root_gpu->invI*cross(root_r, force);
            }
            // else
            {
                real reduced_mass = 1.0f/(1.0f/root_gpu->m+1.0f/body_gpu->m);
                endpoint_force *= reduced_mass/0.125f;
                force *= root_gpu->m/0.125f;

                real_3 net_endpoint_force = force+endpoint_force;
                body_gpu->x_dot += (1.0f/body_gpu->m)*net_endpoint_force;
                body_gpu->omega += body_gpu->invI*cross(r, net_endpoint_force);

                root_gpu->x_dot -= (1.0f/root_gpu->m)*force;
                // root_gpu->omega -= root_gpu->invI*cross(root_r, force);
            }
        }
    }
}

void warm_start_joints(world* w)
{
    for(int j = 0; j < w->n_joints; j++)
    {
        body_joint* joint = &w->joints[j];

        int b0 = get_body_index(w, joint->body_id[0]);
        gpu_body_data* parent_gpu = &w->bodies_gpu[b0];
        cpu_body_data* parent_cpu = &w->bodies_cpu[b0];

        int b1 = get_body_index(w, joint->body_id[1]);
        gpu_body_data* child_gpu = &w->bodies_gpu[b1];
        cpu_body_data* child_cpu = &w->bodies_cpu[b1];

        real warm_fraction = 0.0;
        joint->deltap *= warm_fraction;
        joint->deltaL *= warm_fraction;
        joint->pseudo_force *= warm_fraction;
        joint->pseudo_torque *= warm_fraction;

        real_3 deltap = joint->deltap;
        real_3 deltaL = joint->deltaL;
        real_3 pseudo_force = joint->pseudo_force;
        real_3 pseudo_torque = joint->pseudo_torque;

        for(int a = 0; a <= 1; a++)
        {
            int b = get_body_index(w, joint->body_id[a]);
            gpu_body_data* body = &w->bodies_gpu[b];
            cpu_body_data* body_cpu = &w->bodies_cpu[b];
            real_3 r = apply_rotation(body->orientation, cm_to_joint(body, joint, a));
            real s = (a?1:-1);
            body->x_dot += s*deltap/body->m;
            body->omega += s*body->invI*(cross(r, deltap) + deltaL);

            body->x += s*pseudo_force/body->m;
            real_3 pseudo_omega = s*(body->invI*(cross(r, pseudo_force) + pseudo_torque));
            body->orientation = axis_to_quaternion(pseudo_omega)*body->orientation;

            update_inertia(body_cpu, body);
        }

        // joint->deltap = {};
        // joint->deltaL = {};
        // joint->pseudo_force = {};
        // joint->pseudo_torque = {};
    }
}

void iterate_joints(memory_manager* manager, world* w, bool use_integral)
{
    int* perm = (int*) stalloc(sizeof(int)*w->n_joints);
    for(int i = 0; i < w->n_joints; i++) perm[i] = i;

    for(int j = 0; j < w->n_joints; j++)
    {
        int i = rand(&w->seed, j, w->n_joints);
        swap(perm[i], perm[j]);
        body_joint* joint = &w->joints[perm[j]];

        int b0 = get_body_index(w, joint->body_id[0]);
        gpu_body_data* parent_gpu = &w->bodies_gpu[b0];
        cpu_body_data* parent_cpu = &w->bodies_cpu[b0];

        int b1 = get_body_index(w, joint->body_id[1]);
        gpu_body_data* child_gpu = &w->bodies_gpu[b1];
        cpu_body_data* child_cpu = &w->bodies_cpu[b1];

        if(!child_gpu->substantial || !parent_gpu->substantial) continue;

        assert(child_gpu->omega.x == child_gpu->omega.x);

        if(child_cpu->genome_id)
        {
            genome* g = get_genome(w, child_cpu->genome_id);

            int f = child_cpu->form_id;
            form_t* child_form = &g->forms[f];

            genome* gp = get_genome(w, parent_cpu->genome_id);

            int fp = parent_cpu->form_id;
            form_t* parent_form = &gp->forms[fp];

            //targets in parent coordinates
            quaternion target_rel_orientation = conjugate(parent_form->orientation)*child_form->orientation;

            //apply small force to bring body to target
            quaternion current_rel_orientation = conjugate(parent_gpu->orientation)*child_gpu->orientation;
            real_3 current_rel_x = apply_rotation(conjugate(parent_gpu->orientation), child_gpu->x - parent_gpu->x);

            // real_3 force = (0.3/N_SOLVER_ITERATIONS)*apply_rotation(parent_gpu->orientation, target_rel_x - current_rel_x);

            // force += (1.0/N_SOLVER_ITERATIONS)*(parent_gpu->x_dot-child_gpu->x_dot);

            // real reduced_mass = 1.0/(1.0/parent_gpu->m+1.0/child_gpu->m);
            // force *= reduced_mass;

            // // force = proj(force, parent_gpu->x-child_gpu->x);

            // // parent_gpu->x_dot -= force/parent_gpu->m;
            // // child_gpu->x_dot += force/child_gpu->m;

            // real_3 torque = cross(apply_rotation(parent_gpu->orientation, real_cast(joint->pos[0])+(real_3){0.5,0.5,0.5}-parent_gpu->x_cm), force);

            real_3 torque = 2.0*apply_rotation(parent_gpu->orientation, (target_rel_orientation*conjugate(current_rel_orientation)).ijk);
            torque += 5.0*(parent_gpu->omega-child_gpu->omega);
            torque *= (1.0/N_SOLVER_ITERATIONS);

            real_3x3 reduced_I = inverse(parent_gpu->invI+child_gpu->invI);
            torque = reduced_I*torque;

            parent_gpu->omega -= parent_gpu->invI*torque;
            child_gpu->omega += child_gpu->invI*torque;
        }

        switch(joint->type)
        {
            case joint_ball:
            {
                real_3 u = {0,0,0};
                real mu = 0; //reduced mass
                real_3x3 reduced_I = {};
                real_3 average_joint_pos = {0,0,0};
                real_3x3 K = {};
                for(int a = 0; a <= 1; a++)
                {
                    int b = get_body_index(w, joint->body_id[a]);
                    gpu_body_data* body = &w->bodies_gpu[b];
                    cpu_body_data* body_cpu = &w->bodies_cpu[b];
                    real_3 r = apply_rotation(body->orientation, cm_to_joint(body, joint, a));
                    average_joint_pos += r+body->x;
                    real_3 velocity = cross(body->omega, r)+body->x_dot;
                    u += (a?-1:1)*velocity;
                    mu += 1.0/body->m;
                    reduced_I += body->invI;

                    real_3x3 r_dual = {
                        0   ,+r.z,-r.y,
                        -r.z, 0  ,+r.x,
                        +r.y,-r.x, 0
                    };
                    K += real_identity_3(1.0/body->m) - r_dual*body->invI*r_dual;
                }

                real_3x3 invK = inverse(K);

                mu = 1.0/mu;
                reduced_I = inverse(reduced_I);

                average_joint_pos *= 0.5;

                real kp = 0.2;
                real ki = 0.01;

                real_3 deltap = invK*(kp*u);
                real_3 deltaL = {0,0,0};

                if(use_integral)
                {
                    deltap += joint->deltap;
                    // joint->deltap += ki*u*mu;
                    joint->deltap += invK*(ki*u);
                }

                if(normsq(deltap) > sq(mu*1000))
                {
                    log_output("large joint impulse ", deltap, ", ", joint->deltap, "\n");
                }

                for(int a = 0; a <= 1; a++)
                {
                    int b = get_body_index(w, joint->body_id[a]);
                    gpu_body_data* body = &w->bodies_gpu[b];
                    cpu_body_data* body_cpu = &w->bodies_cpu[b];
                    real_3 r = apply_rotation(body->orientation, cm_to_joint(body, joint, a));
                    real s = (a?1:-1);
                    body->x_dot += s*deltap/body->m;
                    body->omega += s*body->invI*(cross(r, deltap) + deltaL);
                }

                // joint->deltap += deltap;
                // joint->deltaL += deltaL;

                break;
            }
            default:;
        }

        assert(child_gpu->omega.x == child_gpu->omega.x);
        assert(parent_gpu->omega.x == parent_gpu->omega.x);
    }

    stunalloc(perm);
}

void iterate_joints_positions(memory_manager* manager, world* w, bool use_integral)
{
    int* perm = (int*) stalloc(sizeof(int)*w->n_joints);
    for(int i = 0; i < w->n_joints; i++) perm[i] = i;

    for(int j = 0; j < w->n_joints; j++)
    {
        int i = rand(&w->seed, j, w->n_joints);
        swap(perm[i], perm[j]);
        body_joint* joint = &w->joints[perm[j]];

        int b0 = get_body_index(w, joint->body_id[0]);
        gpu_body_data* parent_gpu = &w->bodies_gpu[b0];
        cpu_body_data* parent_cpu = &w->bodies_cpu[b0];

        int b1 = get_body_index(w, joint->body_id[1]);
        gpu_body_data* child_gpu = &w->bodies_gpu[b1];
        cpu_body_data* child_cpu = &w->bodies_cpu[b1];

        if(!child_gpu->substantial || !parent_gpu->substantial) continue;

        assert(child_gpu->omega.x == child_gpu->omega.x);

        switch(joint->type)
        {
            case joint_ball:
            {
                real_3 u = {0,0,0};
                real mu = 0; //reduced mass
                real_3x3 reduced_I = {};
                real_3 average_joint_pos = {0,0,0};
                real_3x3 K = {};
                for(int a = 0; a <= 1; a++)
                {
                    int b = get_body_index(w, joint->body_id[a]);
                    gpu_body_data* body = &w->bodies_gpu[b];
                    cpu_body_data* body_cpu = &w->bodies_cpu[b];
                    real_3 r = apply_rotation(body->orientation, cm_to_joint(body, joint, a));
                    average_joint_pos += r+body->x;
                    mu += 1.0/body->m;
                    reduced_I += body->invI;

                    real_3x3 r_dual = {
                        0   ,+r.z,-r.y,
                        -r.z, 0  ,+r.x,
                        +r.y,-r.x, 0
                    };
                    K += real_identity_3(1.0/body->m) - r_dual*body->invI*r_dual;
                }

                real_3x3 invK = inverse(K);

                mu = 1.0/mu;
                reduced_I = inverse(reduced_I);

                average_joint_pos *= 0.5;

                { //position correction
                    real_3 u = {0,0,0};
                    for(int a = 0; a <= 1; a++)
                    {
                        int b = get_body_index(w, joint->body_id[a]);
                        gpu_body_data* body = &w->bodies_gpu[b];
                        cpu_body_data* body_cpu = &w->bodies_cpu[b];
                        real_3 r = apply_rotation(body->orientation, cm_to_joint(body, joint, a));
                        real_3 pos = r+body->x;
                        u += (a?-1:1)*pos;
                    }

                    real kp_pos = 0.2;
                    real ki_pos = 0.01;
                    real_3 pseudo_force = invK*(kp_pos*u);

                    if(use_integral)
                    {
                        pseudo_force += joint->pseudo_force;
                        joint->pseudo_force += invK*(ki_pos*u);
                    }

                    if(j == debug_menu.active_joint)
                    {
                        sprintf(debug_menu.plotname, "joint position error");
                        debug_menu.plotline[debug_menu.n_plotline_points++] = norm(u);
                    }

                    for(int a = 0; a <= 1; a++)
                    {
                        int b = get_body_index(w, joint->body_id[a]);
                        gpu_body_data* body = &w->bodies_gpu[b];
                        cpu_body_data* body_cpu = &w->bodies_cpu[b];
                        real_3 r = apply_rotation(body->orientation, cm_to_joint(body, joint, a));
                        real s = (a?1:-1);
                        body->x += s*pseudo_force/body->m;
                        real_3 pseudo_omega = body->invI*cross(r, s*pseudo_force);
                        body->orientation = axis_to_quaternion(pseudo_omega)*body->orientation;
                        update_inertia(body_cpu, body);
                    }

                    if(j == debug_menu.active_joint)
                    {
                        u = {0,0,0};
                        for(int a = 0; a <= 1; a++)
                        {
                            int b = get_body_index(w, joint->body_id[a]);
                            gpu_body_data* body = &w->bodies_gpu[b];
                            cpu_body_data* body_cpu = &w->bodies_cpu[b];
                            real_3 r = apply_rotation(body->orientation, cm_to_joint(body, joint, a));
                            real_3 pos = r+body->x;
                            u += (a?-1:1)*pos;
                        }

                        sprintf(debug_menu.plotname, "joint position error");
                        debug_menu.plotline[debug_menu.n_plotline_points++] = norm(u);
                    }

                    // joint->pseudo_force += pseudo_force;
                    // // joint->pseudo_torque += pseudo_torque;
                }

                // joint->deltap += deltap;
                // joint->deltaL += deltaL;

                break;
            }
            default:;
        }

        assert(child_gpu->omega.x == child_gpu->omega.x);
        assert(parent_gpu->omega.x == parent_gpu->omega.x);
    }

    stunalloc(perm);
}

real_3 calculate_contact_impulse(contact_point* contact, real_3 u, real_3 normal, real_3x3 invK)
{
    // real COF = 2.0/(1.0/materials_list[contact->material[0]].friction+1.0/materials_list[contact->material[1]].friction);
    real COF = 0.5*(materials_list[contact->material[0]].friction+materials_list[contact->material[1]].friction);
    real COR = 0.5*(materials_list[contact->material[0]].restitution+materials_list[contact->material[1]].restitution);
    if(COF != COF) COF = 0;
    // log_output("coefficient of friction between body indices ", contact->body_index[0], " and ", contact->body_index[1], " with materials ", contact->material[0], " and ", contact->material[1], ": ", COF, "\n");

    real u_n = dot(u, normal);

    //static friction impulse
    real_3 deltap = invK*(-COR*u_n*normal-u);

    real_3 static_impulse = deltap;

    real normal_force = dot(deltap, normal);

    real_3 u_t = u-u_n*normal;
    real_3 tangent = normalize_or_zero(u_t);

    //if static friction impulse is outside of the friction cone, compute kinetic impulse
    if(normsq(deltap) > (1+sq(COF))*sq(normal_force) || normal_force < -dot(contact->force, normal))
    {
        real_3 impulse_dir = normal;
        //only apply friction if normal force is positive,
        //techincally I think there should be negative friction but need to figure out consistent direction for that
        real effective_COR = COR;
        // if(normal_force > 0)
        // {
        //     effective_COR = 0;
        //     // impulse_dir += -COF*tangent;
        //     impulse_dir += -deltax_dot;
        // }
        // normal_force = -((1.0f+effective_COR)*u_n)/dot(K*impulse_dir, normal);
        // normal_force = max(normal_force, -body_cpu->normal_forces[i]);
        normal_force = max(normal_force, 0.0f);
        real_3 tangent_force = deltap-normal_force*normal;
        impulse_dir += COF*normalize_or_zero(tangent_force);
        deltap = normal_force*impulse_dir;
    }

    return deltap;
}

void iterate_contact_points(memory_manager* manager, world* w, render_data* rd)
{
    real target_penetration = 0.1;

    static int frame_number = 0;
    frame_number++;

    int* perm = (int*) stalloc(sizeof(int)*w->n_contacts);
    for(int i = 0; i < w->n_contacts; i++) perm[i] = i;

    for(int i = 0; i < w->n_contacts; i++)
    {
        int j = rand(&w->seed, i, w->n_contacts);
        swap(perm[i], perm[j]);
        contact_point* contact = &w->contacts[perm[i]];

        if(contact->body_index[1] == -1)
        { //collision with world
            int b = contact->body_index[0];
            cpu_body_data* body_cpu = &w->bodies_cpu[b];
            gpu_body_data* body_gpu = &w->bodies_gpu[b];

            if(!body_gpu->substantial) continue;
            if(body_cpu->phasing) continue;

            if(contact->phase == 1 || contact->phase == 2) //collision with solid or sand
            {
                assert(body_gpu->omega.x == body_gpu->omega.x);

                real_3 normal = contact->normal;
                real_3 r = apply_rotation(body_gpu->orientation, contact->pos[0]-body_gpu->x_cm);
                real_3 x = r+body_gpu->x;
                real_3 u = body_gpu->x_dot+cross(body_gpu->omega, r);

                real_3x3 r_dual = {
                    0   ,+r.z,-r.y,
                    -r.z, 0  ,+r.x,
                    +r.y,-r.x, 0
                };

                real_3x3 K = real_identity_3(1.0/body_gpu->m) - r_dual*body_gpu->invI*r_dual;
                real_3x3 invK = inverse(K);

                real_3 deltap = calculate_contact_impulse(contact, u, normal, invK);

                if(norm(deltap) > 1000)
                {
                    real_3 new_x_dot = body_gpu->x_dot + deltap;
                    real_3 new_omega = body_gpu->omega + body_gpu->m*body_gpu->invI*cross(r, deltap);

                    real_3 new_u = new_x_dot+cross(new_omega, r);

                    log_output("large impulse ", dot(u, normal), ", ", dot(new_u, normal),"\n");
                }

                contact->force += deltap;
                body_gpu->x_dot += (1.0/body_gpu->m)*deltap;
                body_gpu->omega += body_gpu->invI*cross(r, deltap);

                if(body_gpu->omega.x != body_gpu->omega.x)
                {
                    assert(0, "nan omega ", b, ", ", K);
                }
            }
            else
            {
                real_3 r = apply_rotation(body_gpu->orientation, contact->pos[0]-body_gpu->x_cm);
                real_3 x = r+body_gpu->x;
                real_3 u = body_gpu->x_dot+cross(body_gpu->omega, r);

                real_3 deltap = {0,0,0.00005};
                deltap -= 0.0001*u;
                deltap *= (1.0/N_SOLVER_ITERATIONS);
                contact->force += deltap;
                body_gpu->x_dot += (1.0/body_gpu->m)*deltap;
                body_gpu->omega += body_gpu->invI*cross(r, deltap);
            }
        }
        else
        { //collision between two bodies
            // real_3 normal = apply_rotation(w->bodies_gpu[contact->body_index[1]].orientation, contact->normal);
            real_3 normal = contact->normal;

            real_3 u = {0,0,0};
            real_3x3 K = {};
            for(int a = 0; a <= 1; a++)
            {
                int b = contact->body_index[a];
                gpu_body_data* body_gpu = &w->bodies_gpu[b];
                cpu_body_data* body_cpu = &w->bodies_cpu[b];

                if(!body_gpu->substantial) goto next_contact_jump;
                if(body_cpu->phasing) goto next_contact_jump;

                real_3 r = apply_rotation(body_gpu->orientation, contact->pos[a]-body_gpu->x_cm);
                real_3 velocity = cross(body_gpu->omega, r)+body_gpu->x_dot;
                u += (a?-1:1)*velocity;

                real_3x3 r_dual = {
                    0   ,+r.z,-r.y,
                    -r.z, 0  ,+r.x,
                    +r.y,-r.x, 0
                };
                K += real_identity_3(1.0/body_gpu->m) - r_dual*body_gpu->invI*r_dual;
            }

            real_3x3 invK = inverse(K);

            real_3 deltap = calculate_contact_impulse(contact, u, normal, invK);

            contact->force += deltap;

            for(int a = 0; a <= 1; a++)
            {
                int b = contact->body_index[a];
                gpu_body_data* body_gpu = &w->bodies_gpu[b];
                cpu_body_data* body_cpu = &w->bodies_cpu[b];

                real_3 r = apply_rotation(body_gpu->orientation, contact->pos[a]-body_gpu->x_cm);

                body_gpu->x_dot += ((a?-1:1)/body_gpu->m)*deltap;
                body_gpu->omega += (a?-1:1)*(body_gpu->invI*cross(r, deltap));

                if(body_gpu->omega.x != body_gpu->omega.x)
                {
                    assert(0, "nan omega ", b, ", ", K);
                }
            }
        }
    next_contact_jump:;
    }
    stunalloc(perm);
}

void iterate_contact_points_positions(memory_manager* manager, world* w, render_data* rd)
{
    real target_penetration = 0.1;

    static int frame_number = 0;
    frame_number++;

    int* perm = (int*) stalloc(sizeof(int)*w->n_contacts);
    for(int i = 0; i < w->n_contacts; i++) perm[i] = i;

    for(int i = 0; i < w->n_contacts; i++)
    {
        int j = rand(&w->seed, i, w->n_contacts);
        swap(perm[i], perm[j]);
        contact_point* contact = &w->contacts[perm[i]];

        if(contact->body_index[1] == -1)
        { //collision with world
            int b = contact->body_index[0];
            cpu_body_data* body_cpu = &w->bodies_cpu[b];
            gpu_body_data* body_gpu = &w->bodies_gpu[b];

            if(contact->phase == 1 || contact->phase == 2) //collision with solid or sand
            {
                if(!body_gpu->substantial) continue;
                if(body_cpu->phasing) continue;
                assert(body_gpu->omega.x == body_gpu->omega.x);

                real_3 normal = contact->normal;
                real_3 r = apply_rotation(body_gpu->orientation, contact->pos[0]-body_gpu->x_cm);
                real_3 x = r+body_gpu->x;
                real_3 u = body_gpu->x_dot+cross(body_gpu->omega, r);
                real u_n = dot(u, normal);

                real_3x3 r_dual = {
                    0   ,+r.z,-r.y,
                    -r.z, 0  ,+r.x,
                    +r.y,-r.x, 0
                };

                real_3x3 K = real_identity_3(1.0) - body_gpu->m*r_dual*body_gpu->invI*r_dual;
                real_3x3 invK = inverse(K);

                { //position correction
                    //TODO: set consistent target depths across entire contact manifold
                    real depth = dot(contact->pos[1]-x, normal)-contact->depth[1]-target_penetration;
                    // real_3 deltax = 0.05f*max(depth, 0.0f)*normal;
                    real_3 deltax = invK*(0.5f*max(depth, 0.0f)*normal);

                    body_gpu->x += deltax;
                    real_3 pseudo_omega = body_gpu->m*body_gpu->invI*cross(r, deltax);
                    body_gpu->orientation = axis_to_quaternion(pseudo_omega)*body_gpu->orientation;
                    update_inertia(body_cpu, body_gpu);
                }

                if(body_gpu->omega.x != body_gpu->omega.x)
                {
                    assert(0, "nan omega ", b, ", ", K);
                }
            }
        }
        // else
        // { //collision between two bodies
        //     gpu_body_data* body0_gpu = &w->bodies_gpu[contact->body_index[0]];
        //     gpu_body_data* body1_gpu = &w->bodies_gpu[contact->body_index[1]];

        //     // real_3 normal = apply_rotation(body1_gpu->orientation, contact->normal);
        //     real_3 normal = contact->normal;

        //     real_3 u = {0,0,0};
        //     real_3x3 K = {};
        //     for(int a = 0; a <= 1; a++)
        //     {
        //         int b = contact->body_index[a];
        //         gpu_body_data* body_gpu = &w->bodies_gpu[b];
        //         cpu_body_data* body_cpu = &w->bodies_cpu[b];

        //         if(!body_gpu->substantial) goto next_contact_jump_pos;
        //         if(body_cpu->phasing) goto next_contact_jump_pos;

        //         real_3 r = apply_rotation(body_gpu->orientation, contact->pos[a]-body_gpu->x_cm);
        //         real_3 x = body_gpu->x+r;
        //         u += (a?-1:1)*x;

        //         real_3x3 r_dual = {
        //             0   ,+r.z,-r.y,
        //             -r.z, 0  ,+r.x,
        //             +r.y,-r.x, 0
        //         };
        //         K += real_identity_3(1.0/body_gpu->m) - r_dual*body_gpu->invI*r_dual;
        //     }

        //     real_3x3 invK = inverse(K);

        //     real depth = dot(u, normal)-0*contact->depth[1]-0*target_penetration;
        //     real_3 pseudo_force = invK*(-0.5f*max(depth, 0.0f)*normal);

        //     if(normsq(pseudo_force) > sq(1)) log_output("psuedo_force: ", pseudo_force, "\n");
        //     for(int a = 0; a <= 1; a++)
        //     { //position correction
        //         int b = contact->body_index[a];
        //         gpu_body_data* body_gpu = &w->bodies_gpu[b];
        //         cpu_body_data* body_cpu = &w->bodies_cpu[b];

        //         real_3 r = apply_rotation(body_gpu->orientation, contact->pos[a]-body_gpu->x_cm);

        //         body_gpu->x += ((a?-1:1)/body_gpu->m)*pseudo_force;
        //         real_3 pseudo_omega = (a?-1:1)*body_gpu->invI*cross(r, pseudo_force);
        //         body_gpu->orientation = axis_to_quaternion(pseudo_omega)*body_gpu->orientation;
        //         update_inertia(body_cpu, body_gpu);
        //     }
        // }
    next_contact_jump_pos:;
    }
    stunalloc(perm);
}

void iterate_gravity(int iterations, gpu_body_data* bodies_gpu, int n_bodies)
{
    for(int b = 0; b < n_bodies; b++)
    {
        if(!bodies_gpu[b].substantial) continue;

        // bodies_gpu[b].x_dot *= pow(0.995f, 1.0f/iterations);
        bodies_gpu[b].omega -= bodies_gpu[b].invI*(1.0f-pow(0.995f, 1.0f/iterations))*bodies_gpu[b].omega;
        bodies_gpu[b].x_dot.z += -0.12f/iterations;
    }
}

void solve_velocity_constraints(memory_manager* manager, world* w, render_data* rd)
{
    plan_brains(w, rd);
    for(int b = 0; b < w->n_bodies; b++)
    {
        cpu_body_data* body_cpu = &w->bodies_cpu[b];
        gpu_body_data* body_gpu = &w->bodies_gpu[b];

        if(!body_gpu->substantial) continue;

        // for(int i = 0; i < 3; i++)
        // {
        //     body_cpu->normal_forces[i] = 0;
        //     body_cpu->contact_forces[i] = {0,0,0};
        // }

        body_gpu->old_x = body_gpu->x;
        body_gpu->old_orientation = body_gpu->orientation;
    }
    debug_menu.n_plotline_points = 0;
    warm_start_joints(w);
    for(int i = 0; i < N_SOLVER_ITERATIONS; i++)
    {
        iterate_gravity(N_SOLVER_ITERATIONS, w->bodies_gpu, w->n_bodies);
        iterate_brains(w, rd);
        iterate_contact_points(manager, w, rd);
        iterate_joints(manager, w, true);
    }
}

void solve_position_constraints(memory_manager* manager, world* w, render_data* rd)
{
    for(int i = 0; i < N_SOLVER_ITERATIONS; i++)
    {
        iterate_contact_points_positions(manager, w, rd);
        iterate_joints_positions(manager, w, true);
    }
}

void integrate_body_motion(cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, int n_bodies)
{
    for(int b = 0; b < n_bodies; b++)
    {
        if(!bodies_gpu[b].substantial) continue;

        bodies_gpu[b].x += bodies_gpu[b].x_dot;
        real half_angle = 0.5*norm(bodies_gpu[b].omega);
        // if(half_angle > 0.0001)
        // {
        //     real_3 L = bodies_gpu[b].I*bodies_gpu[b].omega;
        //     real E = dot(L, bodies_gpu[b].omega);

        //     real_3 axis = normalize(bodies_gpu[b].omega);
        //     quaternion rotation = (quaternion){cos(half_angle), sin(half_angle)*axis.x, sin(half_angle)*axis.y, sin(half_angle)*axis.z};
        //     bodies_gpu[b].orientation = rotation*bodies_gpu[b].orientation;
        //     update_inertia(&bodies_cpu[b], &bodies_gpu[b]);

        //     //precession
        //     bodies_gpu[b].omega = bodies_gpu[b].invI*L;

        //     bodies_gpu[b].omega = normalize(bodies_gpu[b].invI*L);

        //     // real_3 new_L = bodies_gpu[b].I*bodies_gpu[b].omega;
        //     // real new_E = dot(new_L, bodies_gpu[b].omega);
        //     // // if(b == n_bodies-1) log_output("old_E = ", old_E, ", new_E = ", new_E, "\n");
        // }

        bodies_gpu[b].orientation = normalize(bodies_gpu[b].orientation);
        if(half_angle > 0.0001)
        {
            real_3 world_L = bodies_gpu[b].I*bodies_gpu[b].omega;

            // const real max_omega = 10.0;
            // if(normsq(bodies_gpu[b].omega) > max_omega) bodies_gpu[b].omega = max_omega*normalize(bodies_gpu[b].omega);

            real_3 omega = apply_rotation(conjugate(bodies_gpu[b].orientation), bodies_gpu[b].omega);
            // real_3 L = bodies_cpu[b].I*omega;
            real_3 L = apply_rotation(conjugate(bodies_gpu[b].orientation), world_L); //should be equivalent to ^
            real E = dot(L, omega);

            quaternion total_rotation = bodies_gpu[b].orientation;

            real subdivisions = clamp(ceil(norm(omega)/0.01), 1.0f, 100.0f);

            // for(int i = 0; i < subdivisions; i++)
            // {
            //     quaternion rotation = axis_to_quaternion(omega/subdivisions);
            //     L = apply_rotation(conjugate(rotation), L);
            //     total_rotation = total_rotation*rotation;
            //     omega = bodies_cpu[b].invI*L;

            //     //rotate along energy gradient to fix energy
            //     for(int n = 0; n < 1; n++)
            //     {
            //         real_3 new_omega = bodies_cpu[b].invI*L;
            //         real new_E = dot(new_omega, L);
            //         // log_output("new E: ", new_E, "\n");

            //         // real_3 new_L = L+0.001*(E-new_E)*new_omega;
            //         // new_L = norm(L)*normalize(new_L);
            //         // real_3 axis = cross(normalize(L), normalize(new_L));
            //         // real cosine = sqrt(1.0-normsq(axis));
            //         // real sine   = sqrt(0.5*(1.0-cosine));
            //         // axis = sine*normalize(axis);
            //         // quaternion adjustment = {sqrt(1.0-normsq(axis)), axis.x, axis.y, axis.z};
            //         // // L = new_L;

            //         real_3 axis = 0.1*((E-new_E)/(new_E+E))*cross(normalize(L), normalize(new_omega));
            //         quaternion adjustment = {sqrt(1.0-normsq(axis)), axis.x, axis.y, axis.z};

            //         L = apply_rotation(adjustment, L);
            //         total_rotation = total_rotation*conjugate(adjustment);
            //     }
            // }
            // total_rotation = total_rotation*conjugate(bodies_gpu[b].orientation);

            total_rotation = axis_to_quaternion(bodies_gpu[b].omega);

            bodies_gpu[b].orientation = total_rotation*bodies_gpu[b].orientation;
            bodies_gpu[b].orientation = normalize(bodies_gpu[b].orientation);

            update_inertia(&bodies_cpu[b], &bodies_gpu[b]);
            // bodies_gpu[b].omega = bodies_gpu[b].invI*world_L;

            {
                real new_E = dot(bodies_gpu[b].omega, world_L);

                // log_output("omega's: ", bodies_gpu[b].invI*world_L, ", ", apply_rotation(bodies_gpu[b].orientation, bodies_cpu[b].invI*L), "\n");
                // log_output("L's: ", apply_rotation(conjugate(bodies_gpu[b].orientation), world_L),
                //            ", ", L, "\n");
                // log_output("real E: ", new_E, "\n");

                // draw_circle(rd, bodies_gpu[b].x, 0.2, {0,1,0,1});
                // draw_circle(rd, bodies_gpu[b].x+5*new_E*normalize(world_L), 0.2, {0,0,1,1});
                // draw_circle(rd, bodies_gpu[b].x+5*world_L, 0.4, {1,0,0,1});
                // draw_circle(rd, bodies_gpu[b].x+5*apply_rotation(bodies_gpu[b].orientation, L), 0.2, {0,1,1,1});
                // draw_circle(rd, bodies_gpu[b].x+5*bodies_gpu[b].omega, 0.2, {1,0,1,1});
                // draw_circle(rd, bodies_gpu[b].x+5*(bodies_gpu[b].invI*world_L), 0.4, {1,0,1,1});
                // draw_circle(rd, bodies_gpu[b].x+5*(apply_rotation(bodies_gpu[b].orientation, bodies_cpu[b].invI*L)), 0.2, {1,1,0,1});
            }
        }
    }
}

void simulate_body_voxels(world* w, render_data* rd)
{
    for(int b = w->n_bodies-1; b >= 0; b--)
    {
        gpu_body_data* body_gpu = w->bodies_gpu+b;
        cpu_body_data* body_cpu = w->bodies_cpu+b;
        int_3 lower = body_cpu->region.l;
        int_3 upper = body_cpu->region.u;

        genome* g = get_genome(w, body_cpu->genome_id);

        form_t* form = 0;

        if(g)
        {
            form = g->forms+body_cpu->form_id;
        }

        real_3 x_cm = {};

        body_cpu->I = {};
        body_gpu->m = 0;

        bounding_box new_bb = {upper, lower};

        int n_cells = 0;
        bool floodfill_needed = false;
        for(int z = lower.z-1; z < upper.z+1; z++)
            for(int y = lower.y-1; y < upper.y+1; y++)
                for(int x = lower.x-1; x < upper.x+1; x++)
                {
                    body_cell c = get_cell(body_cpu, {x,y,z});
                    body_cell original_cell = c;

                    uint8 form_voxel = 0;
                    if(form)
                    {
                        if(is_inside({x,y,z}, form->region))
                            form_voxel = form->materials[index_3D((int_3){x,y,z}-form->region.l, dim(form->region))];
                    }

                    uint material_id = c.material;
                    bool is_cell = material_id >= BASE_CELL_MAT;

                    //TODO: reimplement explosions

                    real_3 voxel_x = apply_rotation(body_gpu->orientation, (real_3){x,y,z}-body_gpu->x_cm)+body_gpu->x;
                    // //TODO: apply pushback force
                    for(int be = 0; be < w->n_beams; be++)
                    {
                        beam_data* beam = w->beams+be;
                        real_3 delta = voxel_x-beam->x;
                        real_3 dhat = normalize(beam->dir);
                        real d = clamp(dot(dhat, delta), 0.0, norm(beam->dir));
                        real_3 nearest_x = d*dhat+beam->x;
                        real_3 r = voxel_x-nearest_x;
                        if(normsq(r) <= sq(beam->r))
                        {
                            // c.temperature = clamp((int) c.temperature+100, 0, 255);
                            c.temperature = 255;
                        }
                    }

                    if(g)
                    {
                        if(is_cell) material_id += body_gpu->cell_material_id;
                        int cell_id =  c.material-BASE_CELL_MAT;

                        cell_type* ct = g->cell_types + cell_id;

                        bool condition = false;
                        for(int t = 0; t < ct->n_triggers; t++)
                        {
                            trigger_t* trig = ct->triggers+t;
                            switch(trig->condition)
                            {
                                case trig_always:
                                    condition = true;
                                    break;
                                case trig_hot:
                                    condition = c.temperature > 12;
                                    break;
                                case trig_cold:
                                    condition = c.temperature <= 4;
                                    break;
                                case trig_electric:
                                    condition = c.voltage > 0;
                                    break;
                                case trig_contact:
                                    // condition = trig(c) == i;
                                    //TODO
                                    break;
                                default:
                                    condition = false;
                            }

                            if(condition)
                            {
                                switch(trig->action)
                                {
                                    case act_grow: {
                                        //TODO:
                                        break;
                                    }
                                    case act_die: {
                                        c = {};
                                        break;
                                    }
                                    case act_heat: {
                                        c.temperature++;
                                        break;
                                    }
                                    case act_chill: {
                                        c.temperature--;
                                        break;
                                    }
                                    case act_electrify: {
                                        c.voltage = 3;
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
                                    default: break;
                                }
                            }
                        }

                        bool do_grow = true;
                        // if(ct->growth_time > 0)
                        // {
                        //     int grow_phase = rand(&w->seed, 0, ct->growth_time);
                        //     do_grow = (w->frame_number+grow_phase)%ct->growth_time == 0;
                        // }

                        // if(material_id != form_voxel && c.depth == 0 && do_grow)
                        {
                            c.material = form_voxel;
                        }
                    }

                    body_cell u = get_cell(body_cpu, {x,y,z+1});
                    body_cell d = get_cell(body_cpu, {x,y,z-1});
                    body_cell f = get_cell(body_cpu, {x,y+1,z});
                    body_cell b = get_cell(body_cpu, {x,y-1,z});
                    body_cell r = get_cell(body_cpu, {x+1,y,z});
                    body_cell l = get_cell(body_cpu, {x-1,y,z});

                    c.depth = MAX_DEPTH-1;
                    bool filledness = c.material != 0;
                    if(((u.material != 0) != filledness) ||
                       ((d.material != 0) != filledness) ||
                       ((r.material != 0) != filledness) ||
                       ((l.material != 0) != filledness) ||
                       ((f.material != 0) != filledness) ||
                       ((b.material != 0) != filledness)) c.depth = 0;
                    else
                    {
                        c.depth = min(c.depth, u.depth+1);
                        c.depth = min(c.depth, d.depth+1);
                        c.depth = min(c.depth, r.depth+1);
                        c.depth = min(c.depth, l.depth+1);
                        c.depth = min(c.depth, f.depth+1);
                        c.depth = min(c.depth, b.depth+1);
                    }

                    //TODO: spawn appropiate particles
                    if(c.temperature > materials_list[material_id].melting_point) c = {};
                    if(c.temperature > materials_list[material_id].boiling_point) c = {};

                    if(c.material == 0) c.temperature = ROOM_TEMP;
                    else
                    {
                        float C = materials_list[material_id].heat_capacity;
                        float Q = 0;
                        //NOTE: this would save some divisions by storing thermal resistivity,
                        //      but it's nicer to define materials in terms of conductivity so 0 has well defined behavior
                        float r_c = 1.0/materials_list[c.material].thermal_conductivity;
                        float temp = c.temperature;
                        {float k = 1.0/(r_c+1.0/materials_list[u.material].thermal_conductivity); Q += k*(float(u.temperature)-float(temp));}
                        {float k = 1.0/(r_c+1.0/materials_list[d.material].thermal_conductivity); Q += k*(float(d.temperature)-float(temp));}
                        {float k = 1.0/(r_c+1.0/materials_list[r.material].thermal_conductivity); Q += k*(float(r.temperature)-float(temp));}
                        {float k = 1.0/(r_c+1.0/materials_list[l.material].thermal_conductivity); Q += k*(float(l.temperature)-float(temp));}
                        {float k = 1.0/(r_c+1.0/materials_list[f.material].thermal_conductivity); Q += k*(float(f.temperature)-float(temp));}
                        {float k = 1.0/(r_c+1.0/materials_list[b.material].thermal_conductivity); Q += k*(float(b.temperature)-float(temp));}

                        //NOTE: there might be oscillations due to neighboring temperatures changing past each other,
                        //      but fixing would require each voxel knowing how much heat it's neighbors are getting
                        //      hopefully won't be a problem since everything will eventually converge to room temp

                        float dtemp = Q/C;
                        float dtemp_i = round(dtemp);
                        float dtemp_f = 32.0*(dtemp-dtemp_i);

                        if(dtemp_f < 0 && dtemp_f > -1.0) dtemp_f = -1.0;

                        if(rand(&w->seed,0,32) < int(abs(dtemp_f))) dtemp_i += sign(dtemp_f);
                        temp = uint(clamp(float(temp)+dtemp_i, 0.0, 255.0));

                        // if(avg_temp.x > float(temp)) temp++;
                        // else if(avg_temp.x < float(temp)) temp--;

                        c.temperature = temp;
                    }

                    if(c.material)
                    {
                        real cell_mass = materials_list[material_id].density;
                        body_gpu->m += cell_mass;

                        real_3 pos = (real_3){x+0.5,y+0.5,z+0.5};

                        x_cm += cell_mass*pos;

                        real_3 r_cm = pos - body_gpu->x_cm;

                        body_cpu->I += real_identity_3(cell_mass*(1.0f/6.0f+normsq(r_cm)));
                        for(int i = 0; i < 3; i++)
                            for(int j = 0; j < 3; j++)
                                body_cpu->I[i][j] += -cell_mass*r_cm[i]*r_cm[j];

                        expand_to(new_bb, {x,y,z});
                        n_cells++;
                    }

                    if(c.material || original_cell.material)
                    {
                        set_cell(body_cpu, {x,y,z}, c);
                        if(!c.material && original_cell.material)
                        {
                            floodfill_needed = true;
                        }
                    }
                }
        if(all_less_than(new_bb.l, new_bb.u))
            resize_body(body_cpu, new_bb);

        x_cm /= body_gpu->m;
        body_gpu->x_cm = x_cm;

        if(n_cells <= 1 || body_gpu->m <= 0)
        {
            assert(body_gpu->m <= 0.002);
            delete_body(w, body_cpu->id);
            continue;
        }

        if(body_gpu->m == 0)
        {
            log_warning("body with 0 mass detected, setting mass and inertia to defaults\n");
            body_gpu->m = 0.01;
            body_cpu->I = real_identity_3(100.0);
            body_gpu->x_cm = 0.5*real_cast(body_cpu->region.l)+0.5*real_cast(body_cpu->region.u);
        }

        update_inertia(body_cpu, body_gpu);

        if(floodfill_needed)
        {
            int_3 size = dim(body_cpu->region);
            uint8* found_cells = (uint8*) stalloc_clear((size.x*size.y*size.z+7)/8);
            int* stack = (int*) stalloc(size.x*size.y*size.z*sizeof(int));
            int n_stack = 0;
            int n_splits = 0;

            real_3 old_x = body_gpu->x;
            real_3 old_x_cm = body_gpu->x_cm;

            for(int i = 0; i < size.x*size.y*size.z; i++)
            {
                if((found_cells[i/8]>>(i%8))&1) continue;
                if(body_cpu->materials[i].material==0) continue;

                n_splits++;

                int nb = 0;
                cpu_body_data* new_body_cpu = 0;
                gpu_body_data* new_body_gpu = 0;
                if(n_splits > 1)
                {
//create a new body
                    nb = new_body_index(w);
                    new_body_cpu = &w->bodies_cpu[nb];
                    new_body_gpu = &w->bodies_gpu[nb];
                    // memcpy(new_body_cpu, body_cpu, sizeof(cpu_body_data));
                    // memcpy(new_body_gpu, body_gpu, sizeof(gpu_body_data));
                    int new_id = new_body_cpu->id;
                    *new_body_cpu = *body_cpu;
                    *new_body_gpu = *body_gpu;
                    new_body_cpu->id = new_id;
                    new_body_gpu->brain_id = 0;
                    new_body_cpu->form_id = 0;
                    new_body_cpu->genome_id = 0;
                    new_body_cpu->materials = (body_cell*) dynamic_alloc(size.x*size.y*size.z*sizeof(body_cell));
                    memset(new_body_cpu->materials, 0, size.x*size.y*size.z*sizeof(body_cell));

                    new_body_cpu->is_root = true;
                    new_body_cpu->root = new_body_cpu->id;
                }
                else
                {
                    nb = b;
                    new_body_cpu = body_cpu;
                    new_body_gpu = body_gpu;
                }

                real_3 x_cm = {};

                new_body_cpu->I = {};
                new_body_gpu->m = 0;

                //TODO: split joints correctly

                int new_n_cells = 0;
                n_stack = 0;
                stack[n_stack++] = i;
                found_cells[i/8] |= 1<<(i%8);
                while(n_stack > 0)
                {
                    int c = stack[--n_stack];
                    new_n_cells++;
                    int_3 p = {c%size.x, (c/size.x)%size.y, c/(size.x*size.y)};
                    for(int d = 0; d < 6; d++)
                    {
                        int_3 dp = (d%2?-1:1)*(int_3){d/2==0, d/2==1, d/2==2};
                        int_3 n = p+dp;
                        if(!is_inside(n, {{}, size})) continue;
                        // int j = c+dp.x+size.x*dp.y+size.x*size.y*dp.z;
                        int j = index_3D(n, size);
                        if((found_cells[j/8]>>(j%8))&1) continue;
                        found_cells[j/8] |= 1<<(j%8);
                        if(body_cpu->materials[j].material==0) continue;
                        stack[n_stack++] = j;
                    }

                    uint material_id = body_cpu->materials[c].material;
                    bool is_cell = material_id >= BASE_CELL_MAT;

                    if(g)
                    {
                        if(is_cell) material_id += body_gpu->cell_material_id;
                    }

                    real cell_mass = materials_list[material_id].density;
                    real_3 pos = (real_3){0.5,0.5,0.5}+real_cast(p+new_body_cpu->region.l);

                    new_body_gpu->m += cell_mass;

                    x_cm += cell_mass*pos;

                    real_3 r_cm = pos - new_body_gpu->x_cm;

                    new_body_cpu->I += real_identity_3(cell_mass*(1.0f/6.0f+normsq(r_cm)));
                    for(int i = 0; i < 3; i++)
                        for(int j = 0; j < 3; j++)
                            new_body_cpu->I[i][j] += -cell_mass*r_cm[i]*r_cm[j];

                    if(n_splits > 1)
                    {
                        new_body_cpu->materials[c] = body_cpu->materials[c];
                        body_cpu->materials[c] = {};
                    }
                }

                x_cm /= new_body_gpu->m;
                new_body_gpu->x_cm = x_cm;

                assert(new_n_cells != 0 && new_body_gpu->m != 0, "n_splits = ", n_splits);

                update_inertia(new_body_cpu, new_body_gpu);

                new_body_gpu->x = old_x+apply_rotation(new_body_gpu->orientation, new_body_gpu->x_cm-old_x_cm);
            }

            if(n_splits == 0) delete_body(w, body_cpu->id);
            // if(n_splits > 1) log_output("split body id ", body_cpu->id, " into ", n_splits, " parts\n");
            stunalloc(stack);
            stunalloc(found_cells);
        }
    }
}

void load_bodies_to_gpu(world* w); //TODO: make include headers or something
void find_body_contacts(memory_manager* manager, world* w);

void update_bodies(memory_manager* manager, world* w, render_data* rd)
{
    //add bodies to collision grid
    for(int z = 0; z < collision_cells_per_axis; z++)
        for(int y = 0; y < collision_cells_per_axis; y++)
            for(int x = 0; x < collision_cells_per_axis; x++)
            {
                collision_cell* cell = &w->collision_grid[index_3D({x,y,z}, collision_cells_per_axis)];
                cell->n_bodies = 0;
            }

    for(int b = 0; b < w->n_bodies; b++)
    {
        cpu_body_data* body_cpu = &w->bodies_cpu[b];
        gpu_body_data* body_gpu = &w->bodies_gpu[b];
        update_bounding_box(body_cpu, body_gpu);
        if(!body_gpu->substantial) continue;
        int_3 lower = int_cast(body_gpu->box.l/collision_cell_size);
        int_3 upper = int_cast(body_gpu->box.u/collision_cell_size);
        lower = clamp_per_axis(lower, 0, collision_cells_per_axis);
        upper = clamp_per_axis(upper, 0, collision_cells_per_axis);
        for(int z = lower.z; z <= upper.z; z++)
            for(int y = lower.y; y <= upper.y; y++)
                for(int x = lower.x; x <= upper.x; x++)
                {
                    collision_cell* cell = &w->collision_grid[index_3D({x,y,z}, collision_cells_per_axis)];
                    cell->body_indices[cell->n_bodies++] = b;
                }
    }

    load_bodies_to_gpu(w);
    find_body_contacts(manager, w);

    for(int c = 0; c < w->n_contacts; c++)
    {
        contact_point* contact = &w->contacts[c];
        int b = contact->body_index[0];
        cpu_body_data* body_cpu = &w->bodies_cpu[b];
        body_cpu->has_contact = true;
        body_cpu->phasing = false;
    }

    solve_velocity_constraints(manager, w, rd);
    integrate_body_motion(w->bodies_cpu, w->bodies_gpu, w->n_bodies);
    solve_position_constraints(manager, w, rd);

    simulate_body_voxels(w, rd);

    load_bodies_to_gpu(w);
}

#endif //GAME_COMMON
