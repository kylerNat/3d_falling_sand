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

struct cpu_body_data
{
    uint16* materials;
    GLuint materials_texture;
    int storage_level;

    real_3 last_contact_point;

    int* component_ids;
    int n_components;

    //these are in body coordinates, while the gpu versions are in world coordinates
    real_3x3 I;
    real_3x3 invI;
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

#endif //GAME_COMMON
