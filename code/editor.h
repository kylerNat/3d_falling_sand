#ifndef EDITOR
#define EDITOR

enum editor_object_type
{
    OBJ_PROP,
    OBJ_WORLD,
    OBJ_BODY,
    OBJ_CREATURE
};

struct editor_object
{
    int id;
    char* name;
    int type;

    bounding_box region;
    uint8* materials;

    bounding_box modified_region;
    bounding_box texture_region;

    //color is original_color*tint+highlight;
    real_4 tint;
    real_4 highlight;

    real_3 x;
    quaternion orientation;

    bool gridlocked;
    bool visible;
    bool is_world; //if this is true the data should be uploaded into the memory for world materials
};

#pragma pack(push, 1)
struct gpu_object_data
{
    bounding_box texture_region;
    int_3 origin_to_lower;
    real_3 x;
    quaternion orientation;

    real_4 tint;
    real_4 highlight;
};
#pragma pack(pop)

struct object_joint
{
    int type;
    int object_id[2];
    int_3 pos[2];
    int axis[2];
};

struct editor_data
{
    index_table_entry* object_table;
    int n_max_objects;
    int next_object_id;
    editor_object* objects;
    int n_objects;

    object_joint* joints;
    int n_joints;

    cuboid_space texture_space; //allocation of texture regions

    editor_object* selected_object;
    bounding_box current_selection; //within the current object
    int_3 active_cell; //cell to highlight from mouse hover
    real_3 rotation_center;
    int_3 selected_cell;

    int_3 pending_edit_p0;
    int_3 pending_edit_p1;

    int tool;
    int material;

    real camera_dist;
    real theta, phi;
    real_3 camera_center;

    real rotate_sensitivity;
    real pan_sensitivity;
    real move_sensitivity;
    real zoom_sensitivity;

    render_data rd;
};

editor_object* create_object(editor_data* ed)
{
    for(int i = 0; i < ed->n_max_objects; i++)
    {
        int id = ed->next_object_id++;
        index_table_entry* entry = &ed->object_table[id % ed->n_max_objects];
        if(entry->id == 0)
        {
            entry->id = id;
            entry->index = ed->n_objects++;
            ed->objects[entry->index].id = id;
            return &ed->objects[entry->index];
        }
    }
    return 0;
}

editor_object* get_object(editor_data* ed, int id)
{
    if(id == 0) return 0;
    index_table_entry entry = ed->object_table[id % ed->n_max_objects];
    if(entry.id == id)
    {
        return &ed->objects[entry.index];
    }
    return 0;
}

void delete_object(editor_data* ed, int id)
{
    index_table_entry* entry = &ed->object_table[id % ed->n_max_objects];
    if(entry->id == id)
    {
        entry->id = 0;
        ed->objects[entry->index] = ed->objects[--ed->n_objects];
        int moved_id = ed->objects[entry->index].id;
        if(ed->object_table[moved_id % ed->n_max_objects].id == moved_id)
        {
            ed->object_table[moved_id % ed->n_max_objects].index = entry->index;
        }
    }
}

struct button_state
{
    bool clicked;
    bool hovered;
};

button_state do_text_button(render_data* ui, user_input* input, char* text, real_3 x, real_2 r)
{
    bool hovered = all_less_than_eq(x.xy-r, input->mouse) && all_less_than(input->mouse, x.xy+r);
    real_4 background_color = ui->background_color;
    if(hovered) background_color = ui->highlight_color;
    real_4 text_color = ui->foreground_color;

    draw_rectangle(ui, x, 1.01*r, text_color);
    draw_rectangle(ui, x, r, background_color);
    draw_text(ui, text, x, text_color);
    bool clicked = (hovered && is_pressed(M1, input));
    if(clicked) input->click_blocked = true;
    return {clicked, hovered};
}

button_state do_image_button(render_data* ui, user_input* input, int sprite, real_3 x, real_2 r)
{
    bool hovered = all_less_than_eq(x.xy-r, input->mouse) && all_less_than(input->mouse, x.xy+r);
    real_4 background_color = ui->background_color;
    if(hovered) background_color = ui->highlight_color;
    real_4 text_color = ui->foreground_color;

    draw_rectangle(ui, x, 1.01*r, text_color);
    draw_rectangle(ui, x, r, background_color);
    draw_sprite(ui, x, 0.75*r, text_color, 0.0f, sprite, 0);
    bool clicked = (hovered && is_pressed(M1, input));
    if(clicked) input->click_blocked = true;
    return {clicked, hovered};
}

button_state do_color_button(render_data* ui, user_input* input, real_4 color, real_4 background_color, real_3 x, real_2 r)
{
    bool hovered = all_less_than_eq(x.xy-r, input->mouse) && all_less_than(input->mouse, x.xy+r);
    real_4 outline_color = ui->foreground_color;
    if(hovered) outline_color = ui->highlight_color;

    draw_rectangle(ui, x, 1.1*r, outline_color);
    draw_rectangle(ui, x, r, background_color);
    draw_rectangle(ui, x, 0.6*r, color);
    bool clicked = (hovered && is_pressed(M1, input));
    if(clicked) input->click_blocked = true;
    return {clicked, hovered};
}

