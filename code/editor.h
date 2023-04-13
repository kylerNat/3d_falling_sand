#ifdef EDITOR
#define EDITOR

struct editor_object
{
    bounding_box region
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

struct editor_data
{
    editor_object* objects;
    int n_objects;

    editor_object* selected_object;
    bounding_box current_selection; //within the current object
};

void update_editor(editor_data* ed, input_data* input)
{
    real_3 raw_ray_dir = (+input->mouse.x*ed->camera_axes[0]
                          +input->mouse.y*ed->camera_axes[1]
                          -screen_dist   *ed->camera_axes[2]);
    if(!ed->selected_object)
    {
        for(int i = 0; i < ed->n_open_bodies; i++)
        {
            editor_object* o = ed->objects+i;
            if(!o->visible) continue;

            real_3 ray_dir = apply_orientation(conjugate(o->orientation), raw_ray_dir);
            real_3 ray_pos = ed->camera_x - o->x - real_cast(o->region.l);
            int_3 size = o->region.u-o->region.l;
            int max_iterations = axes_sum(size);
            ray_hit hit = cast_ray(o->materials, ray_dir, ray_pos, size, max_iterations);

            if(hit.hit)
            {
                o->tint = {0.5,0.5,0.5,1};
                o->hightlight = {0.5,0.5,0.5};
                if(input->active_ui_element.type == 0 && is_pressed(M1, input))
                {
                    ed->selected_object = o;
                    break;
                }
            }
            else
            {
                o->tint = {1,1,1,1};
                o->hightlight = {};
            }
        }
    }
    else
    {
        for(int i = 0; i < ed->n_open_bodies; i++)
        {
            editor_object* o = ed->objects+i;
            if(o != ed->selected_object) o->tint = {1,1,1,0.2};
            else o->tint = {1,1,1,1};

            o->hightlight = {};
        }

        editor_object* o = ed->selected_object;

        //TODO: edit stuff
        //button to go to overview mode
        if(do_button)
        {
            ed->selected_object = 0;
        }

        if(input->active_ui_element.type == 0 && is_pressed(M1, input))
        {

        }
    }

    do_object_list(ed, input);
}

#endif //EDITOR
