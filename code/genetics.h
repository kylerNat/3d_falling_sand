#ifndef GENETICS
#define GENETICS

#include "graphics_common.h"
#include "materials.h"
#include "cuboid_packer.h"

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
    bounding_box material_bounds;

    int_3 x_origin;

    real_3 x;
    quaternion orientation;

    int cell_material_id;
    int is_mutating;
};
#pragma pack(pop)

struct cpu_form_data
{
    gpu_form_data* gpu_data;

    bool is_root;

    union
    {
        struct
        {
            int8 xi;
            int8 psi;
            int8 theta;
            int8 phi;
        };
        struct
        {
            int8_2 mn;
            int8_2 thetaphi;
        };
        uint32 int_orientation;
    };

    //extra angle added for visual effects
    real xi_visual;
    real psi_visual;
    real theta_visual;
    real phi_visual;

    uint8* materials;
    int_3 storage_size;
};

// bool allocate_storage(cubiod_space* form_space, cpu_form_data* form_list_cpu, gpu_form_data* form_list_gpu, int n_forms, int f, int_3 size, bool defrag)
// {
//     cpu_form_data* form_cpu = form_list_cpu+f;
//     gpu_form_data* form_gpu = form_list_gpu+f;

//     form_cpu->storage_box = add_region(form_space, size);
//     if(form_cpu->storage_box.u == (int_3){0,0,0})
//     { //allocation failed
//         if(defrag) //defragment and try again
//         {
//             form_space->free_regions[0] = form_space->max_size;
//             form_space->n_free_regions = 1;

//             for(int o = 0; o < n_forms; o++)
//             {
//                 cpu_form_data* orm_cpu = form_list_cpu+o;
//                 gpu_form_data* orm_gpu = form_list_gpu+o;
//                 int_3 orm_size = orm_cpu->storage_box.u-orm_cpu->storage_box.l;
//                 orm_cpu->storage_box = add_region(form_space, orm_size);
//                 if(orm_cpu->storage_box.u == (int_3){0,0,0})
//                 {
//                     assert(true, "somehow made things worse while defragging\n");
//                     return false;
//                 }
//             }

//             form_cpu->storage_box = add_region(form_space, size);
//             if(form_cpu->storage_box.u == (int_3){0,0,0})
//             {
//                 return false;
//             }
//         }
//         else return false;
//     }
//     return true;
// }

quaternion convert_orientation(cpu_form_data* form_cpu)
{
    quaternion orientation = {1,0,0,0};

    real inc = pi/4.0; //angular increment

    real xi = form_cpu->xi*inc+form_cpu->xi_visual;
    real psi = form_cpu->psi*inc+form_cpu->psi_visual;
    real theta = form_cpu->theta*inc+form_cpu->theta_visual;
    real phi = form_cpu->phi*inc+form_cpu->phi_visual;

    orientation = axis_to_quaternion((real_3){0,0,xi})*orientation;
    orientation = axis_to_quaternion((real_3){psi,0,0})*orientation;
    orientation = axis_to_quaternion((real_3){0,theta,0})*orientation;
    orientation = axis_to_quaternion((real_3){0,0,phi})*orientation;
    return orientation;
}

struct genome
{
    int id;

    int* form_id_updates;
    cpu_form_data* forms_cpu; //0th element is the root
    gpu_form_data* forms_gpu;
    int n_forms;

    form_joint* joints;
    int n_joints;

    form_endpoint* endpoints;
    int n_endpoints;

    cell_type* cell_types;
    int n_cell_types;

