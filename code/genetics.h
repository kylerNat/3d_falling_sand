#ifndef GENETICS
#define GENETICS

#include "graphics_common.h"
#include "materials.h"

#define N_MAX_CELL_TYPES 30
#define N_MAX_GENES 30 //max genes per cell type
#define N_MAX_TRIGGERS (N_MAX_GENES/2)

#define genode_radius 0.03
#define genode_spacing 0.06
#define gene_row_spacing 0.07

struct cell_type;

#define form_texture_size 128

struct genode
{
    int gene_id;

    //icon state
    real_2 x;
    real theta;

    cell_type* root_cell;
};

struct cell_type
{
    int genodes[N_MAX_GENES];
    int genes[N_MAX_GENES];
    int n_genes;
    material_visual_info* visual;
    material_physical_info* physical;
};

//a form is the analogue of a body that exists in editor space

struct form_joint
{
    int type;
    int form_id[2];
    int_3 pos[2];
    int axis[2];
};

struct form_endpoint
{
    int type;
    int form_id;
    int_3 pos;
};

#pragma pack(push, 1)
struct gpu_form_data
{
    int_3 materials_origin;
    int_3 size;

    real_3 x_cm;

    real_3 x;
    quaternion orientation;

    int cell_material_id;
    int is_mutating;
};
#pragma pack(pop)

struct cpu_form_data
{
    gpu_form_data* gpu_data;

    real_3 x_dot;
    real_3 omega;

    uint8* materials;
    int storage_level;
};

struct genome
{
    cpu_form_data* forms_cpu; //0th element is the root
    gpu_form_data* forms_gpu;
    int n_forms;

    form_joint* joints;
    int n_joints;

    form_endpoint* endpoints;
    int n_endpoints;

    cell_type* cell_types;
    int n_cell_types;
};

enum gene_type
{
    gene_passive,
    gene_trigger,
    gene_active,
    n_gene_types
};

enum trigger_type
{
    trig_none,
    trig_always,
    trig_hot,
    trig_cold,
    trig_electric,
    trig_contact,
    n_trigger_types
};

enum action_type
{
    act_none,
    act_grow,
    act_die,
    act_heat,
    act_chill,
    act_electrify,
    act_explode,
    act_spray,
    n_actions
};

void reload_form_to_gpu(cpu_form_data* fc, gpu_form_data* fg);

typedef void (*passive_function)(material_visual_info*, material_physical_info*);

struct gene_data
{
    int type;
    char* name;

    char* sprite_filename;
    int_2 sprite_offset;
    int_2 sprite_size;

    int growth_time;
    int life_cost;

    passive_function passive_fun;
    int trigger_id;
    int action_id;
    int material_id;
};

void harden(material_visual_info* visual, material_physical_info* physical)
{
    physical->hardness += 0.1;
}

void conductify(material_visual_info* visual, material_physical_info* physical)
{
    physical->conductivity += 0.1;
    visual->metalicity += 0.1;
}

void densify(material_visual_info* visual, material_physical_info* physical)
{
    physical->density += 0.1;
}

void color_cell_red(material_visual_info* visual, material_physical_info* physical)
{
    visual->base_color -= {0,0.5,0.5};
}

void color_cell_green(material_visual_info* visual, material_physical_info* physical)
{
    visual->base_color -= {0.5,0,0.5};
}

void color_cell_blue(material_visual_info* visual, material_physical_info* physical)
{
    visual->base_color -= {0.5,0.5,0};
}

gene_data gene_list[] =
{
    {.type = gene_passive, .name = "Red Pigment", .sprite_filename = "ui/genes/Red.png",
     .growth_time = 0, .life_cost = 0, .passive_fun = color_cell_red},
    {.type = gene_passive, .name = "Green Pigment", .sprite_filename = "ui/genes/Green.png",
     .growth_time = 0, .life_cost = 0, .passive_fun = color_cell_green},
    {.type = gene_passive, .name = "Blue Pigment", .sprite_filename = "ui/genes/Blue.png",
     .growth_time = 0, .life_cost = 0, .passive_fun = color_cell_blue},
    {.type = gene_passive, .name = "Hardness", .sprite_filename = "ui/genes/Hardness.png",
     .growth_time = 20, .life_cost = 5, .passive_fun = harden},
    {.type = gene_passive, .name = "Conductivity", .sprite_filename = "ui/genes/Conductivity.png",
     .growth_time = 5, .life_cost = 1, .passive_fun = conductify},
    {.type = gene_passive, .name = "Density", .sprite_filename = "ui/genes/Density.png",
     .growth_time = 5, .life_cost = 1, .passive_fun = densify},
    {.type = gene_trigger, .name = "Auto Trigger", .sprite_filename = "ui/genes/trigger_always.png",
     .growth_time = 5, .life_cost = 2, .trigger_id = trig_always},
    {.type = gene_active, .name = "Die", .sprite_filename = "ui/genes/die.png",
     .growth_time = 0, .life_cost = 0, .action_id = act_die},
    {.type = gene_active, .name = "Grow Next", .sprite_filename = "ui/genes/grow.png",
     .growth_time = 0, .life_cost = 10, .action_id = act_grow},
};

