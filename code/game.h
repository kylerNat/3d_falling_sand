#ifndef GAME
#define GAME

#include <maths/maths.h>
#include "graphics_common.h"
#include "xaudio2_audio.h"
#include "game_common.h"
#include "genetics.h"
#include "memory.h"
#include "chunk.h"

real dt = 1.0/60.0; //simulate at 60 hz

struct element_componenticle
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

support_t point_hull_support(real_3 dir, element_componenticle* p, element_componenticle* hull, real_3 hull_x_dot, int hull_len)
{
    element_componenticle* best_h = hull;
    real best_d = dot(dir, hull->x);
    for(element_componenticle* h = hull+1; h < hull+hull_len; h++)
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

bool is_colliding(element_componenticle* p, element_componenticle* hull, real_3 hull_x_dot, int hull_len)
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
    int n_bodies;

    //TODO: turn everything into hashtables
    joint* joints;
    int n_joints;

    brain* brains;
    int n_brains;

    genome* genomes;
    int n_genomes;

    genedit_window gew;

    bool edit_mode;

    chunk * c;
    int8_2 chunk_lookup[2*2*2];
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

void set_chunk_region(memory_manager* manager, world* w, chunk* c, real_3 pos, int_3 size, uint16 material)
{
    // pos = clamp_per_axis(pos, 0, chunk_size-1);

    int_3 active_data_size = {(int(pos.x)%16+size.x+15)/16, (int(pos.y)%16+size.y+15)/16, (int(pos.z)%16+size.z+15)/16};

    byte* data = reserve_block(manager, size.x*size.y*size.z*3*sizeof(uint16)
                               + active_data_size.x*active_data_size.y*active_data_size.z*sizeof(uint));
    uint16* materials_data = (uint16*) data;
    uint* active_data = (uint*) (data+size.x*size.y*size.z*3*sizeof(uint16));
    for(int i = 0; i < active_data_size.x*active_data_size.y*active_data_size.z; i++)
        active_data[i] = {0xFFFFFFFF};

    for(int i = 0; i < size.x*size.y*size.z; i++)
    {
        materials_data[3*i+0] = material;
        materials_data[3*i+1] = 0;
        materials_data[3*i+2] = 0;
    }
    glBindTexture(GL_TEXTURE_3D, materials_textures[current_materials_texture]);
    glActiveTexture(GL_TEXTURE0 + 0);
    glTexSubImage3D(GL_TEXTURE_3D, 0,
                    pos.x, pos.y, pos.z,
                    size.z, size.y, size.z,
                    GL_RGB_INTEGER, GL_UNSIGNED_SHORT,
                    materials_data);

    glBindTexture(GL_TEXTURE_3D, active_regions_textures[current_active_regions_texture]);
    glTexSubImage3D(GL_TEXTURE_3D, 0,
                    pos.x/16, pos.y/16, pos.z/16,
                    active_data_size.x, active_data_size.y, active_data_size.z,
                    GL_RED_INTEGER, GL_UNSIGNED_INT,
                    active_data);
    unreserve_block(manager);
}

void spawn_particles(real_3 x, real_3 x_dot)
{
    particle_data_list particles = {
        .n_particles = 1,
        .particles = {{.voxel_data = {2, (2<<6), (8<<2), 0}, .x = x, .x_dot = x_dot, .alive = true,}},
    };

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(particles.n_particles)+sizeof(particle_data)*particles.n_particles, &particles);
}