void do_object_properties_window(render_data* ui, user_input* input, editor_object* o, real_3 x)
{
    real_2 window_size = {0.4, 0.8};
    draw_rectangle(ui, x+(real_3){0.5*window_size.x, -0.5*window_size.y, 0}, 0.5*window_size, ui->background_color);
    x += (real_3){0.05, -0.05, 0};
    draw_text(ui, o->name, x, ui->foreground_color, {-1, 1});
    real line_spacing = 0.05;
    x.y += -line_spacing;

    draw_text(ui, "Type: Prop", x, ui->foreground_color, {-1, 1});
}

enum tool_enum
{
    TOOL_SELECT,
    TOOL_MOVE,
    TOOL_ROTATE,
    TOOL_DRAW,
    TOOL_EDIT,
    TOOL_EXTRUDE,
    TOOL_JOINT,
    N_TOOLS,
};

struct tool_button
{
    int tool_id;
    int icon;
    int hotkey;
    int_2 icon_pos;
};

//ordered by default hotkey in QWERTY order
tool_button tool_list[] =
{
    {TOOL_EXTRUDE, SPR_ERASER, 'W', {1,0}},
    {TOOL_EDIT,    SPR_ERASER,  'E', {2,0}},
    {TOOL_ROTATE,  SPR_ROTATE,  'R', {3,0}},
    {TOOL_JOINT,   SPR_JOINT,   'T', {4,0}},
    {TOOL_SELECT,  SPR_CURSOR,  'S', {1,1}},
    {TOOL_DRAW,    SPR_PENCIL,  'D', {2,1}},
    {TOOL_MOVE,    SPR_MOVE,    'G', {4,1}},
};

void resize_object_storage(editor_object* o, bounding_box new_region)
{
    bounding_box old_region = o->region;
    if(old_region == new_region) return;

    int_3 new_size = dim(new_region);
    int_3 old_size = dim(old_region);

    int_3 offset = new_region.l-o->region.l;

    size_t new_size_total = new_size.x*new_size.y*new_size.z;
    uint8* new_materials = dynamic_alloc(new_size_total);

    int_3 lower = max_per_axis(new_region.l, old_region.l);
    int_3 upper = min_per_axis(new_region.u, old_region.u);

    memset(new_materials, 0, new_size_total);

    for(int z = lower.z; z < upper.z; z++)
        for(int y = lower.y; y < upper.y; y++)
        {
            int x = lower.x;

            int nx = x-new_region.l.x;
            int ny = y-new_region.l.y;
            int nz = z-new_region.l.z;

            int ox = x-old_region.l.x;
            int oy = y-old_region.l.y;
            int oz = z-old_region.l.z;

            memcpy(new_materials+index_3D({nx,ny,nz}, new_size), o->materials+index_3D({ox, oy, oz}, old_size), upper.x-lower.x);
        }

    dynamic_free(o->materials);

    o->region = new_region;
    o->materials = new_materials;
}

