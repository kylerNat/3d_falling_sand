#ifndef GAME
#define GAME

#include <maths/maths.h>
#include "graphics_common.h"
#include "xaudio2_audio.h"
#include "game_common.h"
#include "memory.h"
#include "chunk.h"

real dt = 1.0/60.0; //simulate at 60 hz

define_printer(real_3 a, ("(%f, %f, %f)", a.x, a.y, a.z));

struct user_input
{
    real_2 mouse;
    real_2 dmouse;
    int16 mouse_wheel;
    byte buttons[32];
    byte prev_buttons[32];
};

#define M1 0x01
#define M2 0x02
#define M3 0x04
#define M4 0x05
#define M5 0x06
#define M_WHEEL_DOWN 0x0A
#define M_WHEEL_UP 0x0B

#define is_down(key_code, input) ((input.buttons[(key_code)/8]>>((key_code)%8))&1)
#define is_pressed(key_code, input) (((input.buttons[(key_code)/8] & ~input.prev_buttons[(key_code)/8])>>((key_code)%8))&1)

#define set_key_down(key_code, input) input.buttons[(key_code)/8] |= 1<<((key_code)%8)
#define set_key_up(key_code, input) input.buttons[(key_code)/8] &= ~(1<<((key_code)%8))

struct element_particle
{
    real_3 x;
    real_3 x_dot;
    real_3 background_x_dot;

    real_3 old_x;
    real_3 old_x_dot;
};

struct support_t
{
    real_3 x;
    real d;
};

support_t point_hull_support(real_3 dir, element_particle* p, element_particle* hull, real_3 hull_x_dot, int hull_len)
{
    element_particle* best_h = hull;
    real best_d = dot(dir, hull->x);
    for(element_particle* h = hull+1; h < hull+hull_len; h++)
    {
        real test_d = dot(dir, h->x);
        if(test_d > best_d) {
            best_h = h;
            best_d = test_d;
        }
    }

    real_3 x = best_h->x - p->x;
    real d = best_d - dot(dir, p->x);
    real_3 rel_x_dot = hull_x_dot - p->x_dot;
    real rel_x_dot_d = dot(dir, rel_x_dot);
    if(rel_x_dot_d > 0) {
        x += dt*rel_x_dot;
        d += dt*rel_x_dot_d;
    }

    return {x, d};
}

bool is_colliding(element_particle* p, element_particle* hull, real_3 hull_x_dot, int hull_len)
{
    real_3 simplex[4];
    support_t support;

    real epsilon = 0.1;
    #define do_support(i)                                       \
        support = point_hull_support(dir, p, hull, hull_x_dot, hull_len); \
        if(support.d <= epsilon) return false;                        \
        simplex[i] = support.x;

    //initialize first simplex point by searching in the direction from a random point to the origin
    real_3 dir = p->x - hull[0].x;
    do_support(0);

    real_3 v[3];

    real_3 face0;
    real_3 face1;
    real_3 face2;
    //main gjk loop
    {
    gjk_point: {
            dir = -simplex[0];
            do_support(1);
            //fall through to gjk_line
        }
    gjk_line: {
            v[0] = simplex[0]-simplex[1];
            dir = cross(v[0], cross(v[0], simplex[0]));
            do_support(2);
            //fall through to gjk_triangle
        }
    gjk_triangle: {
            v[0] = simplex[0]-simplex[2];
            v[1] = simplex[1]-simplex[2];
            face2 = cross(v[0], v[1]); //normal of the triangle
            face0 = cross(v[0], face2); //the normal of the 20 edge
            face1 = cross(face2, v[1]); //the normal of the 21 edge
            if(dot(face1, simplex[2]) < 0)
            { //outside of the 21 side
                simplex[0] = simplex[2];
                if(dot(face0, simplex[2]) < 0)
                { //in the corner region
                    goto gjk_point;
                }
                goto gjk_line;
            }
            else if(dot(face0, simplex[2]) < 0)
            { //outside of 20 side
                simplex[1] = simplex[2];
                goto gjk_line;
            }
            { //inside triangle
                if(dot(face2, simplex[2]) > 0)
                { //swap 0 and 1 to maintain CW winding order
                    dir = -face2;
                    simplex[3] = simplex[0];
                    simplex[0] = simplex[1];
                    simplex[1] = simplex[3];
                }
                else
                    dir = face2;
                do_support(3);
                //fall through to gjk_tetrahedron
            }
        }
    gjk_tetrahedron: {
            v[0] = simplex[0]-simplex[3];
            v[1] = simplex[1]-simplex[3];
            v[2] = simplex[2]-simplex[3];

            for(int i = 0; i < 3; i++)
            {
                int j = (i+1)%3;
                int k = (i+2)%3;

                face2 = cross(v[i], v[j]); //the normal of the 3ij edge

                if(dot(face2, simplex[3]) < 0)
                { //outside of face0
                    face0 = cross(v[i], face2); //the normal of the 3i edge
                    face1 = cross(face2, v[j]); //the normal of the 3j edge

                    if(dot(face1, simplex[3]) < 0)
                    { //outside of the 3j side
                        simplex[0] = simplex[j];
                        simplex[1] = simplex[3];
                        goto gjk_line;
                    }
                    else if(dot(face0, simplex[3]) < 0)
                    { //outside of 3i side
                        simplex[0] = simplex[i];
                        simplex[1] = simplex[3];
                        goto gjk_line;
                    }
                    else
                    { //inside the voronoi region of the 3ij face
                        simplex[k] = simplex[3];
                        dir = face2;
                        do_support(3);
                        goto gjk_tetrahedron;
                    }
                }
            }
            //inside tetrahedron, fall through to return true
        }
    }
    return true;
}