    bool is_mutating;
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

void load_form_to_gpu(cuboid_space* form_space, cpu_form_data* fc, gpu_form_data* fg);
void reload_form_to_gpu(cpu_form_data* fc, gpu_form_data* fg);
void resize_form_storage(memory_manager* manager, cuboid_space* form_space, cpu_form_data* fc, gpu_form_data* fg, int_3 offset, int_3 new_size);

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

void delete_form(genome* g, int form_id)
{
    g->forms_cpu[form_id] = g->forms_cpu[--g->n_forms];
    g->forms_gpu[form_id] = g->forms_gpu[  g->n_forms];
    g->forms_cpu->gpu_data = &g->forms_gpu[form_id];
    g->form_id_updates[g->n_forms] = form_id;
    g->form_id_updates[form_id] = -1;
}

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

struct rotation_gizmo
{
    real_3 x;
    real r;
    real_3 normal;
    real_4 color;
    real_3 last_dir;
    real angle;
    int initial_angle;
};

real_3 project_to_camera(real_3 x, real_3 camera_pos, real_3x3 camera_axes)
{
    float fov = pi*120.0/180.0;
    float screen_dist = 1.0/tan(fov/2);

    real_3 r = x-camera_pos;
    real_3 screen_pos = {dot(r, camera_axes[0]), dot(r, camera_axes[1]), 0};
    screen_pos *= -(screen_dist/dot(r, camera_axes[2]));
    return screen_pos;
}

bool do_rotation_gizmo(render_data* ui, user_input* input, rotation_gizmo* gizmo, real_3 mouse_pos, real_3 mouse_ray, real_3x3 camera_axes)
{
    draw_circle(ui, project_to_camera(gizmo->x, mouse_pos, camera_axes), 0.01, gizmo->color);
    const int subdivs = 32;
    for(int i = 0; i < subdivs; i++)
    {
        real_3 x_hat = normalize(cross(gizmo->normal, {1,1,1}));
        real_3 y_hat = cross(gizmo->normal, x_hat);
        real theta = 2.0*pi*i/subdivs;
        real_3 a = gizmo->r*(cos(theta)*x_hat+sin(theta)*y_hat);
        draw_circle(ui, project_to_camera(gizmo->x+a, mouse_pos, camera_axes), 0.01, gizmo->color);
    }

    real dist = dot((gizmo->x-mouse_pos), gizmo->normal)/(dot(mouse_ray, gizmo->normal));
    if(dist < 0) return false;

    real_3 mouse_intersection = mouse_pos + mouse_ray*dist;
    real_3 r = mouse_intersection-gizmo->x;
    real norm_r = norm(r);
    real_3 rdir = r/norm_r;
    if(0.95*gizmo->r < norm_r && norm_r < 1.05*gizmo->r && input->active_ui_element.type == ui_none)
    {
        if(is_down(M1, input))
        {
            input->active_ui_element = {ui_gizmo, gizmo};
            gizmo->last_dir = rdir;
            gizmo->angle = 0;
        }

        draw_circle(ui, project_to_camera(gizmo->x+gizmo->r*rdir, mouse_pos, camera_axes), 0.015, gizmo->color);
    }

    real angle = asin(dot(cross(gizmo->last_dir, rdir), gizmo->normal));

    if(input->active_ui_element.element == gizmo)
    {
        if(!is_down(M1, input))
        {
            input->active_ui_element = {};
            return false;
        }

        gizmo->angle += angle;

        draw_circle(ui, project_to_camera(gizmo->x+gizmo->r*rdir, mouse_pos, camera_axes), 0.02, gizmo->color);

        gizmo->last_dir = rdir;
        return true;
    }
    return false;
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

    int selected_form;

    rotation_gizmo gizmo_x;
    rotation_gizmo gizmo_y;
    rotation_gizmo gizmo_z;

    real_3 extrude_counter;

    int drag_key;
    int active_cell_material;
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

void solve_form_joints(genome* g)
{
    real_3 centroid = {};
    for(int f = 0; f < g->n_forms; f++)
    {
        cpu_form_data* form_cpu = &g->forms_cpu[f];
        gpu_form_data* form_gpu = &g->forms_gpu[f];

        centroid += form_gpu->x;

        form_gpu->orientation = convert_orientation(form_cpu);
    }
    centroid /= g->n_forms;

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
            joint_r[i] = apply_rotation(forms_gpu[i]->orientation, real_cast(joint->pos[i])+(real_3){0.5,0.5,0.5});
            joint_x[i] = forms_gpu[i]->x+joint_r[i];
        }

        real_3 joint_center = 0.5*(joint_x[0]+joint_x[1]);

        for(int i = 0; i < 2; i++)
        {
            forms_gpu[i]->x += joint_center-joint_x[i];
        }
    }
};

