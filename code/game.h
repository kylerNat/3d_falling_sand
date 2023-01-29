#ifndef GAME
#define GAME

#include <maths/maths.h>
#include "graphics_common.h"
#include "xaudio2_audio.h"
#include "game_common.h"
#include "genetics.h"
#include "memory.h"

const real dt = 1.0/60.0;

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

int create_body_from_form(world* w, genome* g, brain* br, int form_id)
{
    cpu_form_data* form_cpu = &g->forms_cpu[form_id];
    gpu_form_data* form_gpu = &g->forms_gpu[form_id];

    int b = new_body_index(w);
    cpu_body_data* body_cpu = &w->bodies_cpu[b];
    gpu_body_data* body_gpu = &w->bodies_gpu[b];

    body_cpu->genome_id = br->genome_id;
    body_cpu->form_id = form_id;
    body_gpu->brain_id = br->id;

    body_cpu->is_root = form_cpu->is_root;
    if(body_cpu->is_root) br->root_id = body_cpu->id;
    body_cpu->root = br->root_id;

    int pad = 1;
    int_3 padding = {pad, pad, pad};
    body_gpu->form_offset = form_gpu->x_origin+padding;
    body_gpu->orientation = {1,0,0,0};

    //TODO: actually calculate these
    body_gpu->m = 0.01;
    body_cpu->I = real_identity_3(10.0);
    body_cpu->invI = inverse(body_cpu->I);
    update_inertia(body_cpu, body_gpu);

    body_gpu->cell_material_id = form_gpu->cell_material_id;

    body_gpu->substantial = true;

    body_gpu->form_lower  = form_gpu->materials_origin+form_gpu->material_bounds.l;
    body_gpu->form_upper  = form_gpu->materials_origin+form_gpu->material_bounds.u;
    int_3 form_size = body_gpu->form_upper-body_gpu->form_lower;

    load_empty_body_to_gpu(&w->body_space, body_cpu, body_gpu, form_size);

    body_gpu->form_origin = form_gpu->materials_origin+form_gpu->x_origin-body_gpu->form_offset-body_gpu->texture_region.l;

    return body_cpu->id;
}

void create_body_joint_from_form_joint(world* w, genome* g, brain* br, form_joint* joint)
{

    body_joint* new_joint = &w->joints[w->n_joints++];
    *new_joint = {
        .type = joint->type,
    };
    for(int i = 0; i < 2; i++)
    {
        new_joint->body_id[i] = br->body_ids[joint->form_id[i]];
        int_3 padding = {1,1,1};
        int b = get_body_index(w, new_joint->body_id[i]);
        new_joint->pos[i] = joint->pos[i];
        new_joint->axis[i] = joint->axis[i];
    }
}

void create_body_endpoint_from_form_endpoint(brain* br, form_endpoint* ep)
{

    endpoint* new_endpoint = &br->endpoints[br->n_endpoints++];
    *new_endpoint = {
        .type = ep->type,
        .body_id = br->body_ids[ep->form_id],
        .pos = ep->pos,
    };
}

void creature_from_genome(memory_manager* manager, world* w, genome* g, brain* br)
{
    br->n_max_bodies = g->n_forms+4;
    br->body_ids = (int*) dynamic_alloc(sizeof(int)*br->n_max_bodies);

    for(int f = 0; f < g->n_forms; f++)
    {
        br->body_ids[br->n_bodies++] = create_body_from_form(w, g, br, f);
    }

    for(int bi = 0; bi < br->n_bodies; bi++)
    {
        int b = get_body_index(w, br->body_ids[bi]);
        assert(b >= 0);
        w->bodies_cpu[b].root = br->root_id;
    }

    for(int j = 0; j < g->n_joints; j++)
    {
        create_body_joint_from_form_joint(w, g, br, &g->joints[j]);
    }
}

