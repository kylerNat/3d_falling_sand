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

#pragma pack(push, 1)
struct gpu_form_data
{
    int_3 materials_origin;
    int_3 size;

    real_3 x_cm;

    real_3 x;
    quaternion orientation;

    int cell_material_id;
};
#pragma pack(pop)

struct cpu_form_data
{
    gpu_form_data* gpu_data;

    uint8* materials;
    int storage_level;

    int parent_form_id;
    int joint_type;
    int_3 joint_pos[2];
    int joint_axis[2];
};

struct genome
{
    cpu_form_data* forms_cpu; //0th element is the root
    gpu_form_data* forms_gpu;
    int n_forms;

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

    int replication_time;
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
     .replication_time = 0, .life_cost = 0, .passive_fun = color_cell_red},
    {.type = gene_passive, .name = "Green Pigment", .sprite_filename = "ui/genes/Green.png",
     .replication_time = 0, .life_cost = 0, .passive_fun = color_cell_green},
    {.type = gene_passive, .name = "Blue Pigment", .sprite_filename = "ui/genes/Blue.png",
     .replication_time = 0, .life_cost = 0, .passive_fun = color_cell_blue},
    {.type = gene_passive, .name = "Hardness", .sprite_filename = "ui/genes/Hardness.png",
     .replication_time = 2, .life_cost = 5, .passive_fun = harden},
    {.type = gene_passive, .name = "Conductivity", .sprite_filename = "ui/genes/Conductivity.png",
     .replication_time = 1, .life_cost = 1, .passive_fun = conductify},
    {.type = gene_passive, .name = "Density", .sprite_filename = "ui/genes/Density.png",
     .replication_time = 1, .life_cost = 1, .passive_fun = densify},
    {.type = gene_trigger, .name = "Auto Trigger", .sprite_filename = "ui/genes/trigger_always.png",
     .replication_time = 1, .life_cost = 2, .trigger_id = trig_always},
    {.type = gene_active, .name = "Die", .sprite_filename = "ui/genes/die.png",
     .replication_time = 0, .life_cost = 0, .action_id = act_die},
    {.type = gene_active, .name = "Grow Next", .sprite_filename = "ui/genes/grow.png",
     .replication_time = 0, .life_cost = 10, .action_id = act_grow},
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

    real_2 drag_origin;
    int_3 old_dir;

    real extrusion_counter;
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

void drag_form(gpu_form_data* form_gpu, real_3 grab_point, real_3 target_x)
{
    real_3 form_r = apply_rotation(form_gpu->orientation,
                                   grab_point
                                   +(real_3){0.5, 0.5, 0.5}-form_gpu->x_cm);

    real_3 dir = normalize(form_gpu->x-target_x);

    form_gpu->x = norm(form_r)*dir+target_x;

    real_3 old_dir = normalize(form_r);

    real_3 sine = cross(old_dir, dir);
    real cosine = sqrt(1.0f-normsq(sine));
    real_3 sin_half = sqrt(0.5f*(1-cosine))*normalize(sine);
    real cos_half = sqrt(0.5f*(1+cosine));

    quaternion rotation = {cos_half, -sin_half.x, -sin_half.y, -sin_half.z};

    form_gpu->orientation = rotation*form_gpu->orientation;
}

void solve_form_joints(genome* g)
{
    for(int f = 0; f < g->n_forms; f++)
    {
        cpu_form_data* form_cpu = &g->forms_cpu[f];
        gpu_form_data* form_gpu = &g->forms_gpu[f];

        if(form_cpu->joint_type == joint_root) continue;

        cpu_form_data* parent_cpu = &g->forms_cpu[form_cpu->parent_form_id];
        gpu_form_data* parent_gpu = &g->forms_gpu[form_cpu->parent_form_id];

        real_3 parent_r = apply_rotation(parent_gpu->orientation,
                                         real_cast(form_cpu->joint_pos[0])
                                         +(real_3){0.5, 0.5, 0.5}-parent_gpu->x_cm);
        real_3 base_x = parent_gpu->x + parent_r;

        // form_gpu->x += 0.05*(form_gpu->x-parent_gpu->x);
        // form_gpu->x.z -= 0.2;

        drag_form(form_gpu, real_cast(form_cpu->joint_pos[1]), base_x);
    }
};

