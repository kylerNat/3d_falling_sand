#ifndef GRAPHICS_COMMON
#define GRAPHICS_COMMON

struct render_data;
void draw_circle(render_data* rd, real_3 x, real radius, real_4 color);

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

    char* debug_log;
    size_t log_pos;

    char* text_data;
    char* next_text;
    text_render_info* texts;
    int n_texts;
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

void draw_text(render_data* rd, char* text, real_3 x, real_4 color)
{
    memcpy(rd->next_text, text, strlen(text)+1);
    rd->texts[rd->n_texts++] = {
        rd->next_text,
        x,
        color,
    };
    rd->next_text += strlen(text)+1;
}


#endif //GRAPHICS_COMMON