void update_editor(editor_data* ed, render_data* ui, user_input* input)
{
    //update camera
    if(is_down(M3, input))
    {
        ed->phi -= ed->rotate_sensitivity*input->dmouse.x;
        ed->theta += ed->rotate_sensitivity*input->dmouse.y;
    }
    ed->camera_dist *= pow(1-ed->zoom_sensitivity, input->mouse_wheel);
    ed->rd.camera_axes[0] = {-sin(ed->phi), cos(ed->phi), 0};
    ed->rd.camera_axes[2] = {sin(ed->theta)*cos(ed->phi), sin(ed->theta)*sin(ed->phi), cos(ed->theta)};
    ed->rd.camera_axes[1] = cross(ed->rd.camera_axes[2], ed->rd.camera_axes[0]);
    if(is_down(M2, input))
    {
        real sensitivity = 0.5*ed->camera_dist*ed->rd.fov;
        ed->camera_center -= sensitivity*input->dmouse.x*ed->rd.camera_axes[0];
        ed->camera_center -= sensitivity*input->dmouse.y*ed->rd.camera_axes[1];
    }
    real forward_input = is_down(M5, input)-is_down(M4, input);
    ed->camera_center -= ed->move_sensitivity*forward_input*ed->rd.camera_axes[2];
    ed->rd.camera_pos = ed->camera_center+ed->camera_dist*ed->rd.camera_axes[2];

    draw_sphere(&ed->rd, ed->camera_center, 0.5, {1,1,1,0.02});

    //main ui
    real_3 current_pos = {-1.5, 0.7, 0};

    //toolbar
    int new_tool = ed->tool; //don't update immediatly so there isn't any wierd highlight switch in the middle of drawing the icons
    real_3 line_pos = current_pos;
    real button_size = 0.04;
    real button_spacing = 0.09;
    for(int t = 0; t < len(tool_list); t++)
    {
        tool_button* tool = tool_list+t;
        real_4 tint = {1,1,1,1};
        if(ed->tool != tool->tool_id) tint = {0.25,0.25,0.25,1};
        real_4 old_foreground_color = ui->foreground_color;
        real_4 old_background_color = ui->background_color;
        if(ed->tool == tool->tool_id)
        {
            ui->foreground_color = {1,0,0,1};
        }
        real_3 pos = current_pos+(real_3){button_spacing*tool->icon_pos.x, -button_spacing*tool->icon_pos.y, 0};
        button_state bs = do_image_button(ui, input, tool->icon, pos, {button_size, button_size});
        ui->foreground_color = old_foreground_color;
        ui->background_color = old_background_color;
        if(is_pressed(tool->hotkey, input) || bs.clicked) new_tool = tool->tool_id;
    }
    ed->tool = new_tool;

    current_pos.y -= button_spacing*3;

    //materials
    int new_material = ed->material; //don't update immediatly so there isn't any wierd highlight switch in the middle of drawing the icons
    line_pos = current_pos;
    for(int m = 0; m < n_materials; m++)
    {
        material_t* mat = materials_list+m;
        real_4 tint = {1,1,1,1};
        if(ed->material != m) tint = {0.25,0.25,0.25,1};
        real_4 old_foreground_color = ui->foreground_color;
        real_4 old_background_color = ui->background_color;
        if(ed->material == m)
        {
            ui->foreground_color = {1,0,0,1};
        }
        real_3 pos = current_pos;
        current_pos.y -= button_spacing;
        button_state bs = do_color_button(ui, input, pad_4(mat->base_color, 1.0f), pad_4(mat->emission, 1.0f), pos, {button_size, button_size});
        ui->foreground_color = old_foreground_color;
        ui->background_color = old_background_color;
        if(bs.clicked) new_material = m;
    }
    ed->material = new_material;

    //objects
    current_pos = {-1.3, 0.9, 0};
    if(do_text_button(ui, input, "New Object", current_pos, {0.15, 0.05}).clicked)
    {
        editor_object* o = create_object(ed);
        o->name = (char*) malloc(256);
        sprintf(o->name, "Object %d", o->id);
        int_3 size = {5,5,5};
        int total_size = axes_product(size);
        o->region = {{0,0,0},size};
        o->materials = dynamic_alloc(total_size);
        o->tint = {1,1,1,1};
        o->highlight = {};
        o->x = ed->camera_center;
        o->orientation = {1,0,0,0};
        o->gridlocked = false;
        o->visible = true;
        o->is_world = false;
        memset(o->materials, ed->material, total_size);
    }

    current_pos = {-0.9, 0.9, 0};

    real screen_dist = 1.0/tan(ed->rd.fov/2);
    real_3 raw_ray_dir = (+input->mouse.x*ed->rd.camera_axes[0]
                          +input->mouse.y*ed->rd.camera_axes[1]
                          -screen_dist   *ed->rd.camera_axes[2]);
    raw_ray_dir = normalize(raw_ray_dir);

    ray_hit hit = {};
    editor_object* hit_object = 0;
    real_3 list_start_pos = current_pos;
    real_3 list_pos = current_pos;
    list_pos.y -= 0.11;
    for(int i = 0; i < ed->n_objects; i++)
    {
        editor_object* o = ed->objects+i;

        button_state object_button = do_text_button(ui, input, o->name, current_pos, {0.15, 0.05});
        current_pos.y -= 0.11;
        if(object_button.hovered)
        {
            hit_object = o;
            hit = {true};
            hit.dist = -1;
        }

        if(!o->visible) continue;

        real_3 ray_dir = apply_rotation(conjugate(o->orientation), raw_ray_dir);
        real_3 ray_pos = apply_rotation(conjugate(o->orientation), ed->rd.camera_pos - o->x) - real_cast(o->region.l);
        int_3 size = dim(o->region);
        int max_iterations = axes_sum(size);
        ray_hit new_hit = cast_ray(o->materials, ray_pos, ray_dir, size, max_iterations);
        new_hit.pos += o->region.l;
        if(new_hit.hit && (!hit_object || new_hit.dist < hit.dist))
        {
            hit_object = o;
            hit = new_hit;
        }
    }
    for(int i = 0; i < ed->n_joints; i++)
    {
        object_joint* joint = ed->joints+i;
        real_3 base_pos = list_start_pos;
        base_pos.x += 0.15;
        real_3 pos[] = {base_pos, base_pos};
        pos[0].y += -0.11f*joint->object_id[0];
        pos[1].y += -0.11f*joint->object_id[1];
        real line_radius = 0.005;
        add_line_point(ui, pos[0], line_radius, {1,1,1,1});
        add_line_point(ui, pos[0]+(real_3){0.1,0,0}, line_radius, {1,1,1,1});
        add_line_point(ui, pos[1]+(real_3){0.1,0,0}, line_radius, {1,1,1,1});
        add_line_point(ui, pos[1], line_radius, {1,1,1,1});
        draw_line(ui);
    }

    {
        real_3 hit_x = ed->rd.camera_pos+raw_ray_dir*hit.dist;
        ui->log_pos += sprintf(ui->debug_log+ui->log_pos, "hit: (%d, %d, %d), (%f, %f, %f), %f, object id: %d\n",
                               hit.pos.x, hit.pos.y, hit.pos.z,
                               hit_x.x, hit_x.y, hit_x.z,
                               hit.dist,
                               hit_object ? int(hit_object-ed->objects) : 0);
    }

    editor_object* old_object = ed->selected_object;
    int_3 old_selected_cell = ed->selected_cell;
    if(input->active_ui_element.type == 0 && is_pressed(M1, input) && (hit.dist == -1 || ed->tool != TOOL_SELECT))
    {
        ed->selected_object = hit_object;
        ed->selected_cell = hit.pos;
    }

    if(!input->click_blocked && is_pressed(M1, input) && is_down(VK_MENU, input) && hit.hit && hit.dist > 0)
    {
        ed->material = hit_object->materials[index_3D(hit.pos+hit_object->region.l, dim(hit_object->region))];
        input->click_blocked = true;
    }

    for(int i = 0; i < ed->n_objects; i++)
    {
        editor_object* o = ed->objects+i;

        if(o == ed->selected_object || o == hit_object)
        {
            o->tint = {1,1,1,1};
            o->highlight = {};

            real_3x3 axes = apply_rotation(o->orientation, real_identity_3(1));
            //draw outline
            draw_box(&ed->rd, o->x+apply_rotation(o->orientation, real_cast(o->region.l)),
                     real_cast(dim(o->region)), 0.05, axes, {0.8,0.8,0.8,0.02});
        }
    }

    if(ed->selected_object)
    {
        editor_object* o = ed->selected_object;

        current_pos = {1.0, 0.8};
        do_object_properties_window(ui, input, o, current_pos);

        // real_3 ray_dir = apply_rotation(conjugate(o->orientation), raw_ray_dir);
        // real_3 ray_pos = apply_rotation(conjugate(o->orientation), ed->rd.camera_pos - o->x) - real_cast(o->region.l);
        // int_3 size = dim(o->region);
        // int max_iterations = axes_sum(size);
        // ray_hit hit = cast_ray(o->materials, ray_pos, ray_dir, size, max_iterations);
        // hit.pos += o->region.l;

        real_3x3 axes = apply_rotation(o->orientation, real_identity_3(1));

        real gizmo_alpha = 0.2;
        switch(ed->tool)
        {
            case TOOL_SELECT:
            {
                ring_hit giz_hit = {};
                giz_hit.dist = +inf;
                int hit_axis = -1;
                real_3 region_center = 0.5*real_cast(o->region.u+o->region.l); //center in object coordinates
                real_3 center = o->x+apply_rotation(o->orientation, region_center);
                real gizmo_radius = 5;
                //rotation handles
                for(int i = 0; i < 3; i++)
                {
                    real_4 color = {0,0,0,gizmo_alpha};
                    color[i] = 1;
                    draw_ring(&ed->rd, center, gizmo_radius, 0.2, axes[i], color);
                    //increase girth to make it easier to click
                    ring_hit new_hit = project_to_ring(ed->rd.camera_pos, raw_ray_dir, center, gizmo_radius, 0.4, axes[i]);
                    if(new_hit.hit > giz_hit.hit || (new_hit.hit == giz_hit.hit && new_hit.dist < giz_hit.dist))
                    {
                        giz_hit = new_hit;
                        hit_axis = i;
                    }
                }

                //translation handles
                real_3 line_hit = {};
                real_3 line_hit_pos = {};
                for(int i = 0; i < 3; i++)
                {
                    real_4 color = {0,0,0,gizmo_alpha};
                    color[i] = 1;
                    real_3 axis = {};
                    axis[i] = 1;
                    real length = 2;
                    real offset = gizmo_radius+1;
                    ring_hit new_hit;

                    draw_line_3d(&ed->rd, center-offset*axis, length, 0.2, -axis, color);
                    new_hit = project_to_line(ed->rd.camera_pos, raw_ray_dir, center-offset*axis, length, 0.4, -axis);
                    if(new_hit.hit > giz_hit.hit || (new_hit.hit == giz_hit.hit && new_hit.dist < giz_hit.dist))
                    {
                        giz_hit = new_hit;
                        hit_axis = 3+i;
                    }

                    draw_line_3d(&ed->rd, center+offset*axis, length, 0.2, axis, color);
                    //increase girth to make it easier to click
                    new_hit = project_to_line(ed->rd.camera_pos, raw_ray_dir, center+offset*axis, length, 0.4, axis);
                    if(new_hit.hit > giz_hit.hit || (new_hit.hit == giz_hit.hit && new_hit.dist < giz_hit.dist))
                    {
                        giz_hit = new_hit;
                        hit_axis = 3+i;
                    }
                }

                if(giz_hit.hit && input->active_ui_element.type == ui_none)
                {
                    real_4 color = {0,0,0,1};
                    color[hit_axis%3] = 1;
                    draw_sphere(&ed->rd, giz_hit.nearest, 0.5, color);
                }
                else if(input->active_ui_element.type == ui_none && is_pressed(M1, input) && hit.hit)
                {
                    ed->selected_object = hit_object;
                    ed->selected_cell = hit.pos;
                }

                static real_3 grab_point = {}; //TODO: put this somewhere
                if(input->active_ui_element.type == ui_none && is_pressed(M1, input) && giz_hit.hit)
                {
                    input->active_ui_element.type = ui_gizmo;
                    if(hit_axis < 3) input->active_ui_element.element = o->orientation.data+hit_axis;
                    else             input->active_ui_element.element = o->x.data+(hit_axis-3);

                    ed->rotation_center = center; //save center to avoid compounding rounding errors
                    grab_point = giz_hit.nearest-center;
                }

                static real_3 old_r = {}; //TODO: put this somewhere
                for(int i = 0; i < 3; i++)
                {
                    if(input->active_ui_element.type == ui_gizmo && input->active_ui_element.element == o->orientation.data+i)
                    {
                        if(is_down(M1, input))
                        {
                            //TODO: fall back to ring projection if the camera position is close to the plane of rotation
                            real_3 d = ed->rd.camera_pos-center;
                            real_3 r = d-dot(d, axes[i])/dot(raw_ray_dir, axes[i])*raw_ray_dir;
                            r = normalize(r);
                            real angle = asin(dot(cross(old_r, r), axes[i]));
                            o->orientation = axis_to_quaternion(angle*axes[i])*o->orientation;
                            old_r = r;

                            o->x = ed->rotation_center-apply_rotation(o->orientation, region_center);

                            real_4 color = {0,0,0,1};
                            color[i] = 1;
                            draw_sphere(&ed->rd, center+gizmo_radius*r, 0.6, color);
                        }
                        else
                        {
                            input->active_ui_element = {};
                            old_r = {};
                        }
                    }

                    if(input->active_ui_element.type == ui_gizmo && input->active_ui_element.element == o->x.data+i)
                    {
                        if(is_down(M1, input))
                        {
                            real_3 axis = {};
                            axis[i] = 1;

                            //TODO: project to the nearest point in screenspace, rather than world space
                            real_3 d = center-ed->rd.camera_pos;
                            real_3 perp = normalize(cross(axis, raw_ray_dir));
                            real_3 dplane = d-dot(perp, d)*perp;
                            real_3 dplaneperp = dplane-dot(dplane, axis)*axis;
                            real_3 planeperpdir = normalize(dplaneperp);
                            real   ray_dist = max(0.0f, dot(dplaneperp, planeperpdir)/dot(raw_ray_dir, planeperpdir));
                            real_3 ray_point = ed->rd.camera_pos+ray_dist*raw_ray_dir;
                            real   line_dist = dot(ray_point-center, axis);
                            real_3 line_point = line_dist*axis;

                            o->x += dot(line_point-grab_point, axis)*axis;

                            real_4 color = {0,0,0,1};
                            color[i] = 1;
                            draw_sphere(&ed->rd, center+line_point, 0.6, color);
                        }
                        else
                        {
                            input->active_ui_element = {};
                            old_r = {};
                        }
                    }
                }
                break;
            }
            case TOOL_MOVE:
            {
                //TODO: this should move everything connected through joints as well
                if(is_down(M1, input))
                {
                    //TODO: actually project onto plane perpendicular to z going through the initial pos
                    o->x += ed->rd.camera_axes[0]*input->dmouse.x;
                    o->x += ed->rd.camera_axes[1]*input->dmouse.y;
                }
                break;
            }
            case TOOL_ROTATE:
            {
                if(is_pressed(M1, input))
                {
                    real_3 r = apply_rotation(o->orientation, 0.5*real_cast(o->region.l+o->region.u));
                    ed->rotation_center = o->x+r;
                }
                if(is_down(M1, input))
                {
                    real_3 mouse_dir = (ed->rd.camera_axes[0]*input->dmouse.x+ed->rd.camera_axes[1]*input->dmouse.y);
                    real angle = ed->rotate_sensitivity*norm(mouse_dir);
                    real_3 axis = -angle*normalize_or_zero(cross(mouse_dir, ed->rd.camera_axes[2]));
                    quaternion rotation = axis_to_quaternion(axis);
                    o->orientation = rotation*o->orientation;
                    real_3 r = apply_rotation(o->orientation, 0.5*real_cast(o->region.l+o->region.u));
                    o->x = ed->rotation_center-apply_rotation(rotation, r);
                }
                break;
            }
            case TOOL_EDIT:
            {
                if(hit.hit)
                {
                    static bool erase = false;
                    static bool pending_edit = false;
                    erase = is_down(VK_SHIFT, input);
                    int_3 pos = erase ? hit.pos : hit.pos-hit.dir;
                    ed->active_cell = pos;
                    if(!input->click_blocked && is_pressed(M1, input))
                    {
                        ed->pending_edit_p0 = pos;
                        ed->pending_edit_p1 = pos;
                        pending_edit = true;
                    }
                    if(hit_object == o) ed->pending_edit_p1 = pos;

                    bounding_box edit_region = {
                        min_per_axis(ed->pending_edit_p0, ed->pending_edit_p1),
                        max_per_axis(ed->pending_edit_p0, ed->pending_edit_p1)+(int_3){1,1,1}
                    };

                    if(pending_edit)
                    {
                        draw_box(&ed->rd, o->x+apply_rotation(o->orientation, real_cast(edit_region.l)),
                                 real_cast(dim(edit_region)), 0.05, axes, {0.8,0.8,0.8,0.1});
                    }
                    if(pending_edit && is_released(M1, input))
                    {
                        if(o->is_world || erase)
                        {
                            edit_region = {
                                max_per_axis(o->region.l, edit_region.l),
                                min_per_axis(o->region.u, edit_region.u)
                            };
                        }
                        else
                        {
                            bounding_box new_region = {
                                min_per_axis(o->region.l, edit_region.l),
                                max_per_axis(o->region.u, edit_region.u),
                            };
                            resize_object_storage(o, new_region);
                        }

                        int_3 size = dim(o->region);
                        for(int z = edit_region.l.z; z < edit_region.u.z; z++)
                            for(int y = edit_region.l.y; y < edit_region.u.y; y++)
                                for(int x = edit_region.l.x; x < edit_region.u.x; x++)
                                {
                                    int_3 pos = {x,y,z};
                                    o->modified_region = expand_to(o->modified_region, pos);
                                    pos -= o->region.l;
                                    o->materials[index_3D(pos, size)] = erase ? 0 : ed->material;
                                }
                        pending_edit = false;
                    }
                }
                break;
            }
            case TOOL_DRAW:
            {
                if(hit.hit)
                {
                    int n_max_draw = 128*128*128;
                    static bool erase = false;
                    static int_3* draw_cells = (int_3*) stalloc(n_max_draw*sizeof(int_3));
                    static int n_draw_cells = 0;
                    static bool pending_edit = false;
                    static bounding_box edit_region = {};
                    erase = is_down(VK_SHIFT, input);
                    int_3 pos = erase ? hit.pos : hit.pos-hit.dir;
                    ed->active_cell = pos;

                    if(!input->click_blocked && is_pressed(M1, input))
                    {
                        n_draw_cells = 0;
                        pending_edit = true;
                        edit_region = {o->region.u, o->region.l};
                    }

                    if(pending_edit)
                    {
                        bool do_draw = n_draw_cells < n_max_draw && hit_object == o;
                        for(int i = 0;; i++)
                        {
                            bool do_break = false;
                            if(i >= n_draw_cells)
                            {
                                if(do_draw)
                                {
                                    draw_cells[n_draw_cells++] = pos;
                                    edit_region = expand_to(edit_region, pos);
                                }
                                do_break = true;
                            }

                            if(i < n_draw_cells)
                                draw_box(&ed->rd, o->x+apply_rotation(o->orientation, real_cast(draw_cells[i])),
                                         {1,1,1}, 0.05, axes, {0.8,0.8,0.8,0.1});

                            if(do_break) break;

                            if(pos == draw_cells[i]) do_draw = false;
                        }

                        // draw_box(&ed->rd, o->x+apply_rotation(o->orientation, real_cast(edit_region.l)),
                        //          real_cast(dim(edit_region)), 0.05, axes, {0.8,0.8,0.8,0.1});
                    }

                    if(pending_edit && is_released(M1, input))
                    {
                        if(o->is_world || erase)
                        {
                            edit_region = {
                                max_per_axis(o->region.l, edit_region.l),
                                min_per_axis(o->region.u, edit_region.u)
                            };
                        }
                        else
                        {
                            bounding_box new_region = {
                                min_per_axis(o->region.l, edit_region.l),
                                max_per_axis(o->region.u, edit_region.u),
                            };
                            resize_object_storage(o, new_region);
                        }

                        int_3 size = dim(o->region);
                        for(int i = 0; i < n_draw_cells; i++)
                        {
                            int_3 pos = draw_cells[i];
                            if(!is_inside(pos, edit_region)) continue;
                            o->modified_region = expand_to(o->modified_region, pos);
                            pos -= o->region.l;
                            o->materials[index_3D(pos, size)] = erase ? 0 : ed->material;
                        }
                        pending_edit = false;
                    }
                }
                break;
            }
            case TOOL_EXTRUDE:
            {
                static int_3* extrude_cells = (int_3*) stalloc(room_size*room_size*sizeof(int_3));
                static int_3 extrude_dir = {};
                static int n_extrude_cells = 0;
                static bounding_box extrude_region = {};
                if(!input->click_blocked && is_pressed(M1, input) && hit.hit && hit_object == o)
                {
                    extrude_region = {o->region.u, o->region.l};
                    n_extrude_cells = 0;
                    extrude_dir = -hit.dir;

                    int a0 = 0;
                    for(int i = 0; i < 3; i++)
                        if(hit.dir[i]) {
                            a0 = i;
                            break;
                        }
                    int a1 = (a0+1)%3;
                    int a2 = (a0+2)%3;
                    int plane_axes[] = {a1,a2};

                    int width  = dim(o->region)[a1];
                    int height = dim(o->region)[a2];

                    int* unsearched = (int*) stalloc(width*height*sizeof(int));
                    uint8* searched = (uint8*) stalloc_clear((width*height+7)/8);
                    int n_unsearched = 0;

                    int hit_material = o->materials[index_3D(hit.pos-o->region.l, dim(o->region))];

                    unsearched[n_unsearched++] = (hit.pos-o->region.l)[a1]+width*(hit.pos-o->region.l)[a2];
                    while(n_unsearched > 0)
                    {
                        int search_index = unsearched[--n_unsearched];
                        int_3 center_coord = {};
                        center_coord[a0] = hit.pos[a0]-o->region.l[a0];
                        center_coord[a1] = search_index%width;
                        center_coord[a2] = search_index/width;

                        if(o->materials[index_3D(center_coord, dim(o->region))] == hit_material && (center_coord[a0]-hit.dir[a0] < 0 || center_coord[a0]-hit.dir[a0] >= dim(o->region)[a0] || o->materials[index_3D(center_coord-hit.dir, dim(o->region))] == 0))
                        {
                            int_3 pos = center_coord+o->region.l;
                            extrude_region.l = min_per_axis(extrude_region.l, pos);
                            extrude_region.u = max_per_axis(extrude_region.u, pos+(int_3){1,1,1});

                            extrude_cells[n_extrude_cells++] = pos;
                            for(int s = 0; s < 2; s++) { //sign +/-
                                for(int a = 0; a < 2; a++) { //axis a1/a2
                                    if(s ? (center_coord[plane_axes[a]] < dim(o->region)[plane_axes[a]]-1)
                                       : (center_coord[plane_axes[a]] > 0)) {
                                        int_3 search_coord = center_coord;
                                        search_coord[plane_axes[a]] += s ? 1:-1;
                                        int new_search_index = search_coord[a1]+width*search_coord[a2];
                                        if(((searched[new_search_index/8]>>(new_search_index%8))&1)==0)
                                        {
                                            searched[new_search_index/8] |= 1<<(new_search_index%8);
                                            unsearched[n_unsearched++] = new_search_index;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    stunalloc(searched);
                    stunalloc(unsearched);
                }

                real_3 x0 = o->x+apply_rotation(o->orientation, real_cast(extrude_cells[0])+(real_3){0.5,0.5,0.5});
                real_3 line_dir = apply_rotation(o->orientation, real_cast(extrude_dir));

                real_3 screen_x0 = transpose(ed->rd.camera_axes)*(x0-ed->rd.camera_pos);
                screen_x0 *= -screen_dist/screen_x0.z;

                real_3 screen_x1 = transpose(ed->rd.camera_axes)*(x0+line_dir-ed->rd.camera_pos);
                screen_x1 *= -screen_dist/screen_x1.z;

                real_2 screen_dir = normalize_or_zero(screen_x1.xy-screen_x0.xy);

                real_2 screen_point = dot(input->mouse-screen_x0.xy, screen_dir)*screen_dir+screen_x0.xy;
                real_3 ray_dir = (+screen_point.x*ed->rd.camera_axes[0]
                                  +screen_point.y*ed->rd.camera_axes[1]
                                  -screen_dist   *ed->rd.camera_axes[2]);

                real_3 d = x0-ed->rd.camera_pos;
                real_3 perp = normalize_or_zero(cross(line_dir, ray_dir));
                real_3 plane_normal = normalize_or_zero(cross(line_dir, perp));
                real_3 proj_pos = ed->rd.camera_pos + (dot(d, plane_normal)/dot(ray_dir, plane_normal))*ray_dir;
                real_3 p1_real = apply_rotation(conjugate(o->orientation), proj_pos-o->x);

                int a0 = 0;
                for(int i = 0; i < 3; i++)
                    if(extrude_dir[i]) {
                        a0 = i;
                        break;
                    }
                int a1 = (a0+1)%3;
                int a2 = (a0+2)%3;

                int extrude_dist = int(ceil(extrude_dir[a0]*(p1_real[a0]-extrude_cells[0][a0]-0.5)));

                real_3 x1 = x0 + extrude_dist*line_dir;

                int x_a0 = extrude_cells[0][a0];
                extrude_region.l[a0] = min(x_a0+(extrude_dist>0?extrude_dir[a0]:0), x_a0+extrude_dist*extrude_dir[a0]);
                extrude_region.u[a0] = max(x_a0+(extrude_dist>0?extrude_dir[a0]:0), x_a0+extrude_dist*extrude_dir[a0])+1;

                if(n_extrude_cells > 0)
                {
                    draw_line_3d(&ed->rd, x0, x1, 0.04, {1,1,1,1});
                    draw_sphere(&ed->rd, o->x+apply_rotation(o->orientation, p1_real), 0.08, {1,1,1,1});
                    draw_box(&ed->rd, o->x+apply_rotation(o->orientation, real_cast(extrude_region.l)),
                             real_cast(dim(extrude_region)), 0.05, axes, {0.8,0.8,0.8,0.1});
                    // draw_circle(ui, screen_x0, 0.02, {1,0,0,1});
                    // draw_circle(ui, screen_x1, 0.02, {0,0,1,1});

                    ui->log_pos += sprintf(ui->debug_log+ui->log_pos, "extrude_dist: %d", extrude_dist);
                }

                if(is_released(M1, input) && n_extrude_cells > 0)
                {
                    if(o->is_world)
                    {
                        extrude_region = {
                            max_per_axis(o->region.l, extrude_region.l),
                            min_per_axis(o->region.u, extrude_region.u)
                        };
                    }
                    else
                    {
                        bounding_box new_region = {
                            min_per_axis(o->region.l, extrude_region.l),
                            max_per_axis(o->region.u, extrude_region.u),
                        };
                        resize_object_storage(o, new_region);
                    }

                    int_3 size = dim(o->region);
                    for(int d = extrude_dist > 0 ? 1:0; d <= abs(extrude_dist); d++)
                        for(int e = 0; e < n_extrude_cells; e++)
                        {
                            int_3 pos = extrude_cells[e]+(extrude_dist > 0 ? (d)*extrude_dir : -d*extrude_dir);
                            o->modified_region = expand_to(o->modified_region, pos);
                            pos -= o->region.l;
                            o->materials[index_3D(pos, size)] = extrude_dist > 0 ? ed->material : 0;
                        }

                    n_extrude_cells = 0;
                }
                if(!is_down(M1, input)) n_extrude_cells = 0;
                break;
            }
            case TOOL_JOINT:
            {
                static int_3 pending_pos = {};
                static editor_object* pending_object = 0;
                if(is_pressed(M1, input) && hit.hit)
                {
                    pending_object = hit_object;
                    pending_pos = hit.pos;
                }
                if(hit.hit && pending_object)
                {
                    real_3 x0 = pending_object->x+apply_rotation(pending_object->orientation, real_cast(pending_pos)+(real_3){0.5,0.5,0.5});
                    real_3 x1 = hit_object->x+apply_rotation(hit_object->orientation, real_cast(hit.pos)+(real_3){0.5,0.5,0.5});
                    draw_line_3d(&ed->rd, x0, x1, 0.04, {1,1,1,1});
                }
                if(is_released(M1, input) && hit.hit && hit_object != pending_object && !pending_object->is_world && !hit_object->is_world)
                {
                    int obj0 = pending_object->id;
                    int obj1 = hit_object->id;
                    for(int j = ed->n_joints-1; j >= 0; --j)
                    {
                        if(ed->joints[j].object_id[0] == obj0 && ed->joints[j].object_id[1] == obj1)
                            ed->joints[j] = ed->joints[--ed->n_joints];
                    }
                    ed->joints[ed->n_joints++] = {
                        .type = joint_ball,
                        .object_id = {obj0, obj1},
                        .pos = {pending_pos, hit.pos},
                        .axis = {0, 0},
                    };
                }
                if(!is_down(M1, input)) pending_object = 0;
                break;
            }
        }

        //solve joints
        //probably should iterate a few times but should be fine after a few frames as is
        for(int j = 0; j < ed->n_joints; j++)
        {
            object_joint* joint = ed->joints+j;
            editor_object* o0 = get_object(ed, joint->object_id[0]);
            editor_object* o1 = get_object(ed, joint->object_id[1]);
            real_3 r0 = apply_rotation(o0->orientation, real_cast(joint->pos[0])+(real_3){0.5, 0.5, 0.5});
            real_3 r1 = apply_rotation(o1->orientation, real_cast(joint->pos[1])+(real_3){0.5, 0.5, 0.5});
            real_3 center_x = 0.5*(o0->x+r0 + o1->x+r1);
            o0->x = center_x-r0;
            o1->x = center_x-r1;
        }
    }

    for(int i = 0; i < ed->n_objects; i++)
    {
        editor_object* o = ed->objects+i;
        int_3 modified_size = dim(o->modified_region);
        int_3 old_size = dim(o->region);
        if(modified_size.x > 0 && modified_size.y > 0 && modified_size.z > 0)
        {
            bounding_box new_region = {o->region.u, o->region.l};
            for(int z = 0; z < old_size.z; z++)
                for(int y = 0; y < old_size.y; y++)
                    for(int x = 0; x < old_size.x; x++)
                    {
                        int_3 pos = {x,y,z};
                        if(o->materials[index_3D(pos, dim(o->region))]) new_region = expand_to(new_region, pos+o->region.l);
                    }
            if(!all_less_than_eq(new_region.l, new_region.u)) new_region = {};
            resize_object_storage(o, new_region);
        }
    }
}

#endif //EDITOR