bool set_form_cell(cpu_form_data* form_cpu, gpu_form_data* form_gpu, int_3 pos, int material)
{
    if(pos.x >= 0 && pos.y >= 0 && pos.z >= 0
       && pos.x < form_gpu->size.x && pos.y < form_gpu->size.y && pos.z < form_gpu->size.z)
    {
        int index = pos.x+form_gpu->size.x*(pos.y+form_gpu->size.y*pos.z);
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
                gew->extrusion_counter = 0;
                gew->drag_origin = input->mouse;
            }
            if(is_pressed(M2, input))
            {
                int_3 add_pos = hit.pos;

                gpu_form_data* form_gpu = hit_form_cpu->gpu_data;

                hit_form_cpu->materials[add_pos.x+form_gpu->size.x*(add_pos.y+form_gpu->size.y*add_pos.z)] = 0;
                reload_form_to_gpu(hit_form_cpu, form_gpu);
            }
        }
        else
        {
            gew->active_form = -1;
        }
    }

    if(input->active_ui_element.type == ui_form_voxel)
    {

        if(!is_down(M1, input)) input->active_ui_element = {};
        else
        {
            bool extruded = false;

            gpu_form_data* form_gpu = &gew->active_genome->forms_gpu[gew->active_form];
            cpu_form_data* form_cpu = &gew->active_genome->forms_cpu[gew->active_form];

            //the position of the cell in camera coordinates
            real_3 cell_x =
                apply_rotation(form_gpu->orientation, real_cast(gew->active_cell)
                               +(real_3){0.5, 0.5, 0.5}-form_gpu->x_cm) + form_gpu->x - gew->camera_pos;
            //camera axes should be an orthogonal matrix, so this is the inverse
            cell_x = transpose(gew->camera_axes)*cell_x;

            float fov = pi*120.0/180.0;
            float screen_dist = 1.0/tan(fov/2);

            real_3x3 cell_axes = apply_rotation(form_gpu->orientation, real_identity_3(1));
            cell_axes = transpose(gew->camera_axes)*cell_axes;
            int_3 extrude_dir = {};
            real axis_dist = 0;
            real axis_proj = 0;
            for(int i = 0; i < 3; i++)
            {
                real_3 dir = cell_axes[i];
                real_2 axis = {
                    (dir.x*screen_dist)/cell_x.z+(dir.z*cell_x.x*screen_dist)/sq(cell_x.z),
                    (dir.y*screen_dist)/cell_x.z+(dir.z*cell_x.y*screen_dist)/sq(cell_x.z)
                };
                if(abs(dot(cell_axes[i], normalize(cell_x))) > 0.8) continue;
                axis = axis/normsq(axis);

                // real new_axis_dist = abs(dot(input->mouse-gew->drag_origin, normalize(axis)));
                real new_axis_dist = abs(dot(input->dmouse, normalize(axis)));
                if(new_axis_dist > axis_dist)
                {
                    extrude_dir = {};
                    axis_dist = new_axis_dist;
                    // axis_proj = dot(input->mouse-gew->drag_origin, axis);
                    real proj = dot(input->dmouse, axis);
                    axis_proj = 0.5*abs(proj);
                    extrude_dir[i] = -sign(proj);
                }
            }

            gew->extrusion_counter += axis_proj;

            // gew->extrusion_counter += 0.5*dot(input->dmouse, inv_extrude_proj);
            // gew->extrusion_counter += -0.01*input->mouse_wheel;

            gew->extrusion_counter = clamp(gew->extrusion_counter, -8.0f, 8.0f);

            // ui->log_pos += sprintf(ui->debug_log+ui->log_pos, "inv_extrude_proj: (%f, %f)\n",
            //                        inv_extrude_proj.x, inv_extrude_proj.y);

            ui->log_pos += sprintf(ui->debug_log+ui->log_pos, "extrusion_counter: %f\n", gew->extrusion_counter);

            ui->log_pos += sprintf(ui->debug_log+ui->log_pos, "active_cell: (%d, %d, %d)\n",
                                   gew->active_cell.x, gew->active_cell.y, gew->active_cell.z);

            ui->log_pos += sprintf(ui->debug_log+ui->log_pos, "extrude_dir: (%d, %d, %d)\n",
                                   extrude_dir.x, extrude_dir.y, extrude_dir.z);

            while(gew->extrusion_counter >= 1)
            {
                int_3 add_pos = gew->active_cell+extrude_dir;

                if(set_form_cell(form_cpu, form_gpu, add_pos, 128))
                {
                    gew->active_cell += extrude_dir;

                    extruded = true;
                    gew->extrusion_counter -= 1;
                }
                else break;
            }

            if(extruded)
                reload_form_to_gpu(form_cpu, form_gpu);
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
            drag_form(form_gpu, grab_point, cursor_x);
        }
    }

    solve_form_joints(gew->active_genome);
}

#endif //GENETICS
