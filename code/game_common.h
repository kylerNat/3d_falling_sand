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

struct joint
{
    int type;
    int body_id[2];
    int_3 pos[2];
    int axis[2];
};

// struct brain
// {
//     leg_data* legs;
//     arm_data* arms;
//     head_data* heads;

// };

// struct limb
// {
//     int state;

// };

enum body_part_type
{
    part_joint,
    part_brain,
    n_body_part_types,
};

struct body_part
{
    int type;
    void* data;
};

struct cpu_body_data
{
    uint16* materials;
    GLuint materials_texture;
    int storage_level;

    real_3 last_contact_point;

    int* part_ids;
    int n_parts;

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

// void simulate_special_voxels(gpu_body_data* bodies_gpu, special_voxel* special_voxels, int n_special_voxels)
// {
//     for(int v = 0; v < n_special_voxels; v++)
//     {
//         special_voxel* vox = &special_voxels[v];
//         switch(vox->type)
//         {
//             case vox_ball_joint:
//             {
//                 gpu_body_data * body = &bodies_gpu[vox->body_id];
//                 joint* j = (joint*) vox->data;
//                 special_voxel* other_vox = (vox == j->a) ? j->b : j->a;
//                 gpu_body_data* other_body = &bodies_gpu[other_vox->body_id];
//                 real_3 other_r = apply_rotation(other_body->orientation, real_cast(other_vox->pos)-other_body->x_cm);
//                 real_3 target_point = other_r+other_body->x;
//                 real_3 target_velocity = cross(other_body->omega, other_r)+other_body->x_dot;
//                 break;
//             }
//             default:;
//         }
//     }
// }

void simulate_body_parts(cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, body_part* parts, int n_parts)
{
    for(int i = 0; i < n_parts; i++)
    { //TODO: want to randomize joint update order
        body_part* part = &parts[i];
        joint* j = (joint*) part->data;
        switch(j->type)
        {
            case joint_ball:
            {
                real_3 u = {0,0,0};
                real mu = 0; //reduced mass
                for(int a = 0; a <= 1; a++)
                {
                    gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                    real_3 r = apply_rotation(body->orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                    real_3 velocity = cross(body->omega, r)+body->x_dot;
                    u += (a?-1:1)*velocity;
                    mu += 1.0/body->m;
                }
                mu = 1.0/mu;

                real_3 deltap = 0.2*u*mu;

                for(int a = 0; a <= 1; a++)
                {
                    gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                    real_3 r = apply_rotation(body->orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                    real s = (a?1:-1);
                    body->x_dot += s*deltap/body->m;
                    body->omega += body->invI*cross(r, s*deltap);
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
                    mu += 1.0/body->m;
                    reduced_I += body->invI;
                }
                mu = 1.0/mu;
                reduced_I = inverse(reduced_I);

                real_3 deltap = 0.2*u*mu;
                real_3 deltaL = reduced_I*(0.2*nu);
                //TODO: want to avoid applying torque along the axis

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

#endif //GAME_COMMON
