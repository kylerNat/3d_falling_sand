#ifndef GAME_COMMON
#define GAME_COMMON

#define N_SOLVER_ITERATIONS 20

enum ui_type
{
    ui_none,
    ui_gene,
    ui_form,
    ui_form_voxel,
    ui_window,
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
    byte buttons[32];
    byte prev_buttons[32];
    ui_element active_ui_element;
};

#define M1 0x01
#define M2 0x02
#define M3 0x04
#define M4 0x05
#define M5 0x06
#define M_WHEEL_DOWN 0x0A
#define M_WHEEL_UP 0x0B

#define is_down(key_code, input) (((input)->buttons[(key_code)/8]>>((key_code)%8))&1)
#define is_pressed(key_code, input) ((((input)->buttons[(key_code)/8] & ~(input)->prev_buttons[(key_code)/8])>>((key_code)%8))&1)

#define set_key_down(key_code, input) input.buttons[(key_code)/8] |= 1<<((key_code)%8)
#define set_key_up(key_code, input) input.buttons[(key_code)/8] &= ~(1<<((key_code)%8))

enum storage_mode
{
    sm_disk,
    sm_compressed,
    sm_memory,
    sm_gpu,
};

enum special_voxel_type
{
    vox_player_brain,
    vox_brain,
    vox_neck,
    vox_heart,
    vox_hip,
    vox_knee,
    vox_foot,
    vox_shoulder,
    vox_elbow,
    vox_hand,
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

struct ray_hit
{
    bool hit;
    int_3 pos;
    int_3 dir;
    real dist;
};

ray_hit cast_ray(uint8* materials, real_3 ray_dir, real_3 ray_origin, int_3 size, int_3 origin, int max_iterations)
{
    real_3 ray_sign = sign_not_zero_per_axis(ray_dir);
    real_3 inv_ray_dir = divide_components({1,1,1}, ray_dir);
    real_3 invabs_ray_dir = divide_components(ray_sign, ray_dir);

    int_3 bounding_planes = {ray_dir.x < 0 ? size.x:0, ray_dir.y < 0 ? size.y:0, ray_dir.z < 0 ? size.z:0};
    real_3 bounding_dists = multiply_components((real_cast(bounding_planes)-ray_origin), inv_ray_dir);
    real skip_dist = max(max(max(bounding_dists.x, bounding_dists.y), bounding_dists.z), 0.0f);
    skip_dist += 0.001;
    real_3 pos = ray_origin + skip_dist*ray_dir;
    real total_dist = skip_dist;

    int_3 ipos = int_cast(pos);

    int_3 dir = {};

    int i = 0;
    for ever
    {
        if(ipos.x < 0 || ipos.y < 0 || ipos.z < 0
           || ipos.x >= size.x || ipos.y >= size.y || ipos.z >= size.z)
        {
            return {false};
        }

        if(materials[ipos.x+size.x*(ipos.y+size.y*ipos.z)] != 0)
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

#include "genetics.h"

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
#pragma pack(pop)

#pragma pack(push, 1)
struct particle_data_list
{
    int n_particles;
    particle_data particles[4096];
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

struct joint
{
    int type;
    int brain_id;
    real_3 torques;
    real max_torque;
    int body_id[2];
    int_3 pos[2];
    int axis[2];
};

struct child_joint
{
    int type;
    int child_body_id;
    int_3 pos[2];
    int axis[2];
    real_3 omega;
    real_3 torque;
    real max_speed;
    real max_torque;

    real_3 deltap;
    real_3 deltaL;
    real_3 pseudo_force;
    real_3 pseudo_torque;
};

struct cpu_body_data
{
    uint8* materials;
    int storage_level;
    int* component_ids;
    int n_components;

    bool is_root;
    int root;
    child_joint joints[N_MAX_CHILDREN];
    int n_children;

    real_3 com;
    real_3 p;
    real_3 L;
    real total_m;
    real_3x3 total_I;

    //these are in body coordinates, while the gpu versions are in world coordinates
    real_3x3 I;
    real_3x3 invI;

    real normal_forces[3];

    real_3 contact_points[3];
    real_3 contact_pos[3]; //in body space
    real_3 contact_normals[3];
    real contact_depths[3];
    int16 contact_materials[3];

    //TODO: I think I want to just create temporary joints instead, and handle this with the joint system, since it's essentially the same, need to turn off normal contact resolution near those points
    bool contact_locked[3]; //a contact point locked to the ground, used for footsteps

    real_3 contact_forces[3];
    // real_3 deltax_dot_integral[3]; //this is for doing PI constraint solving, if just P isn't enough

    int genome_id;
};

#pragma pack(push, 1)
struct gpu_body_data
{
    int_3 materials_origin;
    int_3 size;
    real_3 x_cm;
    real_3 x;
    real_3 x_dot;
    quaternion orientation;
    real_3 omega;
    real m;
    real_3x3 I;
    real_3x3 invI;

    int cell_material_id; //material_id of cell type 0 for this body
};
#pragma pack(pop)

#define MAX_COYOTE_TIME 6.0f

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

    int_3 root_anchor; //the point on the root body that this endpoints limb is connected to
    real root_dist; //the total distance from that point when fully extended
};

struct brain
{
    endpoint* endpoints;
    int n_endpoints;
    real walk_phase;
    real_3 target_accel;
    real_3 stabilize_accel;
    bool is_moving;
};

struct bounding_box
{
    int_3 l;
    int_3 u;
};

bool is_inside(int_3 p, bounding_box b)
{
    return (b.l.x <= p.x && p.x < b.u.x &&
            b.l.y <= p.y && p.y < b.u.y &&
            b.l.z <= p.z && p.z < b.u.z);
}

bool is_intersecting(bounding_box a, bounding_box b)
{
    //working with coordinates doubled to avoid rounding errors from integer division
    int_3 a_center = (a.l+a.u);
    int_3 a_diag = (a.u-a.l);
    int_3 b_center = (b.l+b.u);
    int_3 b_diag = (b.u+b.l);
    int_3 closest_a_point_to_b = a_center+multiply_components(a_diag, sign_per_axis(b_center-a_center));
    int_3 closest_b_point_to_a = b_center+multiply_components(b_diag, sign_per_axis(b_center-a_center));
    return is_inside(closest_a_point_to_b, {2*b.l, 2*b.u}) || is_inside(closest_b_point_to_a, {2*a.l, 2*a.u});
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

//figure out stuff like which feet to raise or lower
void plan_brains(cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, brain* brains, int n_brains, render_data* rd)
{
    for(int brain_id = 0; brain_id < n_brains; brain_id++)
    {
        brain* br = &brains[brain_id];
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
            gpu_body_data* body_gpu = &bodies_gpu[ep->body_id];
            cpu_body_data* body_cpu = &bodies_cpu[ep->body_id];

            gpu_body_data* root_gpu = &bodies_gpu[body_cpu->root];
            cpu_body_data* root_cpu = &bodies_cpu[body_cpu->root];

            real_3 r = apply_rotation(body_gpu->orientation, real_cast(ep->pos)+(real_3){0.5,0.5,0.5}-body_gpu->x_cm);

            real_3 anchor_x = apply_rotation(root_gpu->orientation, real_cast(ep->root_anchor)+(real_3){0.5,0.5,0.5}-root_gpu->x_cm);

            if(ep->type == endpoint_foot)
            {
                n_feet++;

                real_3 foot_x = r+body_gpu->x;
                real_3 foot_x_dot = body_gpu->x_dot+cross(body_gpu->omega, r);

                real_3 root_r = foot_x - anchor_x;

                if(ep->is_grabbing)
                {
                    n_feet_down++;

                    // real extension = norm(root_r)/ep->root_dist;
                    real extension = -dot(root_r, br->target_accel)/ep->root_dist;
                    if(extension > most_extension)
                    {
                        most_extended_foot = ep;
                        most_extension = extension;
                    }

                    if(br->is_moving)
                    {
                        for(int i = 0; i < 3; i++)
                        {
                            body_cpu->contact_locked[i] = false;
                        }

                        //make sure there's actually a contact
                        for(int i = 0;; i++)
                        {
                            if(i >= 3)
                            {
                                // ep->foot_phase += 0.1f;
                                // ep->coyote_time = 0;
                                ep->is_grabbing = false;
                                break;
                            }

                            if(body_cpu->contact_depths[i] <= 1)
                            {
                                // ep->coyote_time = MAX_COYOTE_TIME;
                                ep->foot_phase = -1.0f;
                                break;
                            }
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

                        real k = 0.005;
                        br->stabilize_accel.z += k*16;
                        br->stabilize_accel -= k*(root_gpu->x-foot_x);
                        br->stabilize_accel -= 0.01f*root_gpu->x_dot;
                    }

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
                    ep->coyote_time -= 1.0f;

                    // real forwardness = dot(root_r, br->target_accel)/ep->root_dist;
                    real forwardness = -ep->foot_phase;
                    if(forwardness > most_forwardness)
                    {
                        most_forward_foot = ep;
                        most_forwardness = forwardness;
                    }

                    if(ep->foot_phase < -0.0f)
                        for(int i = 0; i < 3; i++)
                            if(body_cpu->contact_depths[i] <= 1)
                            {
                                ep->is_grabbing = true;
                                ep->grab_target = 0;
                                ep->grab_point = body_cpu->contact_points[i];
                                // ep->grab_point = foot_x;
                                ep->grab_normal = body_cpu->contact_normals[i];

                                ep->foot_phase = -1.0f;
                                // body_cpu->contact_locked[i] = !br->is_moving;
                                break;
                            }
                }
            }

            // rd->log_pos
            //     += sprintf(rd->debug_log+rd->log_pos,
            //                "foot %i state: %s, phase = %.2f, coyote = %.2f, depth = %.2f, %s\n",
            //                ep->body_id,
            //                ep->is_grabbing ? "grab" : "free",
            //                ep->foot_phase, ep->coyote_time,
            //                body_cpu->contact_depths[0],
            //                body_cpu->contact_locked[0] ? "locked" : "unlock"
            //         );
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
                    // most_extended_foot->foot_phase = 1.0f;

                    cpu_body_data* body_cpu = &bodies_cpu[most_extended_foot->body_id];
                    for(int i = 0; i < 3; i++)
                        body_cpu->contact_locked[i] = false;
                }
            }
        }
        else
        {
            if(most_forward_foot)
            {
                // most_forward_foot->foot_phase = lerp(most_forward_foot->foot_phase, -1, 0.01);
                most_forward_foot->foot_phase -= 0.1*(n_feet-n_feet_down);
                most_forward_foot->foot_phase = clamp(most_forward_foot->foot_phase, -1.0f, 1.0f);

                gpu_body_data* body_gpu = &bodies_gpu[most_forward_foot->body_id];
                real_3 r = apply_rotation(body_gpu->orientation, real_cast(most_forward_foot->pos)+(real_3){0.5,0.5,0.5}-body_gpu->x_cm);
                real_3 foot_x = r+body_gpu->x;
                draw_circle(rd, foot_x, 0.3, {1.0,0.8,0.8,1});
            }
        }
    }
}

void iterate_brains(cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, brain* brains, int n_brains, render_data* rd)
{
    for(int brain_id = 0; brain_id < n_brains; brain_id++)
    {
        brain* br = &brains[brain_id];
        for(int e = 0; e < br->n_endpoints; e++)
        {
            endpoint* ep = &br->endpoints[e];
            gpu_body_data* body_gpu = &bodies_gpu[ep->body_id];
            cpu_body_data* body_cpu = &bodies_cpu[ep->body_id];

            gpu_body_data* root_gpu = &bodies_gpu[body_cpu->root];
            cpu_body_data* root_cpu = &bodies_cpu[body_cpu->root];

            real_3 r = apply_rotation(body_gpu->orientation, real_cast(ep->pos)+(real_3){0.5,0.5,0.5}-body_gpu->x_cm);

            real_3 anchor_x = apply_rotation(root_gpu->orientation, real_cast(ep->root_anchor)+(real_3){0.5,0.5,0.5}-root_gpu->x_cm);

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

                real_3 root_r = foot_x - anchor_x;

                real COF = 1.2; //TODO: actually get this from the collision data

                if(ep->coyote_time > 0) //the foot is on the ground
                {
                    real_3 foot_force = -1.0f*br->target_accel-br->stabilize_accel;
                    real min_normal_force = 1.0f;
                    real normal_force = max(-dot(foot_force, ep->grab_normal), -min_normal_force);
                    real_3 tangent_force = rej(foot_force, ep->grab_normal);
                    real max_tangent_force = COF*abs(normal_force);

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
                        gpu_body_data* grabbed_gpu = &bodies_gpu[ep->grab_target];
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
                    draw_circle(rd, grab_x, 0.3f, {0,1,1,1});
                }
                if(!ep->is_grabbing) //the foot is swinging forward
                {
                    real_3 forward_force = 0.05f*(br->target_accel);
                    // real_3 forward_force = 0.02f*(br->target_accel)-0.001*(foot_x_dot-root_gpu->x_dot);

                    real_3 ground_force = {}; //force to move toward/away from walls
                    real ground_distance = 0;
                    // for(int i = 0; i < 3; i++)
                    // {
                    //     ground_force += body_cpu->contact_normals[i];
                    //     ground_distance += body_cpu->contact_depths[i];
                    // }
                    ground_force = body_cpu->contact_normals[0];
                    ground_force *= 0.05f*(1.0f/3.0f)*ep->foot_phase;
                    // ground_force = -0.05f*(1.0f/3.0f)*body_cpu->contact_normals[0];
                    ground_distance *= (1.0f/3.0f);

                    // log_output("depth: ", ground_distance, "\n");

                    // real mix_factor = clamp(1.0f-0.1f*ground_distance, 0.0f, 1.0f);
                    real mix_factor = clamp(sq(ep->foot_phase) , 0.0f, 1.0f);
                    // if(body_cpu->contact_depths[0] > ep->root_dist/2) mix_factor = 0;
                    force = (1.0f/N_SOLVER_ITERATIONS)*lerp(forward_force, ground_force, mix_factor);
                    // force = (1.0f/N_SOLVER_ITERATIONS)*ground_force;

                    draw_circle(rd, foot_x, 0.3f, {mix_factor, 0.5f+0.5f*ep->foot_phase, 1.0f-mix_factor, 1});
                    // draw_circle(rd, foot_x+100*force, 0.2f, {1, 1, 1, 1});
                }
            }

            // if(ep->is_grabbing)
            {
                if(ep->grab_target) //0 means grabbed to the world, so nothing else to apply a grab_force to
                {
                    gpu_body_data* grabbed_gpu  = &bodies_gpu[ep->grab_target];
                    cpu_body_data* grabbed_cpu  = &bodies_cpu[ep->grab_target];

                    real_3 grabbed_r = apply_rotation(grabbed_gpu->orientation, ep->grab_point);

                    grabbed_gpu->x_dot += (1.0f/grabbed_gpu->m)*grab_force;
                    grabbed_gpu->omega += root_gpu->invI*cross(grabbed_r, grab_force);
                }

                root_gpu->x_dot -= (1.0f/root_gpu->m)*grab_force;
                // root_gpu->omega -= root_gpu->invI*cross(root_r, force);
            }
            // else
            {
                real_3 net_endpoint_force = force+endpoint_force;
                body_gpu->x_dot += (1.0f/body_gpu->m)*net_endpoint_force;
                body_gpu->omega += body_gpu->invI*cross(r, net_endpoint_force);

                root_gpu->x_dot -= (1.0f/root_gpu->m)*force;
                // root_gpu->omega -= root_gpu->invI*cross(root_r, force);
            }
        }
    }
}

void get_inertias(int b, int exclude, cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu)
{
        cpu_body_data* body_cpu = &bodies_cpu[b];
        gpu_body_data* body_gpu = &bodies_gpu[b];

        body_cpu->com = body_gpu->m*body_gpu->x;
        body_cpu->p = body_gpu->m*body_gpu->x_dot;

        // // don't need this for anything yet,
        // // need to supply a point around which to calculate the angular momentum
        // // could just use the com, but that makes the recursion a tiny bit harder
        // body_cpu->L = body_gpu->I*body_gpu->omega;
        // body_cpu->L += cross(body_gpu->x-base_x, body_gpu->m*body_gpu->x_dot);

        body_cpu->total_m = body_gpu->m;
        body_cpu->total_I = body_gpu->I;

        //points to add up later for the parallel axis theorem
        real_3 com_points[N_MAX_CHILDREN];
        real com_masses[N_MAX_CHILDREN];
        int n_com_points = 0;
        com_points[n_com_points] = body_gpu->x;
        com_masses[n_com_points++] = body_gpu->m;

        for(int c = 0; c < body_cpu->n_children; c++)
        {
            child_joint* joint = &body_cpu->joints[c];
            int child_id = joint->child_body_id;
            if(child_id == exclude) continue;
            cpu_body_data* child_cpu = &bodies_cpu[child_id];
            gpu_body_data* child_gpu = &bodies_gpu[child_id];

            real_3 joint_r = apply_rotation(body_gpu->orientation,
                                            real_cast(joint->pos[0])+(real_3){0.5,0.5,0.5}-body_gpu->x_cm);
            real_3 joint_x = joint_r+body_gpu->x;
            real_3 joint_x_dot = cross(body_gpu->omega, joint_r)+body_gpu->x_dot;

            get_inertias(child_id, exclude, bodies_cpu, bodies_gpu);

            body_cpu->total_m += child_cpu->total_m;
            body_cpu->total_I += child_cpu->total_I;

            body_cpu->com += child_cpu->total_m*child_cpu->com;
            body_cpu->p += child_cpu->p;
            // body_cpu->L += child_cpu->L;

            com_points[n_com_points] = child_cpu->com;
            com_masses[n_com_points++] = child_cpu->total_m;
        }

        body_cpu->com /= body_cpu->total_m;

        for(int n = 0; n < n_com_points; n++)
        {
            body_cpu->total_I += com_masses[n]*point_moment(com_points[n]-body_cpu->com);
        }
}

void shift_body_group(int b, int exclude, cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, real_3 pivot_x, real_3 deltax, real_3 deltax_dot, real_3 deltaomega)
{
        cpu_body_data* body_cpu = &bodies_cpu[b];
        gpu_body_data* body_gpu = &bodies_gpu[b];

        body_gpu->x += deltax;
        body_gpu->x_dot += deltax_dot+cross(deltaomega, body_gpu->x-pivot_x);
        body_gpu->omega += deltaomega;

        for(int c = 0; c < body_cpu->n_children; c++)
        {
            child_joint* joint = &body_cpu->joints[c];
            int child_id = joint->child_body_id;
            if(child_id == exclude) continue;

            cpu_body_data* child_cpu = &bodies_cpu[child_id];
            gpu_body_data* child_gpu = &bodies_gpu[child_id];

            shift_body_group(child_id, exclude, bodies_cpu, bodies_gpu, pivot_x,
                             deltax, deltax_dot, deltaomega);
        }
}

void recurse_joints(int root, int b, child_joint* joint, cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, real dist_to_root, int_3 root_joint_pos, bool use_integral)
{
    int body_ids[2] = {b, joint->child_body_id};

    // get_inertias(root, joint->child_body_id, bodies_cpu, bodies_gpu);
    // get_inertias(joint->child_body_id, -1, bodies_cpu, bodies_gpu);

    gpu_body_data* root_gpu = &bodies_gpu[root];
    cpu_body_data* root_cpu = &bodies_cpu[root];

    gpu_body_data* parent_gpu = &bodies_gpu[b];
    cpu_body_data* parent_cpu = &bodies_cpu[b];

    gpu_body_data* child_gpu = &bodies_gpu[joint->child_body_id];
    cpu_body_data* child_cpu = &bodies_cpu[joint->child_body_id];

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
                gpu_body_data* body = &bodies_gpu[body_ids[a]];
                real_3 r = apply_rotation(body->orientation, real_cast(joint->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
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
                K += real_identity_3(1.0)/body->m - r_dual*body->invI*r_dual;
            }

            real_3x3 invK = inverse(K);

            mu = 1.0/mu;
            reduced_I = inverse(reduced_I);

            average_joint_pos *= 0.5;

            real kp = 0.5;
            real ki = 0.05;

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
                gpu_body_data* body = &bodies_gpu[body_ids[a]];
                real_3 r = apply_rotation(body->orientation, real_cast(joint->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                real s = (a?1:-1);
                body->x_dot += s*deltap/body->m;
                body->omega += s*body->invI*(cross(r, deltap) + deltaL);
            }

            { //position correction
                real_3 u = {0,0,0};
                for(int a = 0; a <= 1; a++)
                {
                    gpu_body_data* body = &bodies_gpu[body_ids[a]];
                    quaternion orientation = axis_to_quaternion(body->omega)*body->orientation;
                    real_3 r = apply_rotation(orientation, real_cast(joint->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                    real_3 pos = r+body->x+body->x_dot;
                    u += (a?-1:1)*pos;
                }

                real_3 pseudo_force = invK*(kp*u);

                if(use_integral)
                {
                    pseudo_force += joint->pseudo_force;
                    joint->pseudo_force += invK*(ki*u);
                }

                for(int a = 0; a <= 1; a++)
                {
                    gpu_body_data* body = &bodies_gpu[body_ids[a]];
                    real_3 r = apply_rotation(body->orientation, real_cast(joint->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                    real s = (a?1:-1);
                    body->x += s*pseudo_force/body->m;
                    real_3 pseudo_omega = body->invI*cross(r, s*pseudo_force);
                    body->orientation = axis_to_quaternion(pseudo_omega)*body->orientation;
                    cpu_body_data* body_cpu = &bodies_cpu[body_ids[a]];
                    update_inertia(body_cpu, body);
                }

                // joint->pseudo_force += pseudo_force;
                // // joint->pseudo_torque += pseudo_torque;
            }

            // joint->deltap += deltap;
            // joint->deltaL += deltaL;

            break;
        }
        case joint_hinge:
        {
            real_3 u = {0,0,0};
            real_3 nu = {0,0,0};
            real theta_dot = 0;
            real mu = 0.0;
            real_3x3 reduced_I = {};
            real_3 average_axis = {0,0,0};
            real_3 average_joint_pos = {0,0,0};
            for(int a = 0; a <= 1; a++)
            {
                gpu_body_data* body = &bodies_gpu[body_ids[a]];
                real_3 r = apply_rotation(body->orientation, real_cast(joint->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                real_3 velocity = cross(body->omega, r)+body->x_dot;
                real_3 axis = {0,0,0};
                axis[joint->axis[a]] = 1.0;
                axis = apply_rotation(body->orientation, axis);
                u += (a?-1:1)*velocity;
                nu += (a?-1:1)*rej(body->omega, axis);
                theta_dot += (a?-1:1)*dot(body->omega, axis);
                average_axis += axis;
                average_joint_pos += r+body->x;
                mu += 1.0f/body->m;
                reduced_I += body->invI;
            }
            mu = 1.0/mu;
            reduced_I = inverse(reduced_I);

            average_axis = normalize(average_axis);
            average_joint_pos *= 0.5;

            real k = 0.2;

            real_3 deltap = k*u*mu;
            real_3 deltaL = reduced_I*(k*nu);

            for(int a = 0; a <= 1; a++)
            {
                gpu_body_data* body = &bodies_gpu[body_ids[a]];
                real_3 r = apply_rotation(body->orientation, real_cast(joint->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                real s = (a?1:-1);
                body->x_dot += s*deltap/body->m;
                body->omega += s*body->invI*(cross(r, deltap) + deltaL);
            }

            { //position correction
                real_3 u = {0,0,0};
                real_3 nu = {0,0,0};
                for(int a = 0; a <= 1; a++)
                {
                    gpu_body_data* body = &bodies_gpu[body_ids[a]];
                    quaternion orientation = axis_to_quaternion(body->omega)*body->orientation;
                    real_3 r = apply_rotation(orientation, real_cast(joint->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                    real_3 pos = r+body->x+body->x_dot;
                    real_3 axis = {0,0,0};
                    axis[joint->axis[a]] = 1.0;
                    axis = apply_rotation(orientation, axis);
                    u += (a?-1:1)*pos;
                    if(a == 0) nu = axis;
                    else nu = cross(axis, nu);
                }

                real_3 pseudo_force = k*u*mu;
                real_3 pseudo_torque = reduced_I*(k*nu);

                for(int a = 0; a <= 1; a++)
                {
                    gpu_body_data* body = &bodies_gpu[body_ids[a]];
                    real_3 r = apply_rotation(body->orientation, real_cast(joint->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                    real s = (a?1:-1);
                    body->x += s*pseudo_force/body->m;
                    real_3 pseudo_omega = s*(body->invI*(cross(r, pseudo_force) + pseudo_torque));
                    body->orientation = axis_to_quaternion(pseudo_omega)*body->orientation;
                    cpu_body_data* body_cpu = &bodies_cpu[body_ids[a]];
                    update_inertia(body_cpu, body);
                }

                joint->pseudo_force += pseudo_force;
                joint->pseudo_torque += pseudo_torque;
            }

            joint->deltap += deltap;
            joint->deltaL += deltaL;

            break;
        }
        default:;
    }

    for(int c = 0; c < child_cpu->n_children; c++)
    {
        child_joint* cc_joint = &child_cpu->joints[c];
        recurse_joints(root, joint->child_body_id, cc_joint, bodies_cpu, bodies_gpu,
                       dist_to_root+norm(real_cast(joint->pos[0]))+norm(real_cast(joint->pos[1])),
                       root_joint_pos, use_integral);
    }

    assert(child_gpu->omega.x == child_gpu->omega.x);
}

void recurse_warm_start_joints(int b, child_joint* joint, cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu)
{
    int body_ids[2] = {b, joint->child_body_id};

    // get_inertias(root, joint->child_body_id, bodies_cpu, bodies_gpu);
    // get_inertias(joint->child_body_id, -1, bodies_cpu, bodies_gpu);

    gpu_body_data* parent_gpu = &bodies_gpu[b];
    cpu_body_data* parent_cpu = &bodies_cpu[b];

    gpu_body_data* child_gpu = &bodies_gpu[joint->child_body_id];
    cpu_body_data* child_cpu = &bodies_cpu[joint->child_body_id];

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
        gpu_body_data* body = &bodies_gpu[body_ids[a]];
        real_3 r = apply_rotation(body->orientation, real_cast(joint->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
        real s = (a?1:-1);
        body->x_dot += s*deltap/body->m;
        body->omega += s*body->invI*(cross(r, deltap) + deltaL);

        body->x += s*pseudo_force/body->m;
        real_3 pseudo_omega = s*(body->invI*(cross(r, pseudo_force) + pseudo_torque));
        body->orientation = axis_to_quaternion(pseudo_omega)*body->orientation;
        cpu_body_data* body_cpu = &bodies_cpu[body_ids[a]];
        update_inertia(body_cpu, body);
    }

    // joint->deltap = {};
    // joint->deltaL = {};
    // joint->pseudo_force = {};
    // joint->pseudo_torque = {};

    for(int c = 0; c < child_cpu->n_children; c++)
    {
        child_joint* cc_joint = &child_cpu->joints[c];
        recurse_warm_start_joints(joint->child_body_id, cc_joint, bodies_cpu, bodies_gpu);
    }

}

void warm_start_joints(cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, int n_bodies)
{
    for(int b = 0; b < n_bodies; b++)
    {
        cpu_body_data* body_cpu = &bodies_cpu[b];
        if(!body_cpu->is_root) continue;

        gpu_body_data* body_gpu = &bodies_gpu[b];

        for(int c = 0; c < body_cpu->n_children; c++)
        {
            child_joint* joint = &body_cpu->joints[c];
            recurse_warm_start_joints(b, joint, bodies_cpu, bodies_gpu);
        }
    }
}

void iterate_joints(cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, int n_bodies, bool use_integral)
{
    for(int b = 0; b < n_bodies; b++)
    {
        cpu_body_data* body_cpu = &bodies_cpu[b];
        if(!body_cpu->is_root) continue;

        gpu_body_data* body_gpu = &bodies_gpu[b];

        for(int c = 0; c < body_cpu->n_children; c++)
        {
            child_joint* joint = &body_cpu->joints[c];
            recurse_joints(b, b, joint, bodies_cpu, bodies_gpu, 0, joint->pos[0], use_integral);
        }
    }
}

void iterate_contact_points(cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, int n_bodies, render_data* rd)
{
    real COR = 0.2;
    real COF = 10.2;

    static int frame_number = 0;
    frame_number++;

    for(int b = 0; b < n_bodies; b++)
    {
        cpu_body_data* body_cpu = &bodies_cpu[b];
        gpu_body_data* body_gpu = &bodies_gpu[b];
        assert(body_gpu->omega.x == body_gpu->omega.x);
        for(int i = 0; i < 3; i++)
        {
            // if(body_cpu->contact_materials[i] != 0)
            if(body_cpu->contact_points[i].x >= 0.1 && body_cpu->contact_depths[i] <= 1)
            {
                real_3 normal = body_cpu->contact_normals[i];
                real_3 r = apply_rotation(body_gpu->orientation, body_cpu->contact_pos[i]);
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

                if(body_cpu->contact_locked[i])
                {
                    cpu_body_data* root_cpu = &bodies_cpu[body_cpu->root];
                    gpu_body_data* root_gpu = &bodies_gpu[body_cpu->root];

                    real_3 contact_x = body_cpu->contact_points[i]+(-body_cpu->contact_depths[i]-0.5)*normal;
                    real_3 deltax = contact_x - x;
                    real_3 deltax_dot = -(invK*u);

                    real kp = 0.5;
                    real ki = 0.05;

                    //these are essentially joints, so don't apply full impulse right away
                    deltax *= kp;
                    deltax_dot *= kp;

                    // body_cpu->contact_forces[i] += body_gpu->m*deltax_dot;
                    body_cpu->contact_forces[i] += deltax_dot;

                    body_gpu->x_dot += deltax_dot;
                    body_gpu->omega += body_gpu->m*body_gpu->invI*cross(r, deltax_dot);

                    body_gpu->x += deltax;
                    real_3 pseudo_omega = body_gpu->m*body_gpu->invI*cross(r, deltax_dot);
                    body_gpu->orientation = axis_to_quaternion(pseudo_omega)*body_gpu->orientation;
                    update_inertia(body_cpu, body_gpu);

                    draw_circle(rd, contact_x, 0.3, {1,1,0,1});

                    continue;
                }

                //static friction impulse
                real_3 deltax_dot = invK*(-COR*u_n*normal-u);

                real_3 static_impulse = deltax_dot;

                real normal_force = dot(deltax_dot, normal);

                real_3 u_t = u-u_n*normal;
                real_3 tangent = normalize_or_zero(u_t);

                //if static friction impulse is outside of the friction cone, compute kinetic impulse
                if(normsq(deltax_dot) > (1+sq(COF))*sq(normal_force) || normal_force < 0)
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
                    real_3 tangent_force = deltax_dot-normal_force*normal;
                    impulse_dir += COF*normalize(tangent_force);
                    deltax_dot = normal_force*impulse_dir;
                }

                // //kinetic friction is unstable, so just make coefficient of static friction infinite
                // if(normal_force < 0)
                // {
                //     real_3 impulse_dir = normal;
                //     normal_force = 0;
                //     deltax_dot = {};
                // }

                if(norm(deltax_dot) > 1000)
                {
                    real_3 impulse_dir = normal-COF*normalize(rej(u, normal));
                    real_3 new_x_dot = body_gpu->x_dot + deltax_dot;
                    real_3 new_omega = body_gpu->omega + body_gpu->m*body_gpu->invI*cross(r, deltax_dot);

                    real_3 new_u = new_x_dot+cross(new_omega, r);

                    log_output("large impulse ", u_n, ", ", dot(new_u, normal),"\n");
                }

                body_cpu->normal_forces[i] += normal_force;

                body_gpu->x_dot += deltax_dot;
                body_gpu->omega += body_gpu->m*body_gpu->invI*cross(r, deltax_dot);

                real depth = dot(body_cpu->contact_points[i]-x, normal)-body_cpu->contact_depths[i]-0.5;
                real_3 deltax = 0.05f*max(depth, 0.0f)*normal;
                // deltax = {0,0,0};

                body_gpu->x += deltax;
                real_3 pseudo_omega = body_gpu->m*body_gpu->invI*cross(r, deltax);
                body_gpu->orientation = axis_to_quaternion(pseudo_omega)*body_gpu->orientation;
                update_inertia(body_cpu, body_gpu);

                if(body_gpu->omega.x != body_gpu->omega.x)
                {
                    assert(0, "nan omega ", b);
                }
            }
        }
    }
}

void iterate_gravity(int iterations, gpu_body_data* bodies_gpu, int n_bodies)
{
    for(int b = 0; b < n_bodies; b++)
    {
        bodies_gpu[b].x_dot *= pow(0.995f, 1.0f/iterations);
        bodies_gpu[b].omega -= bodies_gpu[b].invI*(1.0f-pow(0.995f, 1.0f/iterations))*bodies_gpu[b].omega;
        bodies_gpu[b].x_dot.z += -0.1f/iterations;
    }
}

void simulate_joints(cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, int n_bodies, brain* brains, int n_brains, render_data* rd)
{
    plan_brains(bodies_cpu, bodies_gpu, brains, n_brains, rd);
    for(int b = 0; b < n_bodies; b++)
    {
        cpu_body_data* body_cpu = &bodies_cpu[b];
        for(int i = 0; i < 3; i++)
        {
            body_cpu->normal_forces[i] = 0;
            body_cpu->contact_forces[i] = {0,0,0};
        }
    }
    warm_start_joints(bodies_cpu, bodies_gpu, n_bodies);
    for(int i = 0; i < N_SOLVER_ITERATIONS; i++)
    {
        iterate_gravity(N_SOLVER_ITERATIONS, bodies_gpu, n_bodies);
        iterate_brains(bodies_cpu, bodies_gpu, brains, n_brains, rd);
        iterate_contact_points(bodies_cpu, bodies_gpu, n_bodies, rd);
        iterate_joints(bodies_cpu, bodies_gpu, n_bodies, i < N_SOLVER_ITERATIONS-2);
    }
}

void integrate_body_motion(cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, int n_bodies)
{
    for(int b = 0; b < n_bodies; b++)
    {
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

            // if(b==14)
            // {
            //     log_output("b: ", b, "\n");
            //     log_output("E: ", E, "\n");
            // }

            for(int i = 0; i < subdivisions; i++)
            {
                quaternion rotation = axis_to_quaternion(omega/subdivisions);
                L = apply_rotation(conjugate(rotation), L);
                total_rotation = total_rotation*rotation;
                omega = bodies_cpu[b].invI*L;

                //rotate along energy gradient to fix energy
                for(int n = 0; n < 1; n++)
                {
                    real_3 new_omega = bodies_cpu[b].invI*L;
                    real new_E = dot(new_omega, L);
                    // log_output("new E: ", new_E, "\n");

                    // real_3 new_L = L+0.001*(E-new_E)*new_omega;
                    // new_L = norm(L)*normalize(new_L);
                    // real_3 axis = cross(normalize(L), normalize(new_L));
                    // real cosine = sqrt(1.0-normsq(axis));
                    // real sine   = sqrt(0.5*(1.0-cosine));
                    // axis = sine*normalize(axis);
                    // quaternion adjustment = {sqrt(1.0-normsq(axis)), axis.x, axis.y, axis.z};
                    // // L = new_L;

                    real_3 axis = 0.1*((E-new_E)/(new_E+E))*cross(normalize(L), normalize(new_omega));
                    quaternion adjustment = {sqrt(1.0-normsq(axis)), axis.x, axis.y, axis.z};

                    L = apply_rotation(adjustment, L);
                    total_rotation = total_rotation*conjugate(adjustment);
                }
            }
            total_rotation = total_rotation*conjugate(bodies_gpu[b].orientation);

            // if(b==14)
            // {
            //     real_3 new_omega = bodies_cpu[b].invI*L;
            //     real new_E = dot(new_omega, L);
            //     log_output("final E: ", new_E, "\n");
            // }

            bodies_gpu[b].orientation = total_rotation*bodies_gpu[b].orientation;
            bodies_gpu[b].orientation = normalize(bodies_gpu[b].orientation);

            update_inertia(&bodies_cpu[b], &bodies_gpu[b]);
            bodies_gpu[b].omega = bodies_gpu[b].invI*world_L;
            // bodies_gpu[b].omega = apply_rotation(bodies_gpu[b].orientation, bodies_cpu[b].invI*L);

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

#endif //GAME_COMMON
