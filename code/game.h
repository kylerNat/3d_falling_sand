#ifndef GAME
#define GAME

#include <maths/maths.h>
#include "graphics_common.h"
#include "xaudio2_audio.h"
#include "game_common.h"
#include "memory.h"
#include "chunk.h"

real dt = 1.0/60.0; //simulate at 60 hz

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

    body * b;

    chunk* c;
};

void update_and_render(memory_manager* manager, world* w, render_data* rd, render_data* ui, audio_data* ad, user_input input)
{
    entity* player = &w->player;
    // player->x += player->x_dot;
    real_2 walk_inputs = (real_2){
        is_down('D', input)-is_down('A', input),
        is_down('W', input)-is_down('S', input)
    };

    // static real theta = pi/6;
    static real theta = atan(sqrt(0.5)); //isometric
    static real phi = pi/4;
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
    real move_speed = 1.0;
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

    real_3 foot_dist = (real_3){0,0,-12};

    // particle particles[1] = {{player->, x_dotsx+foot_dist, player->x_dot}};
    // simulate_particles(w->c, particles, len(xs));
    // player->x = particles[0].x-foot_dist;
    // player->x_dot = particles[0].x_dot;

    real_3 xs[1] = {player->x+foot_dist};
    real_3 x_dots[1] = {player->x_dot};
    simulate_particles(w->c, xs, x_dots, len(xs));
    player->x = xs[0]-foot_dist;
    player->x_dot = x_dots[0];

    real half_angle = 0.5*norm(w->b->omega);
    real_3 axis = normalize(w->b->omega);
    quaternion rotation = (quaternion){cos(half_angle), sin(half_angle)*axis.x, sin(half_angle)*axis.y, sin(half_angle)*axis.z};
    w->b->orientation = w->b->orientation*rotation;

    w->b->x += w->b->x_dot;

    w->b->x_dot *= 0.95;
    w->b->omega *= 0.95;

    // collision_point collisions[1000];
    // int n_collisions = 0;
    // collide_body(manager, w->c, w->b, collisions, &n_collisions, len(collisions));
    // real_3 deltaomega = {0,0,0};
    // real_3 deltax_dot = {0,0,0};
    // real k = 1;

    // real I = 100;
    // real m = 1;

    // // log_output("n_collisions: ", n_collisions, "\n");

    // real max_depth = 0;
    // for(int i = 0; i < n_collisions; i++)
    // {
    //     real_3 force = k*pow(abs(collisions[i].depth), 3)*collisions[i].normal;
    //     real_3 torque = cross(collisions[i].x - w->b->x, force);
    //     deltax_dot += force/m;
    //     deltaomega += torque/I;
    //     max_depth = max(max_depth, -collisions[i].depth);
    // }

    // w->b->x.z += 0.1*max_depth;

    // real E = 0.5*m*normsq(w->b->x_dot)+0.5*I*normsq(w->b->omega);
    // //rescale force and torque to conserve energy
    // real force_denom = normsq(deltax_dot)+normsq(deltaomega);
    // real force_scale = 0;
    // if(force_denom > 0) force_scale = -2.0*(dot(w->b->x_dot, deltax_dot)+dot(w->b->omega, deltaomega))/force_denom;
    // deltax_dot *= force_scale;
    // deltaomega *= force_scale;

    // w->b->x_dot += deltax_dot + (real_3){0,0,-0.08};
    // w->b->omega += deltaomega;

    // if(n_collisions > 0)
    // {
    //     w->b->x.z += 0.5;
    // }
    // else
    // {
    //     w->b->x.z -= 0.5;
    // }

    // uint16 material_data[10] = {};
    // real_3 feet = player->x+(real_3){0,0,-20};
    // feet = clamp_components(feet, 0, chunk_size-1);
    // // get_chunk_data(int_cast(feet), {1,1,1}, material_data);

    // player->x_dot.z -= 20.0;
    // if(feet.z <= 0.0 or material_data[0] != 0)
    // {
    //     player->x_dot.z = 0;
    //     player->x.z = ceil(player->x.z)+0.001;
    // }

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
        pos = clamp_components(pos, 0, chunk_size-1);

        uint16 materials_data[1] = {1};
        glBindTexture(GL_TEXTURE_3D, w->c->materials_texture);
        glActiveTexture(GL_TEXTURE0 + 0);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        pos.x, pos.y, pos.z,
                        1, 1, 1,
                        GL_RED_INTEGER, GL_UNSIGNED_SHORT,
                        materials_data);
    }

    if(is_down(M4, input))
    {
        real_3 pos = player->x-placement_dist*camera_z;
        pos = clamp_components(pos, 0, chunk_size-1);

        uint16 materials_data[3] = {2, 0, 0};
        glBindTexture(GL_TEXTURE_3D, w->c->materials_texture);
        glActiveTexture(GL_TEXTURE0 + 0);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        pos.x, pos.y, pos.z,
                        1, 1, 1,
                        GL_RGB_INTEGER, GL_UNSIGNED_SHORT,
                        materials_data);
    }

    if(is_down(M5, input))
    {
        real_3 pos = player->x-placement_dist*camera_z;
        pos = clamp_components(pos, 0, chunk_size-1);

        uint16 materials_data[1] = {0};
        glBindTexture(GL_TEXTURE_3D, w->c->materials_texture);
        glActiveTexture(GL_TEXTURE0 + 0);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        pos.x, pos.y, pos.z,
                        1, 1, 1,
                        GL_RED_INTEGER, GL_UNSIGNED_SHORT,
                        materials_data);
    }

    if(is_down(M3, input))
    {
        real_3 pos = player->x-placement_dist*camera_z;
        pos = clamp_components(pos, 0, chunk_size-1);

        uint16 materials_data[1] = {3};
        glBindTexture(GL_TEXTURE_3D, w->c->materials_texture);
        glActiveTexture(GL_TEXTURE0 + 0);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        pos.x, pos.y, pos.z,
                        1, 1, 1,
                        GL_RED_INTEGER, GL_UNSIGNED_SHORT,
                        materials_data);
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