#define value_type void*
#define key_type int
#define hashmapname int_pointer_hashmap
#define NO_STRINGS
#define _calloc(n, type_size) permalloc_clear(manager, n*type_size)
#define extra_calloc_params memory_manager* manager,
#include <utils/hashmap.h>

struct entity
{
    real_3 x;
    real_3 x_dot;
};

struct world
{
    uint32 seed;
    entity player;

    cpu_body_data * bodies_cpu;
    gpu_body_data * bodies_gpu;

    chunk * c;
};

void set_chunk_voxel(world* w, chunk* c, real_3 pos, uint16 material)
{
    // pos = clamp_per_axis(pos, 0, chunk_size-1);

    uint16 materials_data[3] = {material, 0, 0};
    glBindTexture(GL_TEXTURE_3D, materials_textures[current_materials_texture]);
    glActiveTexture(GL_TEXTURE0 + 0);
    glTexSubImage3D(GL_TEXTURE_3D, 0,
                    pos.x, pos.y, pos.z,
                    1, 1, 1,
                    GL_RGB_INTEGER, GL_UNSIGNED_SHORT,
                    materials_data);

    glBindTexture(GL_TEXTURE_3D, active_regions_textures[current_active_regions_texture]);
    uint active_data[] = {0xFFFFFFFF};
    glTexSubImage3D(GL_TEXTURE_3D, 0,
                    pos.x/16, pos.y/16, pos.z/16,
                    1, 1, 1,
                    GL_RGB_INTEGER, GL_UNSIGNED_INT,
                    active_data);
}