bool set_form_cell(memory_manager* manager, cuboid_space* form_space, genome* g, int form_id, int_3* pos, int material)
{
    cpu_form_data* form_cpu = &g->forms_cpu[form_id];
    gpu_form_data* form_gpu = &g->forms_gpu[form_id];
    if(material == 0 && form_gpu->material_bounds.u-form_gpu->material_bounds.l == (int_3){1,1,1} && *pos == form_gpu->material_bounds.l)
    {
        delete_form(g, form_id);
        return false;
    }
    if(!(pos->x >= 0 && pos->y >= 0 && pos->z >= 0
         && pos->x < form_cpu->storage_size.x && pos->y < form_cpu->storage_size.y && pos->z < form_cpu->storage_size.z))
    {
        if(material == 0)
        {
            return false;
        }
        bounding_box new_bounding_box = {{0,0,0}, form_cpu->storage_size};
        new_bounding_box = expand_to(new_bounding_box, *pos);
        // if((new_bounding_box.u.x-new_bounding_box.l.x)%2) new_bounding_box.u.x++;
        // if((new_bounding_box.u.y-new_bounding_box.l.y)%2) new_bounding_box.u.y++;
        // if((new_bounding_box.u.z-new_bounding_box.l.z)%2) new_bounding_box.u.z++;
        new_bounding_box.u.x+=3-(new_bounding_box.u.x-new_bounding_box.l.x-1)%4;
        // new_bounding_box.u.y+=3-(new_bounding_box.u.y-new_bounding_box.l.y-1)%4;
        // new_bounding_box.u.z+=3-(new_bounding_box.u.z-new_bounding_box.l.z-1)%4;
        int_3 new_size = new_bounding_box.u-new_bounding_box.l;
        resize_form_storage(manager, form_space, form_cpu, form_gpu, -new_bounding_box.l, new_size);
        *pos -= new_bounding_box.l;
        form_gpu->x_origin -= new_bounding_box.l;
    }

    int index = pos->x+form_cpu->storage_size.x*(pos->y+form_cpu->storage_size.y*pos->z);
    assert(index < form_cpu->storage_size.x*form_cpu->storage_size.y*form_cpu->storage_size.z, "pos: ", *pos, ", size: ", form_cpu->storage_size, "\n");
    int old_material = form_cpu->materials[index];
    form_cpu->materials[index] = material;
    form_gpu->material_bounds = expand_to(form_gpu->material_bounds, *pos);
    return true;

    return false;
}