void set_chunk_region(memory_manager* manager, world* w, chunk* c, real_3 pos, int_3 size, uint8_4 voxel)
{
    // pos = clamp_per_axis(pos, 0, chunk_size-1);

    int_3 active_data_size = {(int(pos.x)%16+size.x+15)/16, (int(pos.y)%16+size.y+15)/16, (int(pos.z)%16+size.z+15)/16};

    byte* data = stalloc(size.x*size.y*size.z*4*sizeof(uint8)
                         + active_data_size.x*active_data_size.y*active_data_size.z*sizeof(uint));
    uint8* materials_data = (uint8*) data;
    uint* active_data = (uint*) (data+size.x*size.y*size.z*4*sizeof(uint8));
    for(int i = 0; i < active_data_size.x*active_data_size.y*active_data_size.z; i++)
        active_data[i] = {0xFFFFFFFF};

    for(int i = 0; i < size.x*size.y*size.z; i++)
    {
        materials_data[4*i+0] = voxel[0];
        materials_data[4*i+1] = voxel[1];
        materials_data[4*i+2] = voxel[2];
        materials_data[4*i+3] = voxel[3];
    }
    glBindTexture(GL_TEXTURE_3D, materials_textures[current_materials_texture]);
    glActiveTexture(GL_TEXTURE0 + 0);
    glTexSubImage3D(GL_TEXTURE_3D, 0,
                    pos.x, pos.y, pos.z,
                    size.z, size.y, size.z,
                    GL_RGBA_INTEGER, GL_UNSIGNED_BYTE,
                    materials_data);

    glBindTexture(GL_TEXTURE_3D, active_regions_textures[current_active_regions_texture]);
    glTexSubImage3D(GL_TEXTURE_3D, 0,
                    pos.x/16, pos.y/16, pos.z/16,
                    active_data_size.x, active_data_size.y, active_data_size.z,
                    GL_RED_INTEGER, GL_UNSIGNED_INT,
                    active_data);
    stunalloc(data);
}