//returns if the genode is held
bool do_genode(render_data* ui, user_input* input, genode* gn)
{
    real_2 mouse_r = gn->x - input->mouse;
    if(input->active_ui_element.type == 0 && is_pressed(M1, input) && normsq(mouse_r) < sq(genode_radius))
    {
        input->active_ui_element = {ui_gene, gn};
    }

    if((genode*) input->active_ui_element.element == gn)
    {
        if(!is_down(M1, input))
        {
            input->active_ui_element = {};
            return false;
        }

        gn->x = input->mouse;
    }

    real_4 genode_color = {0.1*gn->gene_id,1,1-0.1*gn->gene_id,1};
    draw_circle(ui, {gn->x.x, gn->x.y, 0}, genode_radius, genode_color);

    return ((genode*) input->active_ui_element.element == gn);
}

struct genedit_window
{
    bool open;
    real_2 x;
    real gene_scroll;

    real theta;
    real phi;
    real camera_dist;

    real_3x3 camera_axes;
    real_3 camera_pos;

    real cursor_dist;

    genome* active_genome;

    genode* genodes;
    int n_genodes;

    int active_form;
    int_3 active_cell;
    int_3 active_dir;

    real_3 extrude_counter;

    int drag_key;
    int active_material;
};

void compile_genome(genome* gn, genode* genodes)
{
    for(int c = 0; c < gn->n_cell_types; c++)
    {
        cell_type* cell = &gn->cell_types[c];
        *cell->visual = base_cell_visual;
        *cell->physical = base_cell_physical;
        for(int g = 0; g < cell->n_genes; g++)
        {
            cell->genes[g] = genodes[cell->genodes[g]].gene_id;
        }

        for(int g = 0; g < cell->n_genes; g++)
        {
            int gene_id = cell->genes[g];
            gene_data gene = gene_list[gene_id];

            cell->physical->growth_time += gene.growth_time;

            if(gene.passive_fun) gene.passive_fun(cell->visual, cell->physical);

            if(gene.trigger_id)
            {
                if(g+1 < cell->n_genes)
                {
                    gene_data next_gene = gene_list[cell->genes[g+1]];
                    if(next_gene.action_id)
                    {
                        uint trigger_info = gene.trigger_id
                            | (next_gene.action_id<<8)
                            | (next_gene.material_id<<16);
                        cell->physical->trigger_info[int(cell->physical->n_triggers++)] = real(trigger_info);
                    }
                }
            }
        }
    }

    //TODO: only need to reopload things that changed
    glBindTexture(GL_TEXTURE_2D, material_visual_properties_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, N_MAX_MATERIALS, GL_RGB, GL_FLOAT, material_visuals);

    glBindTexture(GL_TEXTURE_2D, material_physical_properties_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, N_PHYSICAL_PROPERTIES, N_MAX_MATERIALS, GL_RED, GL_FLOAT, material_physicals);
}

