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

    int* part_ids;
    int n_parts;
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

void simulate_body_parts(gpu_body_data* bodies_gpu, body_part* parts, int n_parts)
{
    for(int i = 0; i < n_parts; i++)
    { //TODO: want to randomize joint update order
        body_part* part = &parts[i];
        joint* j = (joint*) part->data;
        switch(j->type)
        {
            case joint_ball:
            {
                real I = 10;
                real m = 1;

                real_3 u = {0,0,0};
                for(int a = 0; a <= 1; a++)
                {
                    gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                    real_3 r = apply_rotation(body->orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                    real_3 velocity = cross(body->omega, r)+body->x_dot;
                    u += (a?-1:1)*velocity;
                }

                real_3 deltax_dot = 0.1*u;

                for(int a = 0; a <= 1; a++)
                {
                    gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                    real_3 r = apply_rotation(body->orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                    real s = (a?1:-1);
                    body->x_dot += s*deltax_dot;
                    body->omega += (m/I)*cross(r, s*deltax_dot);
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

                    real_3 deltax = 0.1*u;

                    for(int a = 0; a <= 1; a++)
                    {
                        gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                        real_3 r = apply_rotation(body->orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                        real s = (a?1:-1);
                        body->x += s*deltax;
                        real_3 pseudo_omega = (m/I)*cross(r, s*deltax);
                        body->orientation = axis_to_quaternion(pseudo_omega)*body->orientation;
                    }
                }


                // //TODO: apply equal forces to both bodies
                // real I = 10;
                // real m = 1;

                // real_3x3 jacobian = real_identity_3(2.0);
                // real_3 u = {0,0,0};
                // for(int a = 0; a <= 1; a++)
                // {
                //     gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                //     real_3 r = apply_rotation(body->orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                //     real_3 velocity = cross(body->omega, r)+body->x_dot;
                //     real_3x3 inverse_inertia = real_identity_3(1.0/I);
                //     u += (a?-1:1)*velocity;
                //     for(int n = 0; n < 3; n++)
                //         for(int l = 0; l < 3; l++)
                //             for(int s = 1; s <= 2; s++)
                //                 for(int t = 1; t <= 2; t++)
                //                     jacobian[n][l] +=
                //                         (t%2?1:-1)*(s%2?1:-1) //levi-cevita sign stuff
                //                         * m*inverse_inertia[(n+s)%3][(l+t)%3]
                //                         * r[(l+2*t)%3]*r[(n+2*s)%3];
                // }

                // real_3 deltax_dot = inverse(jacobian)*u;

                // for(int a = 0; a <= 1; a++)
                // {
                //     gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                //     real_3 r = apply_rotation(body->orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                //     real s = (a?1:-1);
                //     body->x_dot += s*deltax_dot;
                //     body->omega += (m/I)*cross(r, s*deltax_dot);
                // }

                // // real_3 u_final = {0,0,0};
                // // for(int a = 0; a <= 1; a++)
                // // {
                // //     gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                // //     real_3 r = apply_rotation(body->orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                // //     real_3 velocity = cross(body->omega, r)+body->x_dot;
                // //     real_3x3 inverse_inertia = real_identity_3(1.0/I);
                // //     u_final += (a?-1:1)*velocity;
                // // }
                // // log_output(u, " : ", u_final, ", ", u-jacobian*deltax_dot, "\n");

                // { //position correction
                //     real_3x3 jacobian = real_identity_3(2.0);
                //     real_3 u = {0,0,0};
                //     for(int a = 0; a <= 1; a++)
                //     {
                //         gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                //         quaternion orientation = axis_to_quaternion(body->omega)*body->orientation;
                //         real_3 r = apply_rotation(orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                //         real_3 pos = r+body->x+body->x_dot;
                //         real_3x3 inverse_inertia = real_identity_3(1.0/I);
                //         u += (a?-1:1)*pos;
                //         for(int n = 0; n < 3; n++)
                //             for(int l = 0; l < 3; l++)
                //                 for(int s = 1; s <= 2; s++)
                //                     for(int t = 1; t <= 2; t++)
                //                         jacobian[n][l] +=
                //                             (t%2?1:-1)*(s%2?1:-1) //levi-cevita sign stuff
                //                             * m*inverse_inertia[(n+s)%3][(l+t)%3]
                //                             * r[(l+2*t)%3]*r[(n+2*s)%3];
                //     }

                //     real_3 deltax_dot = inverse(jacobian)*u;

                //     for(int a = 0; a <= 1; a++)
                //     {
                //         gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                //         real_3 r = apply_rotation(body->orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                //         real s = (a?1:-1);
                //         body->x += s*deltax_dot;
                //         real_3 pseudo_omega = (m/I)*cross(r, s*deltax_dot);
                //         body->orientation = axis_to_quaternion(pseudo_omega)*body->orientation;
                //     }
                // }

                break;
            }

            case joint_hinge:
            {
                real I = 10;
                real m = 1;

                real_3 u = {0,0,0};
                real_3 nu = {0,0,0};
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
                }

                real_3 deltax_dot = 0.1*u;
                real_3 deltaL = 0.1*nu*I;
                //TODO: figure out how to scale by I for the full matrix
                //TODO: want to make sure to apply no torque along the axis

                for(int a = 0; a <= 1; a++)
                {
                    gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                    real_3 r = apply_rotation(body->orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                    real s = (a?1:-1);
                    body->x_dot += s*deltax_dot;
                    body->omega += (m/I)*cross(r, s*deltax_dot) + s*deltaL/I;
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

                    real_3 deltax = 0.1*u;
                    real_3 deltaorientation = 0.1*nu*I;

                    for(int a = 0; a <= 1; a++)
                    {
                        gpu_body_data* body = &bodies_gpu[j->body_id[a]];
                        real_3 r = apply_rotation(body->orientation, real_cast(j->pos[a])+(real_3){0.5,0.5,0.5}-body->x_cm);
                        real s = (a?1:-1);
                        body->x_dot += s*deltax;
                        real_3 pseudo_omega = (m/I)*cross(r, s*deltax) + s*deltaorientation/I;
                        body->orientation = axis_to_quaternion(pseudo_omega)*body->orientation;
                    }
                }
            }
            default:;
        }
    }
}

#endif //GAME_COMMON
