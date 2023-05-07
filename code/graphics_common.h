#ifndef GRAPHICS_COMMON
#define GRAPHICS_COMMON

struct render_data;
void draw_circle(render_data* rd, real_3 x, real radius, real_4 color);

#define spritesheet_size 4096
#include "rectangle_packer.h"

#define add_sprite(id, filename, n_frames) id,
enum SPRITE_ID
{
    SPR_NONE,
    #include "sprite_list.h"
    N_SPRITES
};
#undef add_sprite

bounding_box_2d sprite_list[N_SPRITES];

#pragma pack(push, 1)
struct sprite_render_info
{
    real_3 x;
    real_2 r;
    real_4 tint;
    real theta;
    real_2 l;
    real_2 u;
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

struct sheet_render_info
{
    real_3 x;
    real_3 r;
    uint16* heights;
};

struct text_render_info
{
    char* text;
    real_3 x;
    real_4 color;
    real_2 alignment;
};

struct ring_render_info
{
    real_3 x;
    real R; //ring center radius
    real r; //girth of the ring
    real_3 axis;
    real_4 color;
};

struct sphere_render_info
{
    real_3 x;
    real r;
    real_4 color;
};

struct line_3d_render_info
{
    real_3 x;
    real length;
    real r;
    real_3 axis;
    real_4 color;
};
#pragma pack(pop)

//TODO: probably should put this somewhere else? I eventually want to be able to handle multiple windows for local multiplayer and stuff
int window_width = 1280;
int window_height = 720;

struct render_data
{
    real fov;
    real_3 camera_pos;
    real_3x3 camera_axes;
    real_4x4 camera; //full matrix with perspective and projection

    circle_render_info* circles;
    uint n_circles;

    rectangle_render_info* rectangles;
    uint n_rectangles;

    sprite_render_info* sprites;
    uint n_sprites;
    uint n_max_sprites;

    line_render_info* line_points;
    uint n_total_line_points; //the total number of line points for all lines
    uint* n_line_points; //list of the number of points in each line
    uint n_lines;

    ring_render_info* rings;
    int n_rings;

    sphere_render_info* spheres;
    int n_spheres;

    line_3d_render_info* line_3ds;
    int n_line_3ds;

    char* debug_log;
    size_t log_pos;

    char* text_data;
    char* next_text;
    text_render_info* texts;
    int n_texts;

