#ifndef GAME_COMMON
#define GAME_COMMON

struct rational
{
    int n;
    int d;
};

struct particle
{
    real_3 x;
    real_3 x_dot;
};

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
    int brain_id;
    int child_body_id;
    int_3 pos[2];
    int axis[2];
    union
    {
        struct
        {
            real theta;
            real phi;
        };
        quaternion orientation; //for ball joints only
    };
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
    uint16* materials;
    GLuint materials_texture;
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

    bool contact_locked[3]; //a contact point locked to the ground, used for footsteps

    real_3 contact_force[3];
    real_3 deltax_dot_integral[3];

    int_2 endpoints[N_MAX_ENDPOINTS];
    int endpoint_types[N_MAX_ENDPOINTS];
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
};
#pragma pack(pop)

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

struct limb
{
    int state;
};

//nodes for the body map
struct body_part
{
    int type;
    int* children;
    int* child_joints;
    int n_children;
    int_3* endpoints;
    int n_endpoints;

    real total_m;
    real_3x3 total_I;

    int body_id;
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

struct endpoint
{
    int type;
    int body_id;
    int_3 pos;
    int state;
    union
    {
        struct {
            real foot_phase_offset;
        };
    };
    int target_type;
    real_3 target_value;
};

struct brain
{
    endpoint* endpoints;
    int n_endpoints;
    real walk_phase;
    real_3 target_accel;
    // body_part* root;
};

enum body_part_type
{
    part_leg,
    n_body_part_types,
};

enum body_component_type
{
    comp_joint,
    comp_brain,
    n_body_component_types,
};

struct forque
{
    real_3 force;
    real_3 torque;
};

//convex hull of range of allowed forces
struct force_hull
{
    forque* points;
};

//things that need to be simulated
struct body_component
{
    int type;
    void* data;
};

struct collision_point
{
    real_3 x;
    real_3 normal;
    real depth;
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

void simulate_brains(cpu_body_data* cpu_bodies, gpu_body_data* gpu_bodies, brain* brains, int n_brains)
{
    for(int brain_id = 0; brain_id < n_brains; brain_id++)
    {
        brain* b = &brains[brain_id];
        for(int e = 0; e < b->n_endpoints; e++)
        {
            endpoint* ep = &b->endpoints[e];
            gpu_body_data* body = &gpu_bodies[ep->body_id];
            //TODO:
            real_3 com_x = gpu_bodies[13].x;
            real_3 com_x_dot = gpu_bodies[13].x_dot;
            // real_3 total_m = ;
            // real_3 total_I = ;

            real walk_period = 120.0;
            b->walk_phase += 1.0f/walk_period;
            b->walk_phase = fmod(b->walk_phase, 1);
            real foot_phase = fmod(b->walk_phase+ep->foot_phase_offset, 1);

            switch(ep->type)
            {
                case endpoint_foot:
                {
                    real_3 r = apply_rotation(body->orientation, real_cast(ep->pos)+(real_3){0.5,0.5,0.5}-body->x_cm);
                    real_3 foot_x = r+body->x;
                    voxel_data world_voxel = get_voxel_data(int_cast(foot_x));
                    // int foot_material = get_body_material(&cpu_bodies[ep->body_id], ep->pos);
                    real COF = 0.1;
                    real foot_swing_speed = 1;
                    real leg_length = 10; //TODO: actually calculate these based on geometry
                    if(ep->state == foot_lift)
                    {
                        //TODO: probably should change this threshold depending on creature size
                        if(world_voxel.distance >= 2 || foot_phase < 0.5)
                        {
                            ep->state = foot_swing;
                        }
                        ep->target_type = tv_velocity;
                        ep->target_value = world_voxel.normal*foot_swing_speed;
                    }
                    if(ep->state == foot_swing)
                    {
                        //swing foot toward position required for balance and target acceleration
                        real_3 target_pos = com_x + com_x_dot*0.5*walk_period - 0.5*leg_length*b->target_accel*walk_period;
                        if(normsq(foot_x-target_pos) < sq(2) || foot_phase < 0.5)
                        {
                            ep->state = foot_down;
                        }

                        ep->target_type = tv_velocity;
                        ep->target_value = foot_swing_speed*normalize_or_zero(target_pos-foot_x);
                    }
                    if(ep->state == foot_down)
                    {
                        if(world_voxel.distance <= 1)
                        {
                            ep->state = foot_push;
                        }
                        ep->target_type = tv_velocity;
                        ep->target_value = -world_voxel.normal*foot_swing_speed;
                    }
                    if(ep->state == foot_push)
                    {
                        if(foot_phase > 0.5)
                        {
                            ep->state = foot_lift;
                        }
                        if(world_voxel.distance >= 2)
                        {
                            ep->state = foot_swing;
                        }

                        ep->target_type = tv_velocity;
                        real_3 accel_T = proj(b->target_accel, world_voxel.normal);
                        real_3 accel_ll = b->target_accel-accel_T;
                        if(sq(COF)*normsq(accel_T) > normsq(accel_ll)) accel_T = norm(accel_ll)/COF*normalize_or_zero(accel_T);
                        ep->target_value = -accel_T - accel_ll;
                    }
                    if(foot_phase > 0.5)
                        ep->target_value = {0,0,1};
                    else
                        ep->target_value = {0,0,-1};

                    // ep->target_type = tv_angular_velocity;
                    // ep->target_value = {0.0,0,0};
                    break;
                }
                default:;
            }
        }
    }
}

void simulate_body_components(cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, body_component* components, int n_components, brain* brains, bool use_brains)
{
    static int loop_dir = 1;
    loop_dir *= -1;
    for(int i = loop_dir>0?0:n_components-1; 0 <= i && i < n_components; i+=loop_dir)
    { //TODO: want to randomize joint update order
        body_component* component = &components[i];
        joint* j = (joint*) component->data;
        switch(j->type)
        {
            case joint_ball:
            {
                real_3 u = {0,0,0};
                real mu = 0; //reduced mass
                real_3 average_joint_pos = {0,0,0};
                for(int a = 0; a <= 1; a++)
                {
                    gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                    real_3 r = apply_rotation(body->orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                    average_joint_pos += r+body->x;
                    real_3 velocity = cross(body->omega, r)+body->x_dot;
                    u += (a?-1:1)*velocity;
                    mu += 1.0/body->m;
                }
                mu = 1.0/mu;
                average_joint_pos *= 0.5;

                real_3 deltap = 0.2*u*mu;
                real_3 deltaL = {0,0,0};

                if(j->brain_id && use_brains)
                {
                    real_3 deltatau = {0,0,0};

                    brain* b = &brains[j->brain_id-1];
                    for(int e = 0; e < b->n_endpoints; e++)
                    {
                        endpoint* ep = &b->endpoints[e];
                        int end_body_id = ep->body_id;
                        gpu_body_data* end_body = &bodies_gpu[end_body_id];
                        real_3 R = end_body->x-average_joint_pos;

                        real_3 r = apply_rotation(end_body->orientation, real_cast(ep->pos)+(real_3){0.5,0.5,0.5}-end_body->x_cm);
                        real_3 endpoint_x_dot = end_body->x_dot+cross(end_body->omega, r);
                        real_3 endpoint_omega = end_body->omega;

                        int current_body_id = 0;
                        int search_stack[100];
                        int n_search_stack = 0;
                        search_stack[n_search_stack++] = j->body_id[0];

                        int torque_sign = 1;
                        while(n_search_stack > 0)
                        {
                            if((current_body_id=search_stack[--n_search_stack]) == ep->body_id)
                            {
                                torque_sign = -1;
                                break;
                            }
                            for(int ii = 0; ii < n_components; ii++)
                            {
                                body_component* component = &components[ii];
                                joint* jj = (joint*) component->data;
                                if(j == jj) continue;
                                for(int a = 0; a < 2; a++)
                                {
                                    if(jj->body_id[a] == current_body_id)
                                    {
                                        search_stack[n_search_stack++] = jj->body_id[1-a];
                                        break;
                                    }
                                }
                            }
                        }

                        real k = 0.0001;
                        switch(ep->target_type)
                        {
                            case tv_velocity:
                            {
                                real_3 target_deltax_dot = ep->target_value-endpoint_x_dot;

                                if(normsq(R) > 1.0) deltatau += torque_sign*(k/normsq(R))*cross(R, target_deltax_dot);
                                break;
                            }
                            case tv_angular_velocity:
                            {
                                deltatau += torque_sign*k*ep->target_value-endpoint_omega;
                                break;
                            }
                            default:;
                        }
                    }

                    //TODO: might need to actually do something like this
                    // total_reduced_m = ;
                    // total_reduced_I = ;
                    // deltatau = total_reduced_I*deltatau;

                    real_3 original_torque = j->torques;
                    j->torques += deltatau;
                    if(normsq(j->torques) > sq(j->max_torque)) j->torques = j->max_torque*normalize(j->torques);
                    deltatau = j->torques-original_torque;
                    deltaL += deltatau;
                }


                for(int a = 0; a <= 1; a++)
                {
                    gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                    real_3 r = apply_rotation(body->orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                    real s = (a?1:-1);
                    body->x_dot += s*deltap/body->m;
                    body->omega += s*body->invI*(cross(r, deltap) + deltaL);
                }

                { //position correction
                    real_3 u = {0,0,0};
                    for(int a = 0; a <= 1; a++)
                    {
                        gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                        quaternion orientation = axis_to_quaternion(body->omega)*body->orientation;
                        real_3 r = apply_rotation(orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                        real_3 pos = r+body->x+body->x_dot;
                        u += (a?-1:1)*pos;
                    }

                    real_3 pseudo_force = 0.2*u*mu;

                    for(int a = 0; a <= 1; a++)
                    {
                        gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                        real_3 r = apply_rotation(body->orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                        real s = (a?1:-1);
                        body->x += s*pseudo_force/body->m;
                        real_3 pseudo_omega = body->invI*cross(r, s*pseudo_force);
                        body->orientation = axis_to_quaternion(pseudo_omega)*body->orientation;
                        cpu_body_data* body_cpu = &bodies_cpu[j->body_id[a]];
                        update_inertia(body_cpu, body);
                    }
                }
                break;
            }

            case joint_hinge:
            {
                real_3 u = {0,0,0};
                real_3 nu = {0,0,0};
                real mu = 0.0;
                real_3x3 reduced_I = {};
                real_3 average_axis = {0,0,0};
                real_3 average_joint_pos = {0,0,0};
                for(int a = 0; a <= 1; a++)
                {
                    gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                    real_3 r = apply_rotation(body->orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                    real_3 velocity = cross(body->omega, r)+body->x_dot;
                    real_3 axis = {0,0,0};
                    axis[j->axis[a]] = 1.0;
                    axis = apply_rotation(body->orientation, axis);
                    u += (a?-1:1)*velocity;
                    nu += (a?-1:1)*rej(body->omega, axis);
                    average_axis += axis;
                    average_joint_pos += r+body->x;
                    mu += 1.0/body->m;
                    reduced_I += body->invI;
                }
                mu = 1.0/mu;
                reduced_I = inverse(reduced_I);

                //TODO: maybe this should be weighted by intertias
                average_axis = normalize(average_axis);
                average_joint_pos *= 0.5;

                real_3 deltap = 0.2*u*mu;
                real_3 deltaL = reduced_I*(0.2*nu);
                //TODO: want to avoid applying constraint torque along the axis

                if(j->brain_id && use_brains)
                {
                    real_3 deltatau = {0,0,0};

                    brain* b = &brains[j->brain_id-1];
                    for(int e = 0; e < b->n_endpoints; e++)
                    {
                        endpoint* ep = &b->endpoints[e];
                        gpu_body_data* end_body = &bodies_gpu[ep->body_id];
                        real_3 R = end_body->x-average_joint_pos;

                        real_3 r = apply_rotation(end_body->orientation, real_cast(ep->pos)+(real_3){0.5,0.5,0.5}-end_body->x_cm);
                        real_3 endpoint_x_dot = end_body->x_dot+cross(end_body->omega, r);
                        real_3 endpoint_omega = end_body->omega;

                        int current_body_id = 0;
                        int current_dist = 0;
                        int search_stack[100];
                        int search_dists[100];
                        int n_search_stack = 0;
                        search_dists[n_search_stack] = 1;
                        search_stack[n_search_stack++] = j->body_id[0];
                        int search_history[100];
                        int n_search_history = 0;

                        int torque_sign = -1;
                        while(n_search_stack > 0)
                        {
                            current_body_id = search_stack[--n_search_stack];
                            current_dist = search_dists[n_search_stack];
                            search_history[n_search_history++] = current_body_id;
                            if(current_body_id == ep->body_id)
                            {
                                torque_sign = -1;
                                break;
                            }
                            for(int ii = 0; ii < n_components; ii++)
                            {
                                body_component* component = &components[ii];
                                joint* jj = (joint*) component->data;
                                for(int a = 0; a <= 1; a++)
                                {
                                    if(jj->body_id[a] == current_body_id)
                                    {
                                        for(int b = 0;; b++)
                                        {
                                            if(b >= n_search_history)
                                            {
                                                search_dists[n_search_stack] = current_dist+1;
                                                search_stack[n_search_stack++] = jj->body_id[1-a];
                                                break;
                                            }
                                            if(jj->body_id[1-a] == search_history[b])
                                            {
                                                break;
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        }

                        switch(ep->target_type)
                        {
                            case tv_velocity:
                            {
                                real_3 target_deltax_dot = ep->target_value-endpoint_x_dot;

                                // real k = 0.002;
                                // if(normsq(R) > 1.0) deltatau += torque_sign*(k/normsq(R))*cross(R, target_deltax_dot);
                                real k = 0.0002;
                                deltatau += torque_sign*k*cross(normalize_or_zero(R), target_deltax_dot);
                                break;
                            }
                            case tv_angular_velocity:
                            {
                                real k = 0.0001;
                                deltatau += torque_sign*k*(ep->target_value-endpoint_omega);
                                break;
                            }
                            default:;
                        }
                    }

                    //TODO: might need to actually do something like this
                    // total_reduced_m = ;
                    // total_reduced_I = ;
                    // deltatau = total_reduced_I*deltatau;

                    //only use component 0 since this is a 1-dof joint
                    j->torques[0] += clamp(dot(deltatau, average_axis),
                                           -j->max_torque - j->torques[0],
                                           j->max_torque - j->torques[0]);
                    deltaL += proj(deltatau, average_axis);
                }

                for(int a = 0; a <= 1; a++)
                {
                    gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                    real_3 r = apply_rotation(body->orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                    real s = (a?1:-1);
                    body->x_dot += s*deltap/body->m;
                    body->omega += s*body->invI*(cross(r, deltap) + deltaL);
                }

                { //position correction
                    real_3 u = {0,0,0};
                    real_3 nu = {0,0,0};
                    for(int a = 0; a <= 1; a++)
                    {
                        gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                        quaternion orientation = axis_to_quaternion(body->omega)*body->orientation;
                        real_3 r = apply_rotation(orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                        real_3 pos = r+body->x+body->x_dot;
                        real_3 axis = {0,0,0};
                        axis[j->axis[a]] = 1.0;
                        axis = apply_rotation(orientation, axis);
                        u += (a?-1:1)*pos;
                        if(a == 0) nu = axis;
                        else nu = cross(axis, nu);
                    }

                    real_3 pseudo_force = 0.2*u*mu;
                    real_3 pseudo_torque = reduced_I*(0.2*nu);

                    for(int a = 0; a <= 1; a++)
                    {
                        gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                        real_3 r = apply_rotation(body->orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                        real s = (a?1:-1);
                        body->x += s*pseudo_force/body->m;
                        real_3 pseudo_omega = s*(body->invI*(cross(r, pseudo_force) + pseudo_torque));
                        body->orientation = axis_to_quaternion(pseudo_omega)*body->orientation;
                        cpu_body_data* body_cpu = &bodies_cpu[j->body_id[a]];
                        update_inertia(body_cpu, body);
                    }
                }
                break;
            }
            default:;
        }
    }
}

/*
  Control Algorithm:
  1. find center of mass
  2. recursively find range of applied torques and forces, (feet jacobians?)
  3. propagate back up to find range of external torques and forces (list of 6d vectors)
  4. find things that have desired motion (arms, eyes etc.)
  5. the external forces define a convex hull in a 6d space, find generalized barycentric coordinates and apply the external forces weighted by these coordinates, with the coordinates clamped to [0,1]
  6. this is not exact since applying torques and forces to the limbs will change the velocities. so iterate a certain number of times or until something close enough to desired motion is achieved. should probably just combine iteration with joint constraint solver

  have a gait controller to decide which limbs should be lifted and which should be seeking the ground
 */
/*
  maybe want to be agnostic to the tree, and just loop through joints?
 */

// void accumulate_masses(gpu_body_data* body_gpu, body_part* body_parts, joint* joints, body_part* part)
// {

//     part->total_m = body_gpu[part->body_id].m;
//     part->total_I = body_gpu[part->body_id].I;
//     for(int c = 0; c < part->n_children; c++)
//     {
//         int child_id = part->children[c];
//         body_part* child = &body_parts[child_id];
//         accumulate_masses(body_gpu, body_parts, joints, child);
//         part->total_m += child->total_m;
//         part->total_I += child->total_I;
//         int joint_id = part->child_joints[c];
//         joint* child_joint = &joints[joint_id];
//         int part_in_joint = -1;
//         for(int i = 0; i < 2; i++)
//             if(child_joint->body_id[i] == part->body_id) part_in_joint = i;
//         assert(part_in_joint >= 0, "joint does not connect the parent to the child");

//         real_3 r = real_cast(child_joint->pos[part_in_joint]) - body_gpu[part->body_id].x //self com to joint
//             - real_cast(child_joint->pos[1-part_in_joint]) + body_gpu[child->body_id].x; //joint to child com

//         part->total_I += real_identity_3(child->total_m*normsq(r));
//         for(int i = 0; i < 3; i++)
//             for(int j = 0; j < 3; j++)
//                 part->total_I[i][j] += -part->total_m*r[i]*r[j];

//         child->forques[child->n_forques++] = ;
//     }
// }

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

void anchor_to_root(int root, int b, cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, real dist_to_root, int_3 root_joint_pos, int_3 body_joint_pos)
{
    int body_ids[2] = {root, b};
    int_3 joint_pos[2] = {root_joint_pos, body_joint_pos};

    real_3 u = {0,0,0};
    real mu = 0; //reduced mass
    real_3x3 reduced_I = {};
    real_3 dist = {0,0,0};
    for(int a = 0; a <= 1; a++)
    {
        gpu_body_data* body = &bodies_gpu[body_ids[a]];
        real_3 r = apply_rotation(body->orientation, real_cast(joint_pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
        real s = (a?-1:1);
        dist += s*(r+body->x);
        real_3 velocity = cross(body->omega, r)+body->x_dot;
        u += s*velocity;
        mu += 1.0/body->m;
        reduced_I += body->invI;
    }

    if(normsq(dist) <= sq(dist_to_root)) return;

    real_3 dir = normalize(dist);

    mu = 1.0/mu;
    reduced_I = inverse(reduced_I);

    real kp = 0.2;

    real_3 deltap = kp*u*mu;
    deltap = proj(deltap, dist);

    if(dot(deltap, dist) < 0)
        for(int a = 0; a <= 1; a++)
        {
            gpu_body_data* body = &bodies_gpu[body_ids[a]];
            real_3 r = apply_rotation(body->orientation, real_cast(joint_pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
            real s = (a?1:-1);
            body->x_dot += s*deltap/body->m;
            body->omega += s*body->invI*(cross(r, deltap));
        }

    { //position correction
        real_3 u = {0,0,0};
        for(int a = 0; a <= 1; a++)
        {
            gpu_body_data* body = &bodies_gpu[body_ids[a]];
            quaternion orientation = axis_to_quaternion(body->omega)*body->orientation;
            real_3 r = apply_rotation(orientation, real_cast(joint_pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
            real_3 pos = r+body->x+body->x_dot;
            u += (a?-1:1)*pos;
        }

        if(normsq(u) < sq(dist_to_root)) return;
        u = u-dist_to_root*normalize(u);

        real_3 pseudo_force = kp*u*mu;

        for(int a = 0; a <= 1; a++)
        {
            gpu_body_data* body = &bodies_gpu[body_ids[a]];
            real_3 r = apply_rotation(body->orientation, real_cast(joint_pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
            real s = (a?1:-1);
            body->x += s*pseudo_force/body->m;
            real_3 pseudo_omega = body->invI*cross(r, s*pseudo_force);
            body->orientation = axis_to_quaternion(pseudo_omega)*body->orientation;
            cpu_body_data* body_cpu = &bodies_cpu[body_ids[a]];
            update_inertia(body_cpu, body);
        }
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

    // if(b != root) anchor_to_root(root, joint->child_body_id, bodies_cpu, bodies_gpu, dist_to_root+norm(real_cast(joint->pos[0])), root_joint_pos, joint->pos[1]);

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

            // if(joint->brain_id != 0)
            // {
            //     real_3 target_x = {256+128, 256+128, 50+16};
            //     real_3 r = root_gpu->x-average_joint_pos;
            //     real_3 target_omega = cross(r, target_x-root_gpu->x)/normsq(r);

            //     // real_3 target_omega = 2.0*cross(apply_rotation(child_gpu->orientation, (real_3){1,0,0}), (real_3){0,0,-1});
            //     deltaL += joint->max_torque*reduced_I*(target_omega-child_gpu->omega);
            //     joint->torques[0] =

            //     // real max_torque = 0.05;
            //     // if(normsq(deltaL) > sq(max_torque)) deltaL = max_torque*normalize_or_zero(deltaL);

            //     // if(!(norm(deltaL) < 0.1))
            //     // {
            //     //     log_output("wut\n ", target_omega, ", ", parent_gpu->omega, ", ", child_gpu->omega, ",", deltaL, "\n", reduced_I, "\n", parent_gpu->invI, "\n", child_gpu->invI, "\n", parent_gpu->invI+child_gpu->invI,"\n");
            //     // }

            //     // deltaL += reduced_I*0.1*joint->max_torque
            //     //     *cross(apply_rotation(child_gpu->orientation, (real_3){0,0,1}), (real_3){0,0,1});
            //     // deltaL += -0.01*reduced_I*child_gpu->omega;
            // }

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

            if(joint->brain_id != 0)
            {
                real_3 target_omega = 1.0*dot(apply_rotation(child_gpu->orientation, (real_3){1,0,0}), cross((real_3){0,0,-1}, average_axis))*average_axis;
                real_3 torque = joint->max_torque*reduced_I*(target_omega-child_gpu->omega);
                // deltaL += reduced_I*0.003*joint->max_torque*average_axis;
                // deltaL += -0.01*reduced_I*dot(child_gpu->omega, average_axis)*average_axis;

                // real max_torque = 0.005;
                // if(normsq(torque) > sq(max_torque)) torque = max_torque*normalize(deltaL);
                deltaL += torque;
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

void iterate_contact_points(cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, int n_bodies)
{
    real COR = 0.2;
    real COF = 1.2;

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
            if(body_cpu->contact_points[i].x >= 0.1)
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

                    real_3 contact_x = body_cpu->contact_points[i]+(body_cpu->contact_depths[i]-0.5)*normal;
                    real_3 deltax = contact_x - x;
                    real_3 deltax_dot = -(invK*u);

                    real kp = 0.5;
                    real ki = 0.05;

                    //these are essentially joints, so don't apply full impulse right away
                    deltax *= kp;
                    deltax_dot *= kp;

                    // deltax_dot += body_cpu->deltax_dot_integral[i];
                    // body_cpu->deltax_dot_integral[i] += -ki*invK*u;

                    body_gpu->x_dot += deltax_dot;
                    body_gpu->omega += body_gpu->m*body_gpu->invI*cross(r, deltax_dot);

                    body_cpu->contact_force[i] += body_gpu->m*deltax_dot;

                    real limb_kp = 0.5;

                    real_3 target_x = {256+128, 256+128, 50+26};
                    real k_sp = 0.01;
                    real_3 target_contact_force = k_sp*(target_x-root_gpu->x);
                    if(normsq(target_contact_force) > sq(0.5*root_gpu->m))
                        target_contact_force = (0.5*root_gpu->m)*normalize(target_contact_force);

                    real_3 limb_force = limb_kp*(target_contact_force-body_cpu->contact_force[i]);

                    real_3 root_r = contact_x-root_gpu->x;

                    limb_force += -0.01*cross(root_r, body_gpu->omega)/normsq(root_r);
                    // limb_force += 0.01*(root_r-apply_rotation(body_gpu->orientation, root_r))/normsq(root_r);

                    root_gpu->x_dot += limb_force/root_gpu->m;
                    root_gpu->omega += root_gpu->invI*cross(root_r, limb_force);
                    body_cpu->contact_force[i] += limb_force;
                    // root_gpu->omega += -0.05*proj(root_gpu->omega, root_r);
                    root_gpu->omega += -0.05*root_gpu->omega;

                    body_gpu->x += deltax;
                    real_3 pseudo_omega = body_gpu->m*body_gpu->invI*cross(r, deltax_dot);
                    body_gpu->orientation = axis_to_quaternion(pseudo_omega)*body_gpu->orientation;
                    update_inertia(body_cpu, body_gpu);
                    continue;
                }

                //static friction impulse
                real_3 deltax_dot = invK*(-COR*u_n*normal-u);

                real_3 static_impulse = deltax_dot;

                real normal_force = dot(deltax_dot, normal);

                //if static friction impulse is outside of the friction cone, compute kinetic impulse
                if(normsq(deltax_dot) > (1.0+sq(COF))*sq(normal_force) || normal_force < 0)
                {
                    real_3 impulse_dir = normal;
                    //only apply friction if normal force is positive,
                    //techincally I think there should be negative friction but need to figure out consistent direction for that
                    real effective_COR = COR;
                    if(normal_force > 0)
                    {
                        effective_COR = 0;
                        impulse_dir += -COF*normalize(rej(u, normal));
                    }
                    normal_force = -((1.0f+effective_COR)*u_n)/dot(K*impulse_dir, normal);
                    normal_force = max(normal_force, -body_cpu->normal_forces[i]);
                    // normal_force = max(normal_force, 0.0f);
                    deltax_dot = normal_force*impulse_dir;
                }

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

                real depth = dot(body_cpu->contact_points[i]-x, normal)+body_cpu->contact_depths[i]-0.5;
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

void simulate_joints(cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, int n_bodies)
{
    for(int b = 0; b < n_bodies; b++)
    {
        cpu_body_data* body_cpu = &bodies_cpu[b];
        for(int i = 0; i < 3; i++)
        {
            body_cpu->normal_forces[i] = 0;
        }
    }
    warm_start_joints(bodies_cpu, bodies_gpu, n_bodies);
    int n_iterations = 40;
    for(int i = 0; i < n_iterations; i++)
    {
        iterate_gravity(n_iterations, bodies_gpu, n_bodies);
        iterate_contact_points(bodies_cpu, bodies_gpu, n_bodies);
        iterate_joints(bodies_cpu, bodies_gpu, n_bodies, i < n_iterations-2);
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