void spawn_particles(real_3 x, real_3 x_dot)
{
    particle_data_list particles = {
        .n_particles = 1,
        .particles = {{.voxel_data = {3, (2<<6), (8<<2), 15}, .x = x, .x_dot = x_dot, .alive = true,}},
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
        do_edit_window(manager, ui, input, &w->gew, &w->form_space);
        real_4 cursor_color = {1,1,1,1};
        if(input->active_ui_element.type != ui_form_voxel)
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
    if(!w->edit_mode && !debug_menu_active)
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
    placement_dist += 1.0*input->mouse_wheel;

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

    player_in_head = (player_in_head + is_pressed('V', input))%3;

    {
        brain* br = &w->brains[0];
        // br->target_accel = (real_3){0,0,-0.1*(-0.5+w->bodies_gpu[12].x_dot.z)}; //by default try to counteract gravity
        br->target_accel = (real_3){0,0,0.00}; //by default try to counteract gravity
        // br->target_accel = (real_3){0,0,0}; //by default try to counteract gravity

        if(player_in_head) br->look_dir = -camera_z;

        br->is_moving = is_down('R', input) || (player_in_head && (walk_dir.x != 0 || walk_dir.y != 0));
        if(br->is_moving)
        {
            real_3 pos = player->x-placement_dist*camera_z;
            int root_index = get_body_index(w, br->root_id);
            real_3 r = pos-w->bodies_gpu[root_index].x;
            // r.z = 0;
            // r = rej(r, w->bodies_cpu[12].contact_normals[0]);
            real_3 target_dir = normalize(r);
            if(player_in_head)
            {
                target_dir = walk_dir;
            }

            if(!player_in_head) br->look_dir = target_dir;

            br->target_accel.z += 0.02;
            br->target_accel += 0.02*target_dir;
            // br->target_accel -= 0.02*w->bodies_gpu[12].x_dot;
        }
        else
        {
            // br->target_accel -= 0.02*w->bodies_gpu[12].x_dot;
            // br->target_accel.z += 0.01;
            // br->target_accel.z -= 0.01*(w->bodies_gpu[12].x.z-(w->bodies_gpu[5].x.z+16));
        }
        if(is_down('T', input)
           || (player_in_head && is_down(M2, input)))
        {
            br->is_moving = true;
            br->target_accel.z += 0.1;
            //TODO: should be foot normals
            // br->target_accel += 0.05*w->bodies_cpu[12].contact_normals[0];
        }

        // if(is_down('Y', input)
        //    || (player_in_head && is_down(' ', input)))
        // {
        //     br->is_moving = true;
        //     if(w->bodies_cpu[12].contact_depths[0] > 5)
        //     br->target_accel.z -= 0.1;
        // }

        br->kick = {};
        if(player_in_head && is_down(' ', input))
        {
            if(br->kick_frames++ < 5)
                br->kick += -0.05*camera_z;
            // br->kick += 0.1*walk_dir;
        }
        else
        {
            br->kick_frames = 0;
        }
    }

    // simulate_brains(w->bodies_cpu, w->bodies_gpu, w->brains, w->n_brains);
    // for(int j = 0; j < w->n_joints; j++)
    // {
    //     w->joints[j].torques = {0,0,0};
    // }

    for(int b = w->n_bodies-1; b >= 0; b--)
    {
        cpu_body_data* body_cpu = &w->bodies_cpu[b];
        gpu_body_data* body_gpu = &w->bodies_gpu[b];

        if(body_cpu->genome_id == 0) continue;

        genome* g = get_genome(w, body_cpu->genome_id);
        if(!g->is_mutating) continue;
        brain* br = get_brain(w, body_gpu->brain_id);

        int new_form_id = g->form_id_updates[body_cpu->form_id];
        br->body_ids[new_form_id] = body_cpu->id;
        if(new_form_id < 0)
        {
            for(int j = w->n_joints-1; j >= 0; j--)
            {
                for(int i = 0; i < 2; i++)
                {
                    int body_id = w->joints[j].body_id[i];
                    if(body_id == body_cpu->id)
                    {
                        w->joints[j] = w->joints[--w->n_joints];
                        break;
                    }
                }
            }
            br->body_ids[body_cpu->form_id] = br->body_ids[--br->n_bodies];
            delete_body(w, body_cpu->id);
            continue;
        }
        body_cpu->form_id = new_form_id;

        cpu_form_data* form_cpu = &g->forms_cpu[new_form_id];
        gpu_form_data* form_gpu = &g->forms_gpu[new_form_id];

        body_gpu->is_mutating = form_gpu->is_mutating;
        if(form_gpu->is_mutating)
        {
            int_3 padding = {1,1,1};
            //form_offset: body->texture_region.l -> O
            //form_gpu->x_origin: form->materials_origin -> O
            int_3 offset = (form_gpu->x_origin-body_gpu->form_offset)+padding;
            int_3 new_size = form_gpu->material_bounds.u-form_gpu->material_bounds.l+2*padding;
            new_size.x = 4*((new_size.x+3)/4);
            //TODO: this does not detect a region change if one side is deleted and the other side is extended
            if(new_size != body_gpu->texture_region.u-body_gpu->texture_region.l)
            {
                log_output("resizing body storage ", body_gpu->texture_region.l, ", ", body_gpu->texture_region.u, "\n");
                resize_body_storage(&w->body_space, body_cpu, body_gpu, offset, new_size);
                log_output("resized body storage ", body_gpu->texture_region.l, ", ", body_gpu->texture_region.u, "\n");
            }
            body_gpu->form_origin = form_gpu->materials_origin+form_gpu->x_origin-body_gpu->form_offset-body_gpu->texture_region.l;
            body_gpu->form_lower  = form_gpu->materials_origin+form_gpu->material_bounds.l;
            body_gpu->form_upper  = form_gpu->materials_origin+form_gpu->material_bounds.u;
        }
    }

    for(int brain_index = 0; brain_index < w->n_brains; brain_index++)
    {
        brain* br = &w->brains[brain_index];
        genome* g = get_genome(w, br->genome_id);

        if(!g->is_mutating) continue;

        int n_new_forms = g->n_forms - br->n_bodies;
        for(int f = g->n_forms-n_new_forms; f < g->n_forms; f++)
        {
            if(br->n_bodies >= br->n_max_bodies)
            {
                br->n_max_bodies = br->n_bodies+1 + 4;
                int* new_body_ids = (int*) dynamic_alloc(sizeof(int)*br->n_max_bodies);
                memcpy(new_body_ids, br->body_ids, sizeof(int)*br->n_bodies);
                dynamic_free(br->body_ids);
                br->body_ids = new_body_ids;
            }
            int b = create_body_from_form(w, g, br, f);
            br->body_ids[br->n_bodies++] = b;
        }

        for(int j = 0; j < g->n_joints; j++)
        {
            if(g->joints[j].form_id[0] >= g->n_forms-n_new_forms
               || g->joints[j].form_id[1] >= g->n_forms-n_new_forms)
            {
                create_body_joint_from_form_joint(w, g, br, &g->joints[j]);
                log_output("creating body joint\n");
            }
        }

        br->n_endpoints = 0;
        for(int e = 0; e < g->n_endpoints; e++)
        {
            create_body_endpoint_from_form_endpoint(br, &g->endpoints[e]);
            // log_output("creating form endpoint: ", g->endpoints[e].type, ", ", g->endpoints[e].form_id, ", ", g->endpoints[e].pos, "\n");
        }
    }

    for(int genome_index = 0; genome_index < w->n_genomes; genome_index++)
    {
        genome* g = &w->genomes[genome_index];
        g->is_mutating = false;
    }

    static bool step_mode = true;
    step_mode = step_mode!=is_pressed(VK_OEM_PERIOD, input);
    if(step_mode ? is_pressed('M', input) : !is_down('M', input))
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
            update_bounding_box(body_gpu);
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

        simulate_bodies(manager, w, w->gew.active_genome->forms_gpu, w->gew.active_genome->n_forms);
        sync_joint_voxels(manager, w);
        simulate_body_physics(manager, w, rd);
        update_joint_fragments(manager, w);

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
    }

    //drawing circles
    static bool draw_circles = false;
    draw_circles = draw_circles != is_pressed(VK_F2, input);
    if(draw_circles)
    {
        for(int b = 0; b < w->n_bodies; b++)
        {
            cpu_body_data* body_cpu = &w->bodies_cpu[b];
            gpu_body_data* body_gpu = &w->bodies_gpu[b];

            // for(int i = 0; i < 3; i++)
            // {
            //     draw_circle(rd, body_cpu->contact_points[i], 0.3, {body_cpu->contact_depths[i] > 1,body_cpu->contact_depths[i] <= 1,0,1});
            //     draw_circle(rd, body_cpu->contact_points[i]+body_cpu->contact_normals[i], 0.3, {0,0,1,1});
            // }

            //     brain* br = &w->brains[0];
            //         for(int z = 0; z < 2; z++)
            //             for(int y = 0; y < 2; y++)
            //                 for(int x = 0; x < 2; x++)
            //                 {
            //                     real_3 r = apply_rotation(body_gpu->orientation, -body_gpu->x_cm+multiply_per_axis(real_cast(body_gpu->size), (real_3){x,y,z}));
            //                     draw_circle(rd, body_gpu->x+r , 0.1, {0,1,1,1});

            //                     if(br->root_id == body_cpu->id)
            //                     {
            //                         real_3 r_storage = apply_rotation(body_gpu->orientation, -body_gpu->x_cm-real_cast(body_gpu->materials_origin-body_cpu->texture_region.l)+multiply_per_axis(real_cast(body_cpu->texture_region.u-body_cpu->texture_region.l), (real_3){x,y,z}));
            //                         draw_circle(rd, body_gpu->x+r_storage , 0.1, {0,0.5,1,1});
            //                     }
            //                 }

            //         real_3 r_origin = apply_rotation(body_gpu->orientation, -body_gpu->x_cm+real_cast(-body_gpu->materials_origin+body_cpu->texture_region.l+body_gpu->form_offset));
            //         draw_circle(rd, body_gpu->x+r_origin , 0.1, {1,0,0,1});

            draw_circle(rd, body_gpu->x, 0.1, {1,0,1,1});
            draw_circle(rd, body_gpu->box.l, 0.1, {1,0,0,1});
            draw_circle(rd, body_gpu->box.u, 0.1, {1,0.2,0,1});
        }

        for(int c = 0; c < w->n_contacts; c++)
        {
            contact_point* contact = &w->contacts[c];
            if(contact->body_index[1] == -1)
            {
                int b = contact->body_index[0];
                cpu_body_data* body_cpu = &w->bodies_cpu[b];
                gpu_body_data* body_gpu = &w->bodies_gpu[b];

                real_3 r = apply_rotation(body_gpu->orientation, contact->pos[0]-body_gpu->x_cm);
                real_3 x = r+body_gpu->x;

                // draw_circle(rd, contact->x-contact->normal*contact->depth[1], 0.15, {0,1,1,1});
                draw_circle(rd, contact->pos[1]-contact->normal*contact->depth[1], 0.15, {0,1,0,1});
                draw_circle(rd, x, 0.1, {0,1,1,1});
            }
            else
            {
                for(int a = 0; a <= 1; a++)
                {
                    int b = contact->body_index[a];
                    cpu_body_data* body_cpu = &w->bodies_cpu[b];
                    gpu_body_data* body_gpu = &w->bodies_gpu[b];

                    real_3 r = apply_rotation(body_gpu->orientation, contact->pos[a]-body_gpu->x_cm);
                    real_3 x = r+body_gpu->x;
                    if(a == 0) x -= contact->normal*contact->depth[1];

                    draw_circle(rd, x, 0.15-0.05*a, {0,1,a,1});
                }
            }
        }
    }

    #ifdef IMGUI_VERSION
    debug_menu.body_inspector_active = debug_menu.body_inspector_active != is_pressed(VK_F1, input);
    if(debug_menu.body_inspector_active)
    {
        ImGui::Begin("Body Inspector", &debug_menu.body_inspector_active, 0);
        int j = debug_menu.active_joint;
        char item_name[100];

        body_joint* joint = &w->joints[j];
        real_3 u = {0,0,0};
        for(int a = 0; a <= 1; a++)
        {
            int b = get_body_index(w, joint->body_id[a]);
            gpu_body_data* body_gpu = &w->bodies_gpu[b];
            cpu_body_data* body_cpu = &w->bodies_cpu[b];
            real_3 r = apply_rotation(body_gpu->orientation, real_cast(joint->pos[a]+body_gpu->form_offset)+(real_3){0.5,0.5,0.5}-body_gpu->x_cm);
            draw_circle(rd, body_gpu->x + r, 0.2, {1,0.5+0.5*a,0,1});

            real_3 pos = r+body_gpu->x;
            u += (a?-1:1)*pos;
        }

        int b0 = w->joints[j].body_id[0];
        int b1 = w->joints[j].body_id[1];
        sprintf(item_name, "joint %d (bodies %d & %d)", j, b0, b1);
        if(ImGui::BeginCombo("selected joint", item_name))
        {
            for(int j = 0; j < w->n_joints; j++)
            {
                sprintf(item_name, "joint %d (bodies %d & %d)", j, w->joints[j].body_id[0], w->joints[j].body_id[1]);
                bool is_selected = j == debug_menu.active_joint;
                if(ImGui::Selectable(item_name, is_selected)) debug_menu.active_joint = j;
                if(is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PlotLines(debug_menu.plotname, debug_menu.plotline, debug_menu.n_plotline_points, 0, 0, -5, 5, ImVec2(0, 80));

        ImGui::Text("current error: %f", norm(u));

        ImGui::End();
    }
    #endif

    // for(int j = 0; j < w->n_joints; j++)
    // {

    //     body_joint* joint = &w->joints[j];
    //     for(int a = 0; a <= 1; a++)
    //     {
    //         int b = get_body_index(w, joint->body_id[a]);
    //         gpu_body_data* body = &w->bodies_gpu[b];
    //         real_3 r = apply_rotation(body->orientation, real_cast(joint->pos[a]+body->form_offset)+(real_3){0.5,0.5,0.5}-body->x_cm);
    //         draw_circle(rd, body->x+r, 0.1, {0,1,0,1});
    //     }
    // }

    // for(int brain_id = 0; brain_id < w->n_brains; brain_id++)
    // {
    //     brain* br = &w->brains[brain_id];

    //     for(int e = 0; e < br->n_endpoints; e++)
    //     {
    //         endpoint* ep = &br->endpoints[e];
    //         gpu_body_data* body_gpu = &w->bodies_gpu[ep->body_id];
    //         cpu_body_data* body_cpu = &w->bodies_cpu[ep->body_id];
    //         rd->log_pos
    //             += sprintf(rd->debug_log+rd->log_pos,
    //                        "foot %i state: %s, phase = %.2f, coyote = %.2f, depth = %.2f, %s\n",
    //                        ep->body_id,
    //                        ep->is_grabbing ? "grab" : "free",
    //                        ep->foot_phase, ep->coyote_time,
    //                        body_cpu->contact_depths[0],
    //                        body_cpu->contact_locked[0] ? "locked" : "unlock"
    //                 );
    //     }
    // }


    {
        brain* br = &w->brains[0];
        int root_index = get_body_index(w, br->root_id);
        if(player_in_head == 2) player->x = lerp(player->x, w->bodies_gpu[root_index].x+10.0*camera_z+2*camera_y, 0.8);
        if(player_in_head == 1) player->x = w->bodies_gpu[root_index].x-5*camera_z;
    }

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

    if(is_pressed('G', input))
    {
        real closest_normsq = sq(1000);
        for(int b = 0; b < w->n_bodies; b++)
        {
            if(!w->bodies_gpu[b].substantial) continue;
            real_3 pos = player->x-placement_dist*camera_z;
            real new_normsq = normsq(pos-w->bodies_gpu[b].x);
            if(new_normsq < closest_normsq)
            {
                selected_body = b;
                closest_normsq = new_normsq;
            }
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
        real_3 force = 0.1*(pos-w->bodies_gpu[selected_body].x);
        // if(normsq(force) > 0.5) force = 0.5*normalize(force);
        w->bodies_gpu[selected_body].x_dot += force;
        w->bodies_gpu[selected_body].x_dot *= 0.9;
        // w->bodies_gpu[selected_body].omega *= 0.95;
    }

    w->n_explosions = 0;
    w->n_beams = 0;

    if(is_down('X', input))
    {
        real_3 pos = player->x-placement_dist*camera_z;
        real radius = 10;
        w->explosions[w->n_explosions++] = {pos, radius, 100};

        uint active_data = 0xFFFFFFFF;
        real_3 rrr = (real_3){radius, radius, radius};
        int_3 lower = int_cast(pos-rrr)/16;
        int_3 size = (int_cast(rrr)*2+(int_3){15,15,15})/16;
        lower = clamp_per_axis(lower, {0,0,0}, {31, 31, 31});
        size = clamp_per_axis(size+lower, {0,0,0}, {31, 31, 31})-lower;
        glClearTexSubImage(active_regions_textures[current_active_regions_texture], 0,
                           lower.x, lower.y, lower.z,
                           size.x, size.y, size.z,
                           GL_RGB_INTEGER, GL_UNSIGNED_INT,
                           &active_data);
    }

    if(is_down('C', input))
    {
        real_3 pos = player->x;
        real radius = 2;
        pos -= radius*camera_y;
        w->beams[w->n_beams++] = {pos, -camera_z, radius, 200, 100};
    }

    static int current_material = 1;
    for(int i = 1; i <= 8; i++)
    {
        if(is_down('0'+i, input))
        {
            current_material = i;
        }
    }

    if(is_down(M1, input) && !w->edit_mode && !debug_menu_active)
    {
        int brush_size = 50;
        real_3 pos = player->x-(placement_dist+brush_size/2)*camera_z-(real_3){brush_size/2,brush_size/2,brush_size/2};
        pos = clamp_per_axis(pos, 0, chunk_size-brush_size-1);
        set_chunk_region(manager, w, w->c, pos, {brush_size, brush_size, brush_size}, {current_material,1<<5,0,current_material==4?15:0});
    }

    if(is_down(M3, input) && !w->edit_mode)
    {
        int brush_size = 1;
        real_3 pos = player->x-(placement_dist+brush_size/2)*camera_z-(real_3){brush_size/2,brush_size/2,brush_size/2};
        pos = clamp_per_axis(pos, 0, chunk_size-brush_size-1);
        set_chunk_region(manager, w, w->c, pos, {brush_size, brush_size, brush_size}, {current_material,0,0,current_material==4?15:0});
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
        set_chunk_region(manager, w, w->c, pos, {10,10,10}, {6, 0, 255, 0});
    }

    if(is_down(M5, input))
    {
        // real_3 pos = player->x-placement_dist*camera_z;
        // pos = clamp_per_axis(pos, 0, chunk_size-1);
        // set_chunk_voxel(w, w->c, pos, 0);

        real_3 pos = player->x-placement_dist*camera_z;
        pos = clamp_per_axis(pos, 0, chunk_size-11);
        set_chunk_region(manager, w, w->c, pos, {10,10,10}, {0});
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

    // for(int wc = 0; wc < n_worker_contexts; wc++)
    // {
    //     // for(int i = 0; i < AUDIO_BUFFER_MAX_LEN; i++)
    //     // {
    //     //     int buffer_index = (ad->samples_played+i)%ad->buffer_length;
    //     //     ad->buffer[buffer_index] += worker_context_list[wc]->audio_buffer[i];
    //     // }
    //     memset(worker_context_list[wc]->audio_buffer, 0, AUDIO_BUFFER_MAX_SIZE);
    // }

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

    // while(n_remaining_tasks) pop_work(main_context);
}

#endif //GAME