    real_4 background_color;
    real_4 foreground_color;
    real_4 highlight_color;
};

void draw_circle(render_data* rd, real_3 x, real radius, real_4 color)
{
    rd->circles[rd->n_circles++] = {
        .x = x,
        .r = radius,
        .color = color,
    };
}

void draw_rectangle(render_data* rd, real_3 x, real_2 radius, real_4 color)
{
    rd->rectangles[rd->n_rectangles++] = {
        .x = x,
        .r = radius,
        .color = color,
    };
}

void draw_sprite(render_data* rd, real_3 x, real_2 radius, real_4 tint, bounding_box_2d sprite_region)
{
    if(rd->n_sprites >= rd->n_max_sprites) return;
    rd->sprites[rd->n_sprites++] = (sprite_render_info){
        .x = x,
        .r = radius,
        .tint = tint,
        .l = real_cast(sprite_region.l)/spritesheet_size,
        .u = real_cast(sprite_region.u)/spritesheet_size,
    };
}

void draw_sprite(render_data* rd, real_3 x, real_2 radius, real_4 tint, real theta, int sprite_id, int animation_index)
{
    if(rd->n_sprites >= rd->n_max_sprites) return;
    bounding_box_2d sprite_region = sprite_list[sprite_id];
    int offset = animation_index*(sprite_region.u.x-sprite_region.l.x);
    sprite_region.l.x += offset;
    sprite_region.u.x += offset;
    rd->sprites[rd->n_sprites++] = (sprite_render_info){
        .x = x,
        .r = radius,
        .tint = tint,
        .theta = theta,
        .l = real_cast(sprite_region.l)/spritesheet_size,
        .u = real_cast(sprite_region.u)/spritesheet_size,
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

void draw_text(render_data* rd, char* text, real_3 x, real_4 color, real_2 alignment)
{
    memcpy(rd->next_text, text, strlen(text)+1);
    rd->texts[rd->n_texts++] = {
        rd->next_text,
        x,
        color,
        alignment,
    };
    rd->next_text += strlen(text)+1;
}

void draw_text(render_data* rd, char* text, real_3 x, real_4 color)
{
    draw_text(rd, text, x, color, {});
}

void draw_ring(render_data* rd, real_3 x, real ring_radius, real profile_radius, real_3 axis, real_4 color)
{
    rd->rings[rd->n_rings++] = {
        x,
        ring_radius,
        profile_radius,
        axis,
        color
    };
}

void draw_sphere(render_data* rd, real_3 x, real radius, real_4 color)
{
    rd->spheres[rd->n_spheres++] = {
        x,
        radius,
        color
    };
}

void draw_line_3d(render_data* rd, real_3 x, real length, real radius, real_3 axis, real_4 color)
{
    rd->line_3ds[rd->n_line_3ds++] = {
        x,
        length,
        radius,
        axis,
        color
    };
}

void draw_line_3d(render_data* rd, real_3 x0, real_3 x1, real radius, real_4 color)
{
    real_3 axis = x1-x0;
    real length = norm(axis);
    axis /= length;
    rd->line_3ds[rd->n_line_3ds++] = {
        x0,
        length,
        radius,
        axis,
        color
    };
}

void draw_box(render_data* rd, real_3 xl, real_3 size, real radius, real_3x3 axes, real_4 color)
{
    real_3 xu = xl+axes*size;

    for(int i = 0; i < 3; i++)
    {
        for(int j = 0; j < 4; j++)
        {
            real_3 offset = size;
            offset[i] = 0;
            offset[(i+1)%3] *= j%2;
            offset[(i+2)%3] *= j/2;
            offset = axes*offset;
            real_3 x = xl+offset;
            draw_line_3d(rd, x, size[i], radius, axes[i], color);
        }
    }
}

void update_camera_matrix(render_data* rd)
{
    real screen_dist = 1.0/tan(rd->fov/2);

    real_4x4 translate = {
        1, 0, 0, -rd->camera_pos.x,
        0, 1, 0, -rd->camera_pos.y,
        0, 0, 1, -rd->camera_pos.z,
        0, 0, 0, 1,
    };

    real_4x4 rotate = {
        rd->camera_axes[0][0], rd->camera_axes[0][1], rd->camera_axes[0][2], 0,
        rd->camera_axes[1][0], rd->camera_axes[1][1], rd->camera_axes[1][2], 0,
        rd->camera_axes[2][0], rd->camera_axes[2][1], rd->camera_axes[2][2], 0,
        0, 0, 0, 1,
    };

    real n = 0.1;
    real f = 1000.0;

    real_4x4 perspective = {
        (screen_dist*window_height)/window_width, 0, 0, 0,
        0, screen_dist, 0, 0,
        0, 0, (f+n)/(f-n), (2*f*n)/(f-n),
        0, 0, -1, 0,
    };

    rd->camera = translate*rotate*perspective;
}

void init_render_data(render_data* rd)
{
    //TODO: non-hardcoded sizes
    rd->circles = (circle_render_info*) stalloc(1000000*sizeof(circle_render_info));
    rd->n_circles = 0;

    rd->rectangles = (rectangle_render_info*) stalloc(1000000*sizeof(rectangle_render_info));
    rd->n_rectangles = 0;

    rd->n_max_sprites = 1000000;
    rd->sprites = (sprite_render_info*) stalloc(rd->n_max_sprites*sizeof(sprite_render_info));
    rd->n_sprites = 0;

    rd->line_points = (line_render_info*) stalloc(10000*sizeof(line_render_info));
    rd->n_total_line_points = 0;
    rd->n_line_points = (uint*) stalloc(1000*sizeof(uint));
    rd->n_lines = 0;

    rd->rings = (ring_render_info*) stalloc(1000000*sizeof(ring_render_info));
    rd->n_rings = 0;

    rd->spheres = (sphere_render_info*) stalloc(1000000*sizeof(sphere_render_info));
    rd->n_spheres = 0;

    rd->line_3ds = (line_3d_render_info*) stalloc(1000000*sizeof(line_3d_render_info));
    rd->n_line_3ds = 0;

    rd->debug_log = (char*) stalloc_clear(4096);

    rd->text_data = (char*) stalloc_clear(65536);
    rd->next_text = rd->text_data;
    rd->texts = (text_render_info*) stalloc_clear(sizeof(text_render_info)*4096);
    rd->n_texts = 0;
}

void start_frame(render_data* rd)
{
    rd->n_circles = 0;
    rd->n_rectangles = 0;
    rd->n_sprites = 0;
    rd->n_total_line_points = 0;
    rd->n_lines = 0;
    rd->n_line_points[0] = 0;
    rd->n_rings = 0;
    rd->n_spheres = 0;
    rd->n_line_3ds = 0;
    rd->log_pos = 0;
    rd->debug_log[0] = 0;
    rd->next_text = rd->text_data;
    rd->n_texts = 0;
}

#endif //GRAPHICS_COMMON