void drag_form(cpu_form_data* form_cpu, gpu_form_data* form_gpu, real_3 grab_point, real_3 target_x)
{
    real_3 r = apply_rotation(form_gpu->orientation,
                              grab_point
                              +(real_3){0.5, 0.5, 0.5}-form_gpu->x_cm);
    real_3 x = form_gpu->x+r;
    real_3 displacement = target_x-x;

    real I = 1;
    real m = 1;

    real_3x3 r_dual = {
        0   ,+r.z,-r.y,
        -r.z, 0  ,+r.x,
        +r.y,-r.x, 0
    };
    real_3x3 K = real_identity_3(1.0)/m - r_dual*(1.0/I)*r_dual;
    real_3x3 invK = inverse(K);

    real_3 pseudo_force = 0.05*invK*displacement;

    form_gpu->x += (1.0/m)*pseudo_force;
    real_3 pseudo_omega = (1.0/I)*cross(r, pseudo_force);
    form_gpu->orientation = axis_to_quaternion(pseudo_omega)*form_gpu->orientation;

    // real_3 force = 0.1*pseudo_force;
    // form_cpu->x_dot += (1.0/m)*force;
    // form_cpu->omega += (1.0/I)*cross(r, force);

    // form_gpu->x = norm(form_r)*dir+target_x;

    // // log_output("grab_point: ", grab_point, "\n");
    // // log_output("x_cm: ", form_gpu->x_cm, "\n");
    // // log_output("r: ", form_r, "\n");

    // real_3 old_dir = normalize_or_zero(form_r);

    // quaternion rotation = get_rotation_between(old_dir, dir);

    // form_gpu->orientation = rotation*form_gpu->orientation;
}

void solve_form_joints(genome* g)
{
    real_3 centroid = {};
    for(int f = 0; f < g->n_forms; f++)
    {
        cpu_form_data* form_cpu = &g->forms_cpu[f];
        gpu_form_data* form_gpu = &g->forms_gpu[f];

        centroid += form_gpu->x;
    }
    centroid /= g->n_forms;

    for(int f = 0; f < g->n_forms; f++)
    {
        cpu_form_data* form_cpu = &g->forms_cpu[f];
        gpu_form_data* form_gpu = &g->forms_gpu[f];

        for(int f2 = f+1; f2 < g->n_forms; f2++)
        {
            cpu_form_data* form_cpu2 = &g->forms_cpu[f2];
            gpu_form_data* form_gpu2 = &g->forms_gpu[f2];

            real_3 r = (form_gpu->x-form_gpu2->x);
            real_3 deltax = 0.01*r/pow(normsq(r), 1.5);

            // form_gpu->x  += deltax;
            // form_gpu2->x -= deltax;

            form_cpu->x_dot  += deltax;
            form_cpu2->x_dot -= deltax;
        }

        if(f == 0) form_cpu->x_dot.z += 0.2;
        else form_cpu->x_dot.z -= 0.005;
        // form_gpu->x += 0.01*(form_gpu->x-centroid);

        real_3 base_x = {};
        real_3 grab_pos = form_gpu->x_cm;

        real center_threshold = 0.5;

        // if(abs(form_gpu->x.y) < center_threshold)
        // {
        //     real_3 axis = form_gpu->x-base_x;
        //     real_3 forward_dir = apply_rotation(form_gpu->orientation, (real_3){0,1,0});
        //     real_3 centered_dir = {1,0,0};
        //     if(normsq(axis) > 0.0001)
        //     {
        //         forward_dir = normalize_or_zero(rej(forward_dir, axis));
        //         centered_dir = normalize_or_zero(rej(centered_dir, axis));
        //     }

        //     quaternion rotation = get_rotation_between(forward_dir, centered_dir);
        //     real t = abs(form_gpu->x.y);
        //     rotation = (quaternion){1.0-t,0,0,0} + t*rotation;
        //     rotation = normalize(rotation);
        //     assert(rotation.r == rotation.r && rotation.i == rotation.i && rotation.j == rotation.j && rotation.k == rotation.k);
        //     form_gpu->orientation = rotation*form_gpu->orientation;
        // }

        form_cpu->x_dot *= 0.8;
        form_cpu->omega *= 0.8;

        form_gpu->x += form_cpu->x_dot;
        form_gpu->orientation = axis_to_quaternion(form_cpu->omega)*form_gpu->orientation;

        form_gpu->orientation = normalize(form_gpu->orientation);
    }

    for(int k = 0; k < 40; k++)
    {
        real_3 foot_centroid;
        int n_feet = 0;
        for(int e = 0; e < g->n_endpoints; e++)
        {
            form_endpoint* ep = &g->endpoints[e];
            int f = ep->form_id;
            cpu_form_data* form_cpu = &g->forms_cpu[f];
            gpu_form_data* form_gpu = &g->forms_gpu[f];

            if(ep->type == endpoint_foot)
            {
                real_3 foot_r = apply_rotation(form_gpu->orientation,
                                               real_cast(ep->pos)
                                               +(real_3){0.5, 0.5, 0.5}-form_gpu->x_cm);
                real_3 foot_x = form_gpu->x+foot_r;
                foot_centroid += foot_x;
                n_feet++;
            }
        }
        foot_centroid /= n_feet;

        for(int e = 0; e < g->n_endpoints; e++)
        {
            form_endpoint* ep = &g->endpoints[e];
            int f = ep->form_id;
            cpu_form_data* form_cpu = &g->forms_cpu[f];
            gpu_form_data* form_gpu = &g->forms_gpu[f];

            if(ep->type == endpoint_foot)
            {
                real_3 foot_r = apply_rotation(form_gpu->orientation,
                                               real_cast(ep->pos)
                                               +(real_3){0.5, 0.5, 0.5}-form_gpu->x_cm);
                real_3 new_foot_x = form_gpu->x+foot_r;
                new_foot_x = lerp(new_foot_x, ((real_3){1.01f*(new_foot_x.x-foot_centroid.x),1.00f*(new_foot_x.y-foot_centroid.y),0}), 0.5);
                drag_form(form_cpu, form_gpu, real_cast(ep->pos), new_foot_x);
            }
        }

        for(int j = 0; j < g->n_joints; j++)
        {
            form_joint* joint = &g->joints[j];

            int f[] = {joint->form_id[0], joint->form_id[1]};
            cpu_form_data* forms_cpu[] = {&g->forms_cpu[f[0]],&g->forms_cpu[f[1]]};
            gpu_form_data* forms_gpu[] = {&g->forms_gpu[f[0]],&g->forms_gpu[f[1]]};
            real_3 joint_r[2];
            real_3 joint_x[2];
            for(int i = 0; i < 2; i++)
            {
                joint_r[i] = apply_rotation(forms_gpu[i]->orientation, real_cast(joint->pos[i])+(real_3){0.5,0.5,0.5}-forms_gpu[i]->x_cm);
                joint_x[i] = forms_gpu[i]->x+joint_r[i];
            }
            for(int i = 0; i < 2; i++)
            {
                // if(f[i] == 0) continue;
                // forms_gpu[i]->x += 0.02*(forms_gpu[i]->x-forms_gpu[1-i]->x);
                // forms_gpu[i]->x += 0.0001*(forms_gpu[i]->x);
                drag_form(forms_cpu[i], forms_gpu[i], real_cast(joint->pos[i]), lerp(joint_x[i], joint_x[1-i], 0.5));
            }
        }
    }
};