void update_and_render(memory_manager* manager, world* w, render_data* rd, render_data* ui, audio_data* ad, user_input input)
{
    entity* player = &w->player;
    // player->x += player->x_dot;
    real_2 walk_inputs = (real_2){
        is_down('D', input)-is_down('A', input),
        is_down('W', input)-is_down('S', input)
    };

    // static real theta = pi/6;
    static real theta = pi/2;
    static real phi = -pi/4;
    // static real phi = 0;
    // if(is_down(M3, input))
    {
        theta += 0.8*input.dmouse.y;
        phi -= 0.8*input.dmouse.x;
    }
    // theta += 0.3*input.dmouse.y;
    // phi -= 0.3*input.dmouse.x;

    // real_3 look_forward = {-sin(phi)*sin(theta), cos(phi)*sin(theta), -cos(theta)};
    real_3 look_forward = {-sin(phi), cos(phi), 0};
    real_3 look_side    = {cos(phi), sin(phi), 0};
    #if 1
    real move_accel = 0.05;
    real move_speed = 0.5;
    real speed = norm(player->x_dot.xy);
    real z_dot = player->x_dot.z;
    real_2 move_dir = player->x_dot.xy/speed;
    real_3 walk_dir = (walk_inputs.y*look_forward + walk_inputs.x*look_side);
    if(speed == 0) move_dir = walk_dir.xy;
    if(walk_dir.x != 0 || walk_dir.y != 0)
    {
        walk_dir = normalize(walk_dir);
        real proj_mag = dot(walk_dir.xy, move_dir);
        real_2 walk_proj = move_dir*proj_mag;
        real_2 walk_rej = walk_dir.xy-walk_proj;

        real_2 accel = +move_accel*walk_rej;
        // if(proj_mag > 0)
            accel += move_accel*(move_speed-speed*sign(proj_mag))*walk_proj;

        player->x_dot.xy += accel;
    }
    else
    {
        player->x_dot *= 0.95;
    }
    #else
    real move_accel = 0.3;
    real move_speed = 1.0;
    real_3 walk_dir = (walk_inputs.y*look_forward + walk_inputs.x*look_side);
    if(walk_dir.x != 0 || walk_dir.y != 0)
    {
        walk_dir = normalize(walk_dir);
        // real_3 accel = move_accel*walk_dir;
        // real speed = norm(player->x_dot);
        real vproj = dot(player->x_dot, walk_dir); //speed along the acceleration direction
        real add_speed = move_speed-vproj;
        if(add_speed > 0)
        {
            real accel_speed = min(move_accel*move_speed, add_speed);
            player->x_dot += accel_speed*walk_dir;
        }
    }
    #endif
    // player->x_dot.x *= 0.95;
    // player->x_dot.y *= 0.95;
    player->x_dot += 2*is_pressed(M2, input)*((real_3){0,0,1}+0.0*look_forward);
    if(is_down(' ', input))
    {
        player->x_dot.x = 3*look_forward.x;
        player->x_dot.y = 3*look_forward.y;
    }

    real_3 foot_dist = (real_3){0,0,-15};

    real_3 xs[1] = {player->x+foot_dist};
    real_3 x_dots[1] = {player->x_dot};
    simulate_particles(w->c, xs, x_dots, len(xs));
    player->x = xs[0]-foot_dist;
    player->x_dot = x_dots[0];

    // player->x += player->x_dot;

    real_3 old_x = w->b->x;
    quaternion old_orientation = w->b->orientation;

    w->b->x_dot *= 0.995;
    w->b->omega *= 0.995;

    w->b->x_dot.z += -0.1;
    w->b->x = old_x+w->b->x_dot;
    w->b->orientation = old_orientation;
    real half_angle = 0.5*norm(w->b->omega);
    if(half_angle > 0.0001)
    {
        real_3 axis = normalize(w->b->omega);
        quaternion rotation = (quaternion){cos(half_angle), sin(half_angle)*axis.x, sin(half_angle)*axis.y, sin(half_angle)*axis.z};
        w->b->orientation = rotation*w->b->orientation;
    }

    // collision_point collisions[2000];
    // int n_collisions = 0;
    // real k = 1.0;

    // real I = 100;
    // real m = 1;

    // // log_output("n_collisions: ", n_collisions, "\n");

    // for(int i = 0; i < 20; i++)
    // {
    //     w->b->x = old_x+w->b->x_dot;
    //     w->b->orientation = old_orientation;
    //     real half_angle = 0.5*norm(w->b->omega);
    //     if(half_angle > 0.0001)
    //     {
    //         real_3 axis = normalize(w->b->omega);
    //         quaternion rotation = (quaternion){cos(half_angle), sin(half_angle)*axis.x, sin(half_angle)*axis.y, sin(half_angle)*axis.z};
    //         w->b->orientation = rotation*w->b->orientation;
    //     }

    //     collide_body(manager, w->c, w->b, collisions, &n_collisions, len(collisions));

    //     real_3 deltaomega = {0,0,0};
    //     real_3 deltax_dot = {0,0,0};

    //     real max_depth = 0;
    //     int deepest_c = -1;
    //     for(int c = 0; c < n_collisions; c++)
    //     {
    //         real_3 r = collisions[c].x - w->b->x;
    //         real u = dot(w->b->x_dot+cross(w->b->omega, r), collisions[c].normal);
    //         if(collisions[c].depth < max_depth && u < 0) deepest_c = c;
    //         if(u >= 0) draw_circle(rd, collisions[c].x, 0.1, {1,0,0,1});
    //         else draw_circle(rd, collisions[c].x, 0.1, {0,1,0,1});
    //     }

    //     if(deepest_c == -1) break;

    //     if(deepest_c >= 0)
    //     {
    //         int c = deepest_c;

    //         real_3 r = collisions[c].x - w->b->x;
    //         real u = dot(w->b->x_dot+cross(w->b->omega, r), collisions[c].normal);
    //         real K = 1.0+m*(normsq(r)-sq(dot(r, collisions[c].normal)))/I;
    //         // real K = 1.0+m*dot(cross(cross(r, collisions[c].normal), r), collisions[c].normal)/I;
    //         deltax_dot = (-(1.0+0.1)*u/K-0.0*collisions[c].depth)*collisions[c].normal;
    //         deltaomega = (m/I)*cross(r, deltax_dot);
    //         // draw_circle(rd, collisions[c].x, 0.1, {1,1,1,1});
    //         draw_circle(rd, collisions[c].x+20*(deltax_dot), 0.1, {0,1,0,1});
    //         if(dot(deltax_dot, collisions[c].normal) < 0)
    //         {
    //             log_output("negative normal force\n");
    //             log_output("r: ", r, "\n");
    //             log_output("normal: ", collisions[c].normal, "\n");
    //             log_output("omega: ", w->b->omega, "\n");
    //             log_output("deltax_dot: ", deltax_dot, "\n");
    //         }
    //     }

    //     w->b->x_dot += deltax_dot;
    //     w->b->omega += deltaomega;
    // }

    draw_circle(rd, w->b->x, 1, {1,1,1,1});
    draw_circle(rd, w->b->x+10.0*w->b->omega, 1, {0,0,1,1});

    // if(n_collisions > 0)
    // {
    //     w->b->x.z += 0.5;
    // }
    // else
    // {
    //     w->b->x.z -= 0.5;
    // }

    if(is_down('F', input))
    {
        real_3 r = {0,0,-27.0/2};
        real_3 deltax_dot = {-sin(theta)*sin(phi), sin(theta)*cos(phi), -cos(theta)};
        deltax_dot *= 0.1;
        real_3 deltaomega = (m/I)*cross(r, deltax_dot);
        w->b->x_dot += deltax_dot;
        w->b->omega += deltaomega;
    }

    real fov = pi*120.0/180.0;
    real screen_dist = 1.0/tan(fov/2);

    real n = 0.1;
    real f = 1000.0;

    static real camera_dist_target = 240;
    static real camera_dist = camera_dist_target;
    camera_dist_target *= pow(1.0+0.001, -input.mouse_wheel);
    const real min_camera_dist = 1;
    if(camera_dist_target < min_camera_dist) camera_dist_target = min_camera_dist;
    camera_dist += 0.2*(camera_dist_target-camera_dist);

    real_3 camera_z = {sin(theta)*sin(phi), -sin(theta)*cos(phi), cos(theta)};
    // real_3 camera_x = normalize(cross({0,0,1}, camera_z));
    real_3 camera_x = {cos(phi),sin(phi),0};
    real_3 camera_y = cross(camera_z, camera_x);
    real_3 camera_pos;

    camera_pos = player->x;

    if(is_pressed('V', input))
    {
        play_sound(ad, &waterfall, 0.1);
    }

    static real placement_dist = 20.0;
    placement_dist += 0.02*input.mouse_wheel;
    if(is_down(M1, input))
    {
        real_3 pos = player->x-placement_dist*camera_z;
        pos = clamp_per_axis(pos, 0, chunk_size-1);
        set_chunk_voxel(w, w->c, pos, 1);
    }

    if(is_down(M4, input))
    {
        real_3 pos = player->x-placement_dist*camera_z;
        pos = clamp_per_axis(pos, 0, chunk_size-1);
        set_chunk_voxel(w, w->c, pos, 2);
    }

    if(is_down(M5, input))
    {
        real_3 pos = player->x-placement_dist*camera_z;
        pos = clamp_per_axis(pos, 0, chunk_size-1);
        set_chunk_voxel(w, w->c, pos, 0);
    }

    if(is_down(M3, input))
    {
        real_3 pos = player->x-placement_dist*camera_z;
        pos = clamp_per_axis(pos, 0, chunk_size-1);
        set_chunk_voxel(w, w->c, pos, 3);
    }

    if(is_down('G', input))
    {
        real_3 pos = player->x-placement_dist*camera_z;
        w->b->x_dot += 0.1*(pos-w->b->x);
        w->b->x_dot *= 0.95;
        w->b->omega *= 0.95;
    }

    // for(int f = 0; f < AUDIO_BUFFER_MAX_LEN; f++)
    // {
    //     real omega = (4+f)*50*2*pi/ad->sample_rate;
    //     real phi = omega*ad->samples_played;
    //     complex e = {cos(phi), sin(phi)};
    //     complex de = {cos(omega), sin(omega)};
    //     for(int i = 0; i < ad->sample_rate/30; i++)
    //     {
    //         uint64 sample_number = ad->samples_played+i;
    //         int buffer_index = sample_number%ad->buffer_length;
    //         for(int wc = 0; wc < n_worker_contexts; wc++)
    //         {
    //             ad->buffer[buffer_index] += worker_context_list[wc]->audio_buffer[f]*e.x;
    //         }
    //         e *= de;
    //     }
    // }

    for(int wc = 0; wc < n_worker_contexts; wc++)
    {
        // for(int i = 0; i < AUDIO_BUFFER_MAX_LEN; i++)
        // {
        //     int buffer_index = (ad->samples_played+i)%ad->buffer_length;
        //     ad->buffer[buffer_index] += worker_context_list[wc]->audio_buffer[i];
        // }
        memset(worker_context_list[wc]->audio_buffer, 0, AUDIO_BUFFER_MAX_SIZE);
    }

    real_4x4 translate_to_player = {
        1, 0, 0, -camera_pos.x,
        0, 1, 0, -camera_pos.y,
        0, 0, 1, -camera_pos.z,
        0, 0, 0, 1,
    };

    real_4x4 rotate = {
        1, 0, 0, 0,
        0, cos(theta), sin(theta), 0,
        0, -sin(theta), cos(theta), 0,
        0, 0, 0, 1,
    };

    real_4x4 rotate2 = {
        cos(phi), sin(phi), 0, 0,
        -sin(phi), cos(phi), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };

    real_4x4 translate = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };

    // real_4x4 recenter = {
    //     1, 0, 0, 0,
    //     0, 1, 0, 0.25,
    //     0, 0, 1, 0,
    //     0, 0, 0, 1,
    // };

    real_4x4 perspective = {
        (screen_dist*window_height)/window_width, 0, 0, 0,
        0, screen_dist, 0, 0,
        0, 0, (f+n)/(f-n), (2*f*n)/(f-n),
        0, 0, -1, 0,
    };

    //yeah, the *operator is "broken" for efficiency, although I'm not even SIMDing right now which probably matters more
    rd->camera = translate_to_player*rotate2*rotate*translate*perspective;//*recenter;
    rd->camera_pos = camera_pos;
    rd->camera_axes = (real_3x3) {camera_x, camera_y, camera_z};

    // rd->spherical_functions[rd->n_spherical_functions++] = {
    //     .x = w->creatures[0].x,
    //     .coefficients = player->surface_coefficients,
    // };

    while(n_remaining_tasks) pop_work(main_context);
}

#endif //GAME
