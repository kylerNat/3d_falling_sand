#ifndef EDITOR_DATA_BINDING
#define EDITOR_DATA_BINDING 0
#endif

struct brush_stroke
{
    int material;
    int shape;
    float x; float y; float z;
    float r;
};

#define BS_CUBE 0
#define BS_SPHERE 1

struct bounding_box
{
    int l_x; int l_y; int l_z;
    int u_x; int u_y; int u_z;
};

layout(std430, binding = EDITOR_DATA_BINDING) buffer editor_data
{
    bounding_box new_selection;
    int selection_mode; //0 visual indicator only, 1 select, 2 deselect
    ivec3 sel_move;
    int sel_fill;
    int n_strokes;
    brush_stroke strokes[];
};