bool set_form_cell(cpu_form_data* form_cpu, gpu_form_data* form_gpu, int_3 pos, int material)
{
    if(pos.x >= 0 && pos.y >= 0 && pos.z >= 0
       && pos.x < form_gpu->size.x && pos.y < form_gpu->size.y && pos.z < form_gpu->size.z)
    {
        int index = pos.x+form_gpu->size.x*(pos.y+form_gpu->size.y*pos.z);
        int old_material = form_cpu->materials[index];
        form_cpu->materials[index] = material;
        return true;
    }
    return false;
}

void do_edit_window(render_data* ui, user_input* input, genedit_window* gew)
{
    //TODO: hotkeys, shift-click
    for(int i = 0; i < len(gene_list); i++)
    {
        gene_data* g = &gene_list[i];
        real_2 x = gew->x + (real_2){0.2, -0.2};
        x.x += i*genode_spacing;
        real_4 genode_color = {0.1*i,1,1-0.1*i,1};
        draw_circle(ui, {x.x, x.y, 0}, genode_radius, genode_color);
        if(is_pressed(M1, input) && normsq(x-input->mouse) < sq(genode_radius))
        {
            genode* new_genode = &gew->genodes[gew->n_genodes++];
            *new_genode = {
                .gene_id = i,
                .x = input->mouse,
                .theta = 0,
            };

            input->active_ui_element = {ui_gene, new_genode};
        }
    }

    if(gew->active_genome)
        for(int c = 0; c < gew->active_genome->n_cell_types; c++)
        {
            cell_type* cell = &gew->active_genome->cell_types[c];
            real_2 x_anchor = gew->x + (real_2){0.2, -0.2-1.5*gene_row_spacing};
            x_anchor.y += -c*gene_row_spacing;

            bool inserting = false;

            for(int l = 0; l < cell->n_genes+1; l++)
            {
                int genode_id = cell->genodes[l];
                genode* gn = &gew->genodes[genode_id];

                real_2 mouse_r = input->mouse - x_anchor;

                if(input->active_ui_element.type == ui_gene && normsq(mouse_r) < sq(1.5*genode_radius))
                {
                    genode* held_gn = (genode*) input->active_ui_element.element;
                    int held_gn_id = held_gn-gew->genodes;
                    if(cell->n_genes < N_MAX_GENES)
                    {
                        if(!is_down(M1, input))
                        {
                            memmove(cell->genodes+l+1, cell->genodes+l, (cell->n_genes-l)*sizeof(int));
                            cell->genodes[l] = held_gn_id;
                            held_gn->root_cell = cell;
                            cell->n_genes++;
                            input->active_ui_element = {};
                            compile_genome(gew->active_genome, gew->genodes);
                        }
                        else
                        {
                            draw_circle(ui, {x_anchor.x, x_anchor.y, 0}, genode_radius, {0.8, 0.8, 0.8, 0.5});
                            x_anchor.x += genode_spacing;
                            inserting = true;
                        }
                    }
                }

                genode_id = cell->genodes[l];
                gn = &gew->genodes[genode_id];

                if(l < cell->n_genes)
                {
                    real_2 r = x_anchor - gn->x;
                    if(normsq(r) > sq(0.01)) gn->x = lerp(gn->x, x_anchor, 0.3);
                    else gn->x = x_anchor;
                }
                else if(!inserting && cell->n_genes < N_MAX_GENES)
                {
                    draw_circle(ui, {x_anchor.x, x_anchor.y, 0}, genode_radius/4, {0.8, 0.8, 0.8, 1.0});
                }

                x_anchor.x += genode_spacing;
            }
        }

    for(int g = gew->n_genodes-1; g >= 0; g--)
    {
        genode* gn = &gew->genodes[g];
        bool held = do_genode(ui, input, gn);

        if(held && gn->root_cell)
        {
            for(int slot = 0; slot < gn->root_cell->n_genes; slot++)
                if(gn->root_cell->genodes[slot] == g)
                {
                    memmove(gn->root_cell->genodes+slot, gn->root_cell->genodes+slot+1,
                            (gn->root_cell->n_genes-slot)*sizeof(int));
                    gn->root_cell->n_genes--;
                    gn->root_cell = 0;

                    compile_genome(gew->active_genome, gew->genodes);
                    break;
                }
        }

        if(!held && !gn->root_cell)
        { //delete the genode
            int last_genode = --gew->n_genodes;
            *gn = gew->genodes[last_genode];
            if(gn->root_cell)
            {
                for(int slot = 0; slot < gn->root_cell->n_genes; slot++)
                    if(gn->root_cell->genodes[slot] == last_genode)
                    {
                        gn->root_cell->genodes[slot] = g;
                        break;
                    }
            }
        }
    }

    if(is_down(M3, input) && input->active_ui_element.type == 0)
    {
        gew->theta += 0.8*input->dmouse.y;
        gew->phi -= 0.8*input->dmouse.x;
        gew->theta = clamp(gew->theta, 0.0f, pi);
    }
    if(input->active_ui_element.type == 0) gew->camera_dist *= pow(1.0+0.001, -input->mouse_wheel);

    real_3 dir = {cos(gew->phi)*sin(gew->theta), sin(gew->phi)*sin(gew->theta), cos(gew->theta)};
    real_3 side_dir    = {-sin(gew->phi), cos(gew->phi), 0};
    gew->camera_pos = dir;
    gew->camera_pos *= gew->camera_dist;
    gew->camera_axes[2] = dir;
    gew->camera_axes[0] = side_dir;
    gew->camera_axes[1] = cross(gew->camera_axes[2], gew->camera_axes[0]);


    ray_hit hit = {};
    cpu_form_data* hit_form_cpu;
    for(int f = 0; f < gew->active_genome->n_forms; f++)
    {
        gpu_form_data* form_gpu = &gew->active_genome->forms_gpu[f];
        cpu_form_data* form_cpu = &gew->active_genome->forms_cpu[f];

        form_gpu->is_mutating = false;

        float fov = pi*120.0/180.0;
        float screen_dist = 1.0/tan(fov/2);
        real_3 ray_dir = (+input->mouse.x*gew->camera_axes[0]
                          +input->mouse.y*gew->camera_axes[1]
                          -screen_dist   *gew->camera_axes[2]);
        ray_dir = normalize(ray_dir);

        real_3 ray_pos = gew->camera_pos;

        ray_dir = apply_rotation(conjugate(form_gpu->orientation), ray_dir);
        ray_pos = apply_rotation(conjugate(form_gpu->orientation), ray_pos - form_gpu->x) + form_gpu->x_cm;
        ray_hit new_hit = cast_ray(form_cpu->materials, ray_dir, ray_pos, form_gpu->size, {}, 100);

        if(new_hit.hit && (!hit.hit || new_hit.dist < hit.dist))
        {
            hit = new_hit;
            hit_form_cpu = form_cpu;
        }
    }

    ui->log_pos += sprintf(ui->debug_log+ui->log_pos, "ray cast: %s, (%d, %d, %d), %f, f=%d\n",
                           hit.hit ? "hit" : "no hit",
                           hit.pos.x, hit.pos.y, hit.pos.z,
                           hit.dist,
                           (int)(hit_form_cpu-gew->active_genome->forms_cpu));
    if(input->active_ui_element.type == 0)
    {
        if(hit.hit)
        {
            gew->active_form = hit_form_cpu-gew->active_genome->forms_cpu;
            gew->active_cell = hit.pos;
            gew->active_dir = hit.dir;
            gew->cursor_dist = hit.dist;

            if(is_pressed('G', input))
                input->active_ui_element = {ui_form, hit_form_cpu};
            if(is_pressed(M1, input))
            {
                input->active_ui_element = {ui_form_voxel, hit_form_cpu};
                gew->extrude_counter = {};
                gew->drag_key = M1;
                gew->active_material = 128;
            }
            if(is_pressed(M2, input))
            {
                input->active_ui_element = {ui_form_voxel, hit_form_cpu};
                gew->extrude_counter = {};
                gew->drag_key = M2;
                gew->active_material = 0;

                if(set_form_cell(hit_form_cpu, hit_form_cpu->gpu_data, hit.pos, gew->active_material))
                {
                    hit_form_cpu->gpu_data->is_mutating = true;
                    reload_form_to_gpu(hit_form_cpu, hit_form_cpu->gpu_data);
                }
            }
        }
        else
        {
            gew->active_form = -1;
        }
    }

    if(input->active_ui_element.type == ui_form_voxel)
    {

        if(!is_down(gew->drag_key, input)) input->active_ui_element = {};
        else
        {
            bool extruded = false;

            gpu_form_data* form_gpu = &gew->active_genome->forms_gpu[gew->active_form];
            cpu_form_data* form_cpu = &gew->active_genome->forms_cpu[gew->active_form];

            float fov = pi*120.0/180.0;
            float screen_dist = 1.0/tan(fov/2);

            //the position of the cell in camera coordinates
            real_3 cell_x =
                apply_rotation(form_gpu->orientation, real_cast(gew->active_cell)
                               +(real_3){0.5, 0.5, 0.5}-form_gpu->x_cm) + form_gpu->x - gew->camera_pos;
            //camera axes should be an orthogonal matrix, so this is the inverse
            cell_x = transpose(gew->camera_axes)*cell_x;

            real_3x3 cell_axes = apply_rotation(form_gpu->orientation, real_identity_3(1));
            cell_axes = transpose(gew->camera_axes)*cell_axes;

            int extrude_dir = 0;
            real max_extrude_counter = 0;
            int extrude_sign = 0;
            real_2 voxel_screen_pos = -(screen_dist/cell_x.z)*cell_x.xy;
            input->mouse = voxel_screen_pos;
            //TODO: disable the axis going more into the screen when axes overlap

            real_2 axis[3] = {};

            for(int i = 0; i < 3; i++)
            {
                real_3 dir = cell_axes[i];
                axis[i] = {
                    (dir.x*screen_dist)/cell_x.z-(dir.z*cell_x.x*screen_dist)/sq(cell_x.z),
                    (dir.y*screen_dist)/cell_x.z-(dir.z*cell_x.y*screen_dist)/sq(cell_x.z)
                };

                axis[i] = 40.0f*normalize(axis[i]);
            }
            for(int i = 0; i < 3; i++)
            {
                for(int j = i+1; j < 3; j++)
                {
                    real overlap = abs(dot(normalize(axis[i]), normalize(axis[j])));
                    real overlap_threshold = 0.7;
                    if(overlap > overlap_threshold)
                    {
                        real z1 = abs(dot(cell_axes[i], normalize(cell_x)));
                        real z2 = abs(dot(cell_axes[j], normalize(cell_x)));
                        if(z1 > z2)
                        {
                            axis[i] /= 1.0f+(overlap-overlap_threshold)/(1.0f-overlap_threshold);
                        }
                        else
                        {
                            axis[j] /= 1.0f+(overlap-overlap_threshold)/(1.0f-overlap_threshold);
                        }
                    }
                }
            }

            for(int i = 0; i < 3; i++)
            {
                ui->log_pos += sprintf(ui->debug_log+ui->log_pos, "axis %d: (%f, %f)\n",
                                       i, axis[i].x, axis[i].y);
                draw_circle(ui, pad_3(voxel_screen_pos+0.05*normalize(axis[i])), 0.01, {i==0, i==1, i==2, 1});
                draw_circle(ui, pad_3(voxel_screen_pos-0.05*normalize(axis[i])), 0.01, {i==0, i==1, i==2, 1});
                // draw_circle(ui, pad_3(voxel_screen_pos-0.05*normalize(axis[i])*gew->extrude_counter[i]), 0.01, {i==0, i==1, i==2, 1});

                gew->extrude_counter[i] += -0.5f*dot(input->dmouse, axis[i]);
                if(abs(gew->extrude_counter[i]) > max_extrude_counter)
                {
                    max_extrude_counter = abs(gew->extrude_counter[i]);
                    extrude_dir = i;
                    extrude_sign = sign(gew->extrude_counter[i]);
                }
            }

            // real_3 extrude_x =
            //     apply_rotation(form_gpu->orientation, real_cast(gew->active_cell)
            //                    +(real_3){0.5, 0.5, 0.5}-form_gpu->x_cm+gew->extrude_counter) + form_gpu->x - gew->camera_pos;
            // extrude_x = transpose(gew->camera_axes)*extrude_x;
            // real_2 extrude_screen_pos = -(screen_dist/extrude_x.z)*extrude_x.xy;
            // draw_circle(ui, pad_3(extrude_screen_pos), 0.01, {1,1,1,1});

            for(; max_extrude_counter > 1; max_extrude_counter -= 1)
            {
                gew->extrude_counter = {};

                int_3 pos = gew->active_cell;
                pos[extrude_dir] += extrude_sign;
                if(set_form_cell(form_cpu, form_gpu, pos, gew->active_material))
                {
                    gew->active_cell = pos;
                    form_gpu->is_mutating = true;
                    reload_form_to_gpu(form_cpu, form_gpu);
                }
                else break;
            }

            ui->log_pos += sprintf(ui->debug_log+ui->log_pos, "active_cell: (%d, %d, %d)\n",
                                   gew->active_cell.x, gew->active_cell.y, gew->active_cell.z);

            ui->log_pos += sprintf(ui->debug_log+ui->log_pos, "extrude_counter: (%f, %f, %f)\n",
                                   gew->extrude_counter.x, gew->extrude_counter.y, gew->extrude_counter.z);
        }
    }

    if(input->active_ui_element.type == ui_form)
    {
        if(!is_down('G', input)) input->active_ui_element = {};
        else
        {
            int f = gew->active_form;

            gpu_form_data* form_gpu = &gew->active_genome->forms_gpu[f];
            cpu_form_data* form_cpu = &gew->active_genome->forms_cpu[f];

            float fov = pi*120.0/180.0;
            float screen_dist = 1.0/tan(fov/2);
            real_3 ray_dir = (+input->mouse.x*gew->camera_axes[0]
                              +input->mouse.y*gew->camera_axes[1]
                              -screen_dist   *gew->camera_axes[2]);
            ray_dir = normalize(ray_dir);

            gew->cursor_dist += 0.01*input->mouse_wheel;
            real_3 cursor_x = gew->camera_pos+gew->cursor_dist*ray_dir;
            real_3 grab_point = real_cast(gew->active_cell)+(real_3){0.5,0.5,0.5};
            drag_form(form_cpu, form_gpu, grab_point, cursor_x);
        }
    }

    solve_form_joints(gew->active_genome);
}

#endif //GENETICS