void do_edit_window(memory_manager* manager, render_data* ui, user_input* input, genedit_window* gew, cuboid_space* form_space)
{
    //TODO: hotkeys, shift-click
    for(int i = 0; i < len(gene_list); i++)
    {
        gene_data* g = &gene_list[i];
        real_2 x = gew->x + (real_2){0.2, -0.2};
        x.x += i*genode_spacing;
        real_4 genode_color = {0.1*i,1,1-0.1*i,1};
        draw_circle(ui, {x.x, x.y, 0}, genode_radius, genode_color);
        if(normsq(x-input->mouse) < sq(genode_radius))
        {
            if(is_pressed(M1, input))
            {
                genode* new_genode = &gew->genodes[gew->n_genodes++];
                *new_genode = {
                    .gene_id = i,
                    .x = input->mouse,
                    .theta = 0,
                };

                input->active_ui_element = {ui_gene, new_genode};
            }
            draw_text(ui, g->name, {input->mouse.x, input->mouse.y, 0}, {0,0,0,1});
        }
    }

    if(gew->active_genome)
        for(int c = 0; c < gew->active_genome->n_cell_types; c++)
        {
            cell_type* cell = &gew->active_genome->cell_types[c];
            real_2 x_anchor = gew->x + (real_2){0.2, -0.2-1.5*gene_row_spacing};
            x_anchor.y += -c*gene_row_spacing;

            real_2 x_activate = x_anchor;
            x_activate.x -= 1.5*genode_spacing;
            real_4 button_color = {0.5,0.5,0.5,1};
            int cell_material = c+128;
            real button_radius = genode_radius*1.5;
            if(is_pressed(M1, input) && normsq(input->mouse-x_activate) < sq(button_radius))
            {
                gew->active_cell_material = cell_material;
            }
            if(gew->active_cell_material == cell_material) button_color = {1,1,1,1};
            draw_circle(ui, pad_3(x_activate), button_radius, button_color);

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
    cpu_form_data* hit_form_cpu = 0;
    for(int f = 0; f < gew->active_genome->n_forms; f++)
    {
        gpu_form_data* form_gpu = &gew->active_genome->forms_gpu[f];
        cpu_form_data* form_cpu = &gew->active_genome->forms_cpu[f];

        gew->active_genome->form_id_updates[f] = f;

        form_gpu->is_mutating = false;

        float fov = pi*120.0/180.0;
        float screen_dist = 1.0/tan(fov/2);
        real_3 ray_dir = (+input->mouse.x*gew->camera_axes[0]
                          +input->mouse.y*gew->camera_axes[1]
                          -screen_dist   *gew->camera_axes[2]);
        ray_dir = normalize(ray_dir);

        real_3 ray_pos = gew->camera_pos;

        ray_dir = apply_rotation(conjugate(form_gpu->orientation), ray_dir);
        ray_pos = apply_rotation(conjugate(form_gpu->orientation), ray_pos - form_gpu->x) + real_cast(form_gpu->x_origin);
        ray_hit new_hit = cast_ray(form_cpu->materials, ray_dir, ray_pos, form_cpu->storage_size, {}, 100);

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

            if(is_pressed(M1, input))
            {
                input->active_ui_element = {ui_form_voxel, hit_form_cpu};
                gew->extrude_counter = {};
                gew->drag_key = M1;
                gew->active_material = gew->active_cell_material;
                gew->selected_form = gew->active_form;

                if(set_form_cell(manager, form_space, gew->active_genome, gew->active_form, &hit.pos, gew->active_material))
                {
                    hit_form_cpu->gpu_data->is_mutating = true;
                    reload_form_to_gpu(hit_form_cpu, hit_form_cpu->gpu_data);
                }
            }
            if(is_pressed(M2, input))
            {
                input->active_ui_element = {ui_form_voxel, hit_form_cpu};
                gew->extrude_counter = {};
                gew->drag_key = M2;
                gew->active_material = 0;
                gew->selected_form = gew->active_form;

                if(set_form_cell(manager, form_space, gew->active_genome, gew->active_form, &hit.pos, gew->active_material))
                {
                    hit_form_cpu->gpu_data->is_mutating = true;
                    reload_form_to_gpu(hit_form_cpu, hit_form_cpu->gpu_data);
                }
            }
            if(is_pressed('X', input))
            {
                delete_form(gew->active_genome, gew->active_form);
            }
        }
        else
        {
            gew->active_form = -1;
        }
    }

    real_2 tool_bar_x = (real_2){0.2, -0.1};

    {
        real r = 0.05;
        real_2 x = gew->x+tool_bar_x;
        tool_bar_x.x += 0.1;
        draw_circle(ui, pad_3(x), r, {1,1,1,1});

        real_2 mouse_r = x - input->mouse;
        if(input->active_ui_element.type == 0 && is_pressed(M1, input) && normsq(mouse_r) < sq(r)) input->active_ui_element.type = ui_form_joint;

        if(input->active_ui_element.type == ui_form_joint)
        {

            draw_circle(ui, pad_3(input->mouse), r, {1,1,1,1});

            if(!is_down(M1, input))
            {
                if(hit.hit)
                { //insert new part
                    int_3 size = {4,1,1};
                    int total_size = size.x*size.y*size.z;
                    // size += abs_per_axis(hit.dir);
                    int f = gew->active_genome->n_forms++;
                    cpu_form_data* form_cpu = &gew->active_genome->forms_cpu[f];
                    gpu_form_data* form_gpu = &gew->active_genome->forms_gpu[f];
                    *form_cpu = {};
                    *form_gpu = {};
                    form_cpu->gpu_data = form_gpu;
                    form_cpu->materials = dynamic_alloc(manager->first_region, total_size);
                    gew->active_material = gew->active_cell_material;
                    for(int i = 0; i < total_size; i++) form_cpu->materials[i] = gew->active_material;
                    // form_cpu->materials[0] = gew->active_material;
                    // int_3 extend_dir = abs_per_axis(hit.dir);
                    // form_cpu->materials[extend_dir.x+size.x*(extend_dir.y+size.y*extend_dir.z)] = gew->active_material;
                    form_cpu->storage_size = size;
                    form_gpu->x_origin = {0,0,0};
                    form_gpu->cell_material_id = 0;

                    form_gpu->x = {0,0,0};
                    form_gpu->orientation = {1,0,0,0};

                    form_cpu->int_orientation = 0;

                    gew->active_genome->form_id_updates[f] = f;

                    int hit_form_id = hit_form_cpu-gew->active_genome->forms_cpu;
                    gew->active_genome->joints[gew->active_genome->n_joints++] = {
                        .type = joint_ball,
                        .form_id = {hit_form_id, f},
                        .pos = {hit.pos-hit_form_cpu->gpu_data->x_origin, (int_3){0,0,0}-form_gpu->x_origin},
                    };

                    load_form_to_gpu(form_space, form_cpu, form_gpu);
                    log_output("added form\n");
                }
                input->active_ui_element = {};
            }
        }
    }

    {
        real r = 0.05;
        real_2 x = gew->x+tool_bar_x;
        tool_bar_x.x += 0.1;
        draw_circle(ui, pad_3(x), r, {1,1,1,1});

        real_2 mouse_r = x - input->mouse;
        if(input->active_ui_element.type == 0 && is_pressed(M1, input) && normsq(mouse_r) < sq(r)) input->active_ui_element.type = ui_form_foot;

        if(input->active_ui_element.type == ui_form_foot)
        {

            draw_circle(ui, pad_3(input->mouse), r, {1,1,1,1});

            if(!is_down(M1, input))
            {
                int hit_form_id = hit_form_cpu-gew->active_genome->forms_cpu;
                if(hit.hit)
                { //insert new endpoint
                    gew->active_genome->endpoints[gew->active_genome->n_endpoints++] = {
                        .type = endpoint_foot,
                        .form_id = hit_form_id,
                        .pos = hit.pos-hit_form_cpu->gpu_data->x_origin,
                    };

                    log_output("added foot\n");
                }
                input->active_ui_element = {};
            }
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
                apply_rotation(form_gpu->orientation, real_cast(gew->active_cell-form_gpu->x_origin)
                               +(real_3){0.5, 0.5, 0.5}) + form_gpu->x - gew->camera_pos;
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
                if(set_form_cell(manager, form_space, gew->active_genome, gew->active_form, &pos, gew->active_material))
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

    // if(input->active_ui_element.type == ui_form)
    // {
    //     if(!is_down('G', input)) input->active_ui_element = {};
    //     else
    //     {
    //         int f = gew->active_form;

    //         gpu_form_data* form_gpu = &gew->active_genome->forms_gpu[f];
    //         cpu_form_data* form_cpu = &gew->active_genome->forms_cpu[f];

    //         float fov = pi*120.0/180.0;
    //         float screen_dist = 1.0/tan(fov/2);
    //         real_3 ray_dir = (+input->mouse.x*gew->camera_axes[0]
    //                           +input->mouse.y*gew->camera_axes[1]
    //                           -screen_dist   *gew->camera_axes[2]);
    //         ray_dir = normalize(ray_dir);

    //         gew->cursor_dist += 0.01*input->mouse_wheel;
    //         real_3 cursor_x = gew->camera_pos+gew->cursor_dist*ray_dir;
    //         real_3 grab_point = real_cast(gew->active_cell)+(real_3){0.5,0.5,0.5};
    //         drag_form(form_cpu, form_gpu, grab_point, cursor_x);
    //     }
    // }

    {
        gpu_form_data* form_gpu = &gew->active_genome->forms_gpu[gew->selected_form];
        cpu_form_data* form_cpu = &gew->active_genome->forms_cpu[gew->selected_form];

        float fov = pi*120.0/180.0;
        float screen_dist = 1.0/tan(fov/2);
        real_3 ray_dir = (+input->mouse.x*gew->camera_axes[0]
                          +input->mouse.y*gew->camera_axes[1]
                          -screen_dist   *gew->camera_axes[2]);
        ray_dir = normalize(ray_dir);
        real_3 ray_pos = gew->camera_pos;

        bool clicked = false;

        const real angle_decay = 0.4;

        form_cpu->xi_visual *= angle_decay;
        form_cpu->psi_visual *= angle_decay;
        form_cpu->theta_visual *= angle_decay;
        form_cpu->phi_visual *= angle_decay;

        if(form_cpu->theta%2 == 0 && (form_cpu->xi%2 == 0 || form_cpu->theta%4 != 0) && (input->active_ui_element.type == ui_none || input->active_ui_element.element == &gew->gizmo_x))
        {
            int8* angle;
            real* angle_visual;
            if(form_cpu->theta%4 == 0)
            {
                angle = &form_cpu->psi;
                angle_visual = &form_cpu->psi_visual;
            }
            else
            {
                angle = &form_cpu->xi;
                angle_visual = &form_cpu->xi_visual;
            }
            if(input->active_ui_element.type == ui_none)
            {
                gew->gizmo_x.x = form_gpu->x;
                gew->gizmo_x.normal = {cos((pi/4.0)*form_cpu->phi),sin((pi/4.0)*form_cpu->phi),0};
            }
            clicked = do_rotation_gizmo(ui, input, &gew->gizmo_x, ray_pos, ray_dir, gew->camera_axes);
            if(clicked)
            {
                int added_angle = (gew->gizmo_x.angle)/(pi/4.0);
                *angle = gew->gizmo_x.initial_angle+added_angle;

                *angle_visual = lerp(*angle_visual, gew->gizmo_x.angle-added_angle*(pi/4.0), angle_decay);

                ui->log_pos += sprintf(ui->debug_log+ui->log_pos, "angle: %f, %d\n", gew->gizmo_x.angle*180.0/pi, added_angle);
            }
            else
            {
                gew->gizmo_x.initial_angle = *angle;
            }
        }
        else if(input->active_ui_element.element == &gew->gizmo_x)
        {
            input->active_ui_element = {};
        }

        if(form_cpu->psi%2 == 0 && (form_cpu->xi%2 == 0 || form_cpu->psi%4 != 0) && (input->active_ui_element.type == ui_none || input->active_ui_element.element == &gew->gizmo_y))
        {
            int8* angle;
            real* angle_visual;
            if(form_cpu->psi%4 == 0)
            {
                angle = &form_cpu->theta;
                angle_visual = &form_cpu->theta_visual;
            }
            else
            {
                angle = &form_cpu->xi;
                angle_visual = &form_cpu->xi_visual;
            }
            if(input->active_ui_element.type == ui_none)
            {
                gew->gizmo_y.x = form_gpu->x;
                gew->gizmo_y.normal = {-sin((pi/4.0)*form_cpu->phi),cos((pi/4.0)*form_cpu->phi),0};
            }
            clicked = do_rotation_gizmo(ui, input, &gew->gizmo_y, ray_pos, ray_dir, gew->camera_axes);
            if(clicked)
            {
                int added_angle = (gew->gizmo_y.angle)/(pi/4.0);
                *angle = gew->gizmo_y.initial_angle+added_angle;

                *angle_visual = lerp(*angle_visual, gew->gizmo_y.angle-added_angle*(pi/4.0), angle_decay);

                ui->log_pos += sprintf(ui->debug_log+ui->log_pos, "angle: %f, %d\n", gew->gizmo_y.angle*180.0/pi, added_angle);
            }
            else
            {
                gew->gizmo_y.initial_angle = *angle;
            }
        }
        else if(input->active_ui_element.element == &gew->gizmo_y)
        {
            input->active_ui_element = {};
        }

        if(input->active_ui_element.type == ui_none || input->active_ui_element.element == &gew->gizmo_z)
        {
            if(input->active_ui_element.type == ui_none)
            {
                gew->gizmo_z.x = form_gpu->x;
                gew->gizmo_z.normal = {0,0,1};
            }
            clicked = do_rotation_gizmo(ui, input, &gew->gizmo_z, ray_pos, ray_dir, gew->camera_axes);
            if(clicked)
            {
                int added_angle = (gew->gizmo_z.angle)/(pi/4.0);
                form_cpu->phi = gew->gizmo_z.initial_angle+added_angle;

                form_cpu->phi_visual = lerp(form_cpu->phi_visual, gew->gizmo_z.angle-added_angle*(pi/4.0), angle_decay);

                ui->log_pos += sprintf(ui->debug_log+ui->log_pos, "angle: %f, %d\n", gew->gizmo_z.angle*180.0/pi, added_angle);
            }
            else
            {
                gew->gizmo_z.initial_angle = form_cpu->phi;
            }
        }

        gew->active_genome->is_mutating = true;

        // quaternion orientation = {1,0,0,0};
        // real inc = pi/4.0; //angular increment
        // orientation = axis_to_quaternion((real_3){0,form_cpu->theta*inc,0})*orientation;
        // orientation = axis_to_quaternion((real_3){0,0,form_cpu->phi*inc})*orientation;

        // real_3 psi_axis = apply_rotation(orientation, (real_3){1,0,0});

        // orientation = orientation*axis_to_quaternion((real_3){form_cpu->psi*inc,0,0});

        // real_3 xi_axis = apply_rotation(orientation, (real_3){0,0,1});

        // const int subdivs = 32;
        // for(int i = 0; i < subdivs; i++)
        // {
        //     real_3 x_hat = normalize(cross(psi_axis, {1,1,1}));
        //     real_3 y_hat = cross(psi_axis, x_hat);
        //     real theta = 2.0*pi*i/subdivs;
        //     real_3 a = 4*(cos(theta)*x_hat+sin(theta)*y_hat);
        //     draw_circle(ui, project_to_camera(form_gpu->x+a, ray_pos, gew->camera_axes), 0.01, {1,1,0,1});
        // }

        // for(int i = 0; i < subdivs; i++)
        // {
        //     real_3 x_hat = normalize(cross(xi_axis, {1,1,1}));
        //     real_3 y_hat = cross(xi_axis, x_hat);
        //     real theta = 2.0*pi*i/subdivs;
        //     real_3 a = 4*(cos(theta)*x_hat+sin(theta)*y_hat);
        //     draw_circle(ui, project_to_camera(form_gpu->x+a, ray_pos, gew->camera_axes), 0.01, {0,1,1,1});
        // }
    }

    genome* g = gew->active_genome;
    for(int j = g->n_joints-1; j >= 0; j--)
    {
        for(int i = 0; i < 2; i++)
        {
            int new_id = g->form_id_updates[g->joints[j].form_id[i]];
            if(new_id < 0)
            {
                g->joints[j] = g->joints[--g->n_joints];
                break;
            }
            g->joints[j].form_id[i] = new_id;
        }
    }

    for(int e = g->n_endpoints-1; e >= 0; e--)
    {
        int new_id = g->form_id_updates[g->endpoints[e].form_id];
        if(new_id < 0)
        {
            g->endpoints[e] = g->endpoints[--g->n_endpoints];
            continue;
        }
        g->endpoints[e].form_id = new_id;

        cpu_form_data* form_cpu = &g->forms_cpu[new_id];
        gpu_form_data* form_gpu = &g->forms_gpu[new_id];
        uint8 material = form_cpu->materials[index_3D(g->endpoints[e].pos+form_gpu->x_origin, form_cpu->storage_size)];
        // if(material == 0)
        // {
        //     g->endpoints[e] = g->endpoints[--g->n_endpoints];
        //     continue;
        // }

        real_3 r = apply_rotation(form_gpu->orientation, real_cast(g->endpoints[e].pos)+(real_3){0.5,0.5,0.5});
        draw_circle(ui, project_to_camera(form_gpu->x+r, gew->camera_pos, gew->camera_axes), 0.01, {1,1,0,1});
    }

    solve_form_joints(gew->active_genome);
}

#endif //GENETICS
