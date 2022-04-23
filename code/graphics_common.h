#ifndef GRAPHICS_COMMON
#define GRAPHICS_COMMON

#include "game_common.h"

/*NOTE: I should probably be enforcing the packing of these structs, since they are directly copied
  to opengl buffers, but they're all multiples of 4 bytes anyway so it will probably work out*/

struct sprite_render_info
{
    real_3 x;
    real r;
    real_2 u1;
    real_2 u2;
};

struct rectangle_render_info
{
    real_3 x;
    real_2 r;
    real_4 color;
};

struct circle_render_info
{
    real_3 x;
    real r;
    real_4 color;
};

struct line_render_info
{
    real_3 x;
    real r;
    real_4 color;
};

struct spherical_function_render_info
{
    real_3 x;
    real* coefficients;
};

struct blob_render_info
{
    real_3 x;
    int width;
    uint16* heights;
    uint16* bottoms;
};

struct sheet_render_info
{
    real_3 x;
    real_3 r;
    uint16* heights;
};

struct tile_render_info
{
    int x_offset;
    int y_offset;
    uint16* top;
    uint16* bottom;
    int_2* flow;
    uint8_4* properties;
};

//TODO: probably should put this somewhere else? I eventually want to be able to handle multiple windows for local multiplayer and stuff
int window_width = 1;
int window_height = 1;

struct render_data
{
    real_4x4 camera;
    real_3 camera_pos;
    real_3x3 camera_axes;

    circle_render_info* circles;
    uint n_circles;

    line_render_info* line_points;
    uint n_total_line_points; //the total number of line points for all lines
    uint* n_line_points; //list of the number of points in each line
    uint n_lines;

    sheet_render_info* sheets;
    uint n_sheets;

    spherical_function_render_info* spherical_functions;
    uint n_spherical_functions;

    blob_render_info* blobs;
    uint n_blobs;
};

void draw_circle(render_data* rd, real_3 x, real radius, real_4 color)
{
    rd->circles[rd->n_circles++] = {
        .x = x,
        .r = radius,
        .color = color,
    };
}

void add_line_point(render_data* rd, real_3 x, real radius, real_4 color)
{
    rd->line_points[rd->n_total_line_points++] = {
        .x = x,
        .r = radius,
        .color = color,
    };
    rd->n_line_points[rd->n_lines]++;
}

void draw_line(render_data* rd)
{
    rd->n_lines++;
    rd->n_line_points[rd->n_lines] = 0;
}

#endif //GRAPHICS_COMMON