void update_and_render(memory_manager* manager, world* w, render_data* rd, render_data* ui, audio_data* ad, user_input* input)
{
    bool toggle_edit = is_pressed('\t', input);
    w->edit_mode = w->edit_mode != toggle_edit;

    if(w->edit_mode)
    {
        do_edit_window(ui, input, &w->gew);
        real_4 cursor_color = {1,1,1,1};
        draw_circle(ui, {input->mouse.x, input->mouse.y, 0}, 0.01, cursor_color);
    }
    // else
    // {
    //     if(toggle_edit)
    //     {
    //         genome* gn = w->gew.active_genome;
    //         compile_genome(gn, w->gew.genodes);
    //     }
    // }

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
    if(!w->edit_mode)
    {
        theta += 0.8*input->dmouse.y;
        phi -= 0.8*input->dmouse.x;
    }
    // theta += 0.3*input->dmouse.y;
    // phi -= 0.3*input->dmouse.x;

    // real_3 look_forward = {-sin(phi)*sin(theta), cos(phi)*sin(theta), -cos(theta)};
    real_3 look_forward = {-sin(phi), cos(phi), 0};
    real_3 look_side    = {cos(phi), sin(phi), 0};

    real move_accel = 0.05;
    real move_speed = 1.5;
    real speed = norm(player->x_dot.xy);
    real z_dot = player->x_dot.z;
    real_2 move_dir = player->x_dot.xy/speed;
    real_3 walk_dir = (walk_inputs.y*look_forward + walk_inputs.x*look_side);
    if(!w->edit_mode)
        walk_dir.z += is_down(M2, input)-is_down(' ', input);
    if(speed == 0) move_dir = 0.5*walk_dir.xy;
    if(walk_dir.x != 0 || walk_dir.y != 0 || walk_dir.z != 0)
    {
        walk_dir = normalize(walk_dir);
        player->x_dot += move_accel*walk_dir;
        player->x_dot *= 1.0f-move_accel/move_speed;
    }
    else
    {
        player->x_dot *= 0.5;
    }

    // player->x_dot.x *= 0.95;
    // player->x_dot.y *= 0.95;
    // player->x_dot += 2*is_pressed(M2, input)*((real_3){0,0,1}+0.0*look_forward);
    // if(is_down(' ', input))
    // {
    //     // player->x_dot.x = 3*look_forward.x;
    //     // player->x_dot.y = 3*look_forward.y;

    //     // player->x_dot.x = 3*look_forward.x;
    //     player->x_dot.z -= move_accel;
    // }

    static int player_in_head = 0;

    if(!player_in_head) player->x += player->x_dot;

    real fov = pi*120.0/180.0;
    real screen_dist = 1.0/tan(fov/2);

    real n = 0.1;
    real f = 1000.0;

    static real camera_dist_target = 240;
    static real camera_dist = camera_dist_target;
    camera_dist_target *= pow(1.0+0.001, -input->mouse_wheel);
    const real min_camera_dist = 1;
    if(camera_dist_target < min_camera_dist) camera_dist_target = min_camera_dist;
    camera_dist += 0.2*(camera_dist_target-camera_dist);

    real_3 camera_z = {sin(theta)*sin(phi), -sin(theta)*cos(phi), cos(theta)};
    // real_3 camera_x = normalize(cross({0,0,1}, camera_z));
    real_3 camera_x = {cos(phi),sin(phi),0};
    real_3 camera_y = cross(camera_z, camera_x);
    real_3 camera_pos;

    camera_pos = player->x;

    real_3 foot_dist = (real_3){0,0,-15};

    // real_3 xs[1] = {player->x+foot_dist};
    // real_3 x_dots[1] = {player->x_dot};
    // simulate_particles(w->c, xs, x_dots, len(xs));
    // player->x = xs[0]-foot_dist;
    // player->x_dot = x_dots[0];

    // player->x += player->x_dot;

    static real placement_dist = 20.0;
    placement_dist += 0.02*input->mouse_wheel;

    for(int b = 0; b < w->n_bodies; b++)
    {
        gpu_body_data* body_gpu = &w->bodies_gpu[b];
        if(isnan(body_gpu->x.x) || isnan(body_gpu->x.y) || isnan(body_gpu->x.z) ) body_gpu->x = {0,0,0};
        if(isnan(body_gpu->x_dot.x) || isnan(body_gpu->x_dot.y) || isnan(body_gpu->x_dot.z) ) body_gpu->x_dot = {0,0,0};
        if(isnan(body_gpu->omega.x) || isnan(body_gpu->omega.y) || isnan(body_gpu->omega.z) ) body_gpu->omega = {0,0,0};
        if(isnan(body_gpu->orientation.r) || isnan(body_gpu->orientation.i) || isnan(body_gpu->orientation.j) || isnan(body_gpu->orientation.k))
        {
            body_gpu->orientation = {1,0,0,0};
            update_inertia(w->bodies_cpu+b, body_gpu);
        }

        // w->bodies_gpu[b].x_dot *= 0.995;
        // w->bodies_gpu[b].omega -= w->bodies_gpu[b].invI*0.005*w->bodies_gpu[b].omega;
        // if(is_down('Y', input))
        // w->bodies_gpu[b].x_dot.z += -0.01;
        // else
        // w->bodies_gpu[b].x_dot.z += -0.1;
    }

    // static float supported_weight = 0.0;
    // static float leg_phase = 0.0;
    // {
    //     real multiplier = 30;
    //     real target_height = 14;
    //     // multiplier *= -(1.0*(w->bodies_gpu[13].x.z-0.5*w->bodies_gpu[5].x.z-0.5*w->bodies_gpu[7].x.z-target_height));
    //     if(is_down('T', input) || (player_in_head && is_down(M2, input))) multiplier = 100;
    //     bool striding = fmod(leg_phase, 0.5) > 0.1;
    //     if(leg_phase > 0.5)
    //     {
    //         w->bodies_cpu[5].contact_locked[0] = striding;
    //         w->bodies_cpu[7].contact_locked[0] = false;
    //         if(striding)
    //         {
    //             w->bodies_gpu[13].x_dot.z += 0.001*multiplier/w->bodies_gpu[13].m;
    //             w->bodies_gpu[5].x_dot.z  -= 0.001*multiplier*1.0/w->bodies_gpu[5].m;
    //         }
    //         w->bodies_gpu[5].x_dot.z  -= 0.001*multiplier*0.02/w->bodies_gpu[5].m;
    //         w->bodies_gpu[7].x_dot.z  -= -0.001*multiplier*0.02/w->bodies_gpu[7].m;
    //     }
    //     else
    //     {
    //         w->bodies_cpu[7].contact_locked[0] = striding;
    //         w->bodies_cpu[5].contact_locked[0] = false;
    //         if(striding)
    //         {
    //             w->bodies_gpu[13].x_dot.z += 0.001*multiplier/w->bodies_gpu[13].m;
    //             w->bodies_gpu[7].x_dot.z  -= 0.001*multiplier*1.0/w->bodies_gpu[7].m;
    //         }
    //         w->bodies_gpu[7].x_dot.z  -= 0.001*multiplier*0.02/w->bodies_gpu[7].m;
    //         w->bodies_gpu[5].x_dot.z  -= -0.001*multiplier*0.02/w->bodies_gpu[5].m;
    //     }

    //     if(is_down('R', input) || (player_in_head && (walk_dir.x != 0 || walk_dir.y != 0)))
    //     {
    //         real_3 pos = player->x-placement_dist*camera_z;
    //         real_3 target_dir = pos-w->bodies_gpu[13].x;
    //         target_dir.z = 0;
    //         if(player_in_head && (walk_dir.x != 0 || walk_dir.y != 0)) target_dir = walk_dir;
    //         target_dir = normalize(target_dir);

    //         real walk_accel = 0.01;
    //         w->bodies_gpu[13].x_dot += (walk_accel/w->bodies_gpu[13].m)*target_dir;
    //         if(fmod(leg_phase+0.8,1.0) > 0.5)
    //         {
    //             w->bodies_gpu[5].x_dot -= (walk_accel*1.1/w->bodies_gpu[5].m)*target_dir;
    //             w->bodies_gpu[7].x_dot -= -(walk_accel*0.1/w->bodies_gpu[7].m)*target_dir;
    //         }
    //         else
    //         {
    //             w->bodies_gpu[7].x_dot -= (walk_accel*1.1/w->bodies_gpu[7].m)*target_dir;
    //             w->bodies_gpu[5].x_dot -= -(walk_accel*0.1/w->bodies_gpu[5].m)*target_dir;
    //         }

    //         // w->bodies_gpu[7].omega  *= 0.0;
    //         // w->bodies_gpu[5].omega  *= 0.0;
    //         w->bodies_gpu[12].omega += 0.2*cross(apply_rotation(w->bodies_gpu[12].orientation, (real_3){0,0,1}), target_dir);
    //         w->bodies_gpu[13].omega += 0.2*cross(apply_rotation(w->bodies_gpu[13].orientation, (real_3){1,0,0}), target_dir);
    //     }

    //     // w->bodies_gpu[13].x_dot.z *= 0.8;
    // }
    // leg_phase += 0.03;
    // leg_phase = fmod(leg_phase, 1.0);

    player_in_head = (player_in_head + is_pressed('V', input))%3;

    {
        brain* br = &w->brains[0];
        // br->target_accel = (real_3){0,0,-0.1*(-0.5+w->bodies_gpu[13].x_dot.z)}; //by default try to counteract gravity
        br->target_accel = (real_3){0,0,0.00}; //by default try to counteract gravity
        // br->target_accel = (real_3){0,0,0}; //by default try to counteract gravity

        br->is_moving = is_down('R', input) || (player_in_head && (walk_dir.x != 0 || walk_dir.y != 0));
        if(br->is_moving)
        {
            real_3 pos = player->x-placement_dist*camera_z;
            real_3 r = pos-w->bodies_gpu[13].x;
            // r.z = 0;
            // r = rej(r, w->bodies_cpu[13].contact_normals[0]);
            real_3 target_dir = normalize(r);
            if(player_in_head)
            {
                target_dir = walk_dir;
            }

            br->target_accel.z += 0.05;
            br->target_accel += 0.05*target_dir;
            br->target_accel -= 0.02*w->bodies_gpu[13].x_dot;
        }
        else
        {
            // br->target_accel -= 0.02*w->bodies_gpu[13].x_dot;
            br->target_accel.z += 0.02;
            // br->target_accel.z -= 0.01*(w->bodies_gpu[13].x.z-(w->bodies_gpu[5].x.z+16));
        }
        if(is_down('T', input)
           || (player_in_head && is_down(M2, input)))
        {
            br->is_moving = true;
            br->target_accel.z += 0.1;
            //TODO: should be foot normals
            // br->target_accel += 0.05*w->bodies_cpu[13].contact_normals[0];
        }

        if(is_down('Y', input)
           || (player_in_head && is_down(' ', input)))
        {
            br->is_moving = true;
            br->target_accel.z -= 0.1;
        }
    }

    // simulate_brains(w->bodies_cpu, w->bodies_gpu, w->brains, w->n_brains);
    // for(int j = 0; j < w->n_joints; j++)
    // {
    //     w->joints[j].torques = {0,0,0};
    // }

    simulate_bodies(w->bodies_cpu, w->bodies_gpu, w->n_bodies);
    simulate_body_physics(manager, w->bodies_cpu, w->bodies_gpu, w->n_bodies, rd);

    // w->bodies_cpu[5].contact_locked[0] = true;
    // w->bodies_cpu[5].contact_points[0] = {256+128, 256+128, 50};
    // w->bodies_cpu[5].contact_pos[0] = {5,0,0};
    // w->bodies_cpu[5].contact_normals[0] = {1,0,0};
    // w->bodies_cpu[5].contact_depths[0] = 0;
    // w->bodies_cpu[5].contact_forces[0] = {0,0,0};
    // w->bodies_cpu[7].contact_forces[0] = {0,0,0};
    // w->bodies_cpu[5].deltax_dot_integral[0] = {0,0,0};

    simulate_joints(w->bodies_cpu, w->bodies_gpu, w->n_bodies, w->brains, w->n_brains, rd);

    integrate_body_motion(w->bodies_cpu, w->bodies_gpu, w->n_bodies);

    if(player_in_head == 2) player->x = lerp(player->x, w->bodies_gpu[13].x+10.0*camera_z+2*camera_y, 0.3);
    if(player_in_head == 1) player->x = w->bodies_gpu[13].x-5*camera_z;

    // if(is_pressed('V', input))
    // {
    //     play_sound(ad, &waterfall, 0.1);
    // }

    static int selected_body = 0;
    if(is_pressed('Q', input)) selected_body--;
    if(is_pressed('E', input)) selected_body++;
    if(w->n_bodies) selected_body = (selected_body+w->n_bodies)%w->n_bodies;

    if(is_down('F', input))
    {
        for(int b = 0; b < w->n_bodies; b++)
        {
            real_3 r = {0,0,-27.0/2};
            real_3 deltax_dot = {-sin(theta)*sin(phi), sin(theta)*cos(phi), -cos(theta)};
            deltax_dot *= 0.1;
            real_3 deltaomega = w->bodies_gpu[b].m*(w->bodies_gpu[b].invI*cross(r, deltax_dot));
            w->bodies_gpu[b].x_dot += deltax_dot;
            w->bodies_gpu[b].omega += deltaomega;
        }
    }

    if(is_down('G', input))
    {
        // for(int b = 0; b < w->n_bodies; b++)
        // {
        //     real_3 pos = player->x-placement_dist*camera_z;
        //     w->bodies_gpu[b].x_dot += 0.02*(pos-w->bodies_gpu[b].x);
        //     w->bodies_gpu[b].x_dot *= 0.9;
        //     w->bodies_gpu[b].omega *= 0.95;
        // }

        real_3 pos = player->x-placement_dist*camera_z;
        w->bodies_gpu[selected_body].x_dot += 0.1*(pos-w->bodies_gpu[selected_body].x);
        w->bodies_gpu[selected_body].x_dot *= 0.9;
        // w->bodies_gpu[selected_body].omega *= 0.95;
    }

    static int current_material = 1;
    for(int i = 1; i <= 8; i++)
    {
        if(is_down('0'+i, input))
        {
            current_material = i;
        }
    }

    if(is_down(M1, input) && !w->edit_mode)
    {
        int brush_size = 50;
        real_3 pos = player->x-(placement_dist+brush_size/2)*camera_z-(real_3){brush_size/2,brush_size/2,brush_size/2};
        pos = clamp_per_axis(pos, 0, chunk_size-brush_size-1);
        set_chunk_region(manager, w, w->c, pos, {brush_size, brush_size, brush_size}, current_material);
    }

    // if(is_down('T', input))
    // {
    //     real_3 pos = player->x-placement_dist*camera_z;
    //     pos = clamp_per_axis(pos, 0, chunk_size-11);
    //     set_chunk_region(manager, w, w->c, pos, {10,10,10}, 2);
    // }

    if(is_down(M4, input))
    {
        // real_3 pos = player->x-placement_dist*camera_z;
        // pos = clamp_per_axis(pos, 0, chunk_size-1);
        // set_chunk_voxel(w, w->c, pos, 2);

        real_3 pos = player->x-placement_dist*camera_z;
        pos = clamp_per_axis(pos, 0, chunk_size-11);
        set_chunk_region(manager, w, w->c, pos, {10,10,10}, 2);
    }

    if(is_down(M5, input))
    {
        // real_3 pos = player->x-placement_dist*camera_z;
        // pos = clamp_per_axis(pos, 0, chunk_size-1);
        // set_chunk_voxel(w, w->c, pos, 0);

        real_3 pos = player->x-placement_dist*camera_z;
        pos = clamp_per_axis(pos, 0, chunk_size-11);
        set_chunk_region(manager, w, w->c, pos, {10,10,10}, 0);
    }

    // if(is_down(M3, input))
    // {
    //     real_3 pos = player->x-placement_dist*camera_z;
    //     pos = clamp_per_axis(pos, 0, chunk_size-1);
    //     spawn_particles(pos, -1*camera_z);
    // }

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
