#ifndef GRAPHICS
#define GRAPHICS

#include <gl/gl.h>
#include <utils/logging.h>
#include <utils/misc.h>

#include "graphics_common.h"
#include "game_common.h"
#include "chunk.h"

#define DEBUG_SEVERITY_HIGH                           0x9146
#define DEBUG_SEVERITY_MEDIUM                         0x9147
#define DEBUG_SEVERITY_LOW                            0x9148

void APIENTRY gl_error_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
    #ifndef GL_SUPPRESS_WARNINGS
    switch(severity)
    {
        case DEBUG_SEVERITY_HIGH:
            assert(0, "HIGH: ", message);
            // log_warning("HIGH: ");
            break;
        case DEBUG_SEVERITY_MEDIUM:
            log_warning("MEDIUM: ");
            break;
        case DEBUG_SEVERITY_LOW:
            log_warning("LOW: ");
            break;
        default:
            log_output("NOTICE: ");
            break;
    }
    log_output(message, "\n\n");
    #endif //GL_SUPPRESS_WARNINGS
}

inline uint gl_get_type_size(GLenum type)
{
    switch(type)
    {
        case GL_BYTE: return sizeof(GLbyte);
        case GL_UNSIGNED_BYTE: return sizeof(GLubyte);
        case GL_SHORT: return sizeof(GLshort);
        case GL_UNSIGNED_SHORT: return sizeof(GLushort);
        case GL_INT: return sizeof(GLint);
        case GL_UNSIGNED_INT: return sizeof(GLuint);
        case GL_HALF_FLOAT: return 16;
        case GL_FLOAT: return sizeof(GLfloat);
        case GL_DOUBLE: return sizeof(GLdouble);
        case GL_FIXED: return sizeof(GLfixed);
        case GL_INT_2_10_10_10_REV: return sizeof(GLint);
        case GL_UNSIGNED_INT_2_10_10_10_REV: return sizeof(GLuint);
        case GL_UNSIGNED_INT_10F_11F_11F_REV: return sizeof(GLuint);
        default: log_error("Unknown GL type: ", type);
    }
}

#define N_GENERAL_BUFFERS 2
#define GENERAL_BUFFER_SIZE (512*megabyte)
GLuint gl_general_buffers[N_GENERAL_BUFFERS];

#define N_MAX_LINE_POINTS 4096
GLuint round_line_texture;

GLuint spherical_function_texture;

GLuint blob_height_texture;
GLuint blob_bottom_texture;

#define layer_render_size (9*grid_tile_size)

GLuint frame_buffer;

#define active_regions_width 16
#define active_regions_size (chunk_size/active_regions_width)

GLuint active_regions_textures[2];
int current_active_regions_texture = 0;

GLuint occupied_regions_texture;
#define occupied_regions_width 16
#define occupied_regions_size (chunk_size/occupied_regions_width)

#define N_MAX_BODIES 4096
GLuint body_buffer;

#define N_MAX_PARTICLES 4096
GLuint particles_x_texture;
GLuint particles_x_dot_texture;

GLuint body_x_dot_texture;
GLuint body_omega_texture;

#define N_MAX_CAPSULES 4096
GLuint capsules_texture;

GLuint collision_point_texture;
GLuint collision_normal_texture;

GLuint prepass_depth_texture;
GLuint color_texture;
GLuint depth_buffer;

int resolution_x = 1280;
int resolution_y = 720;

int prepass_resolution_x = resolution_x/4;
int prepass_resolution_y = resolution_y/4;

GLuint lightprobe_color_textures[2];
GLuint lightprobe_depth_textures[2];
#define lightprobes_w 256
#define lightprobes_h 128
#define lightprobe_resolution 16
#define lightprobe_resolution_x (lightprobes_w*lightprobe_resolution)
#define lightprobe_resolution_y (lightprobes_h*lightprobe_resolution)

GLuint lightprobe_x_textures[2];

GLuint blue_noise_texture;
int blue_noise_w;
int blue_noise_h;

void gl_init_general_buffers(memory_manager* manager)
{
    glGenTextures(1, &prepass_depth_texture);
    glBindTexture(GL_TEXTURE_2D, prepass_depth_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, prepass_resolution_x, prepass_resolution_y);

    glGenTextures(len(lightprobe_color_textures), lightprobe_color_textures);
    for(int i = 0; i < len(lightprobe_color_textures); i++)
    {
        glBindTexture(GL_TEXTURE_2D, lightprobe_color_textures[i]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R11F_G11F_B10F, lightprobe_resolution_x, lightprobe_resolution_y);
        //TODO: can add mip maps to allow for rougher surfaces
    }

    glGenTextures(len(lightprobe_depth_textures), lightprobe_depth_textures);
    for(int i = 0; i < len(lightprobe_depth_textures); i++)
    {
        glBindTexture(GL_TEXTURE_2D, lightprobe_depth_textures[i]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG16F, lightprobe_resolution_x, lightprobe_resolution_y);
    }

    glGenTextures(len(lightprobe_x_textures), lightprobe_x_textures);
    for(int i = 0; i < len(lightprobe_x_textures); i++)
    {
        glBindTexture(GL_TEXTURE_2D, lightprobe_x_textures[i]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, lightprobes_w, lightprobes_h);
    }

    glGenTextures(1, &color_texture);
    glBindTexture(GL_TEXTURE_2D, color_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, resolution_x, resolution_y);

    glGenRenderbuffers(1, &depth_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, resolution_x, resolution_y);

    glGenBuffers(N_GENERAL_BUFFERS, gl_general_buffers);
    for(int i = 0; i < N_GENERAL_BUFFERS; i++)
    {
        glBindBuffer(GL_ARRAY_BUFFER, gl_general_buffers[i]);
        glBufferData(GL_ARRAY_BUFFER, GENERAL_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
    }

    glGenTextures(len(active_regions_textures), active_regions_textures);
    for(int i = 0; i < len(active_regions_textures); i++)
    {
        glBindTexture(GL_TEXTURE_3D, active_regions_textures[i]);
        glTexStorage3D(GL_TEXTURE_3D, 1, GL_R8UI, active_regions_size, active_regions_size, active_regions_size);
        uint clear = 0x1;
        glClearTexImage(active_regions_textures[i], 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &clear);
    }

    glGenTextures(1, &occupied_regions_texture);
    glBindTexture(GL_TEXTURE_3D, occupied_regions_texture);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_R8UI, occupied_regions_size, occupied_regions_size, occupied_regions_size);
    uint clear = 0x0;
    glClearTexImage(occupied_regions_texture, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &clear);

    // glGenTextures(1, &round_line_texture);
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, round_line_texture);
    // glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 2, N_MAX_LINE_POINTS);

    glGenTextures(len(materials_textures), materials_textures);
    for(int i = 0; i < len(materials_textures); i++)
    {
        glBindTexture(GL_TEXTURE_3D, materials_textures[i]);
        glTexStorage3D(GL_TEXTURE_3D, 1, GL_RG32I, chunk_size, chunk_size, chunk_size);
    }

    glGenTextures(len(body_materials_textures), body_materials_textures);
    for(int i = 0; i < len(body_materials_textures); i++)
    {
        glBindTexture(GL_TEXTURE_3D, body_materials_textures[i]);
        glTexStorage3D(GL_TEXTURE_3D, 1, GL_RG32I, body_texture_size, body_texture_size, body_texture_size);
    }

    glGenTextures(1, &body_forces_texture);
    glBindTexture(GL_TEXTURE_3D, body_forces_texture);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGB16F, body_texture_size, body_texture_size, body_texture_size);

    glGenTextures(1, &body_shifts_texture);
    glBindTexture(GL_TEXTURE_3D, body_shifts_texture);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGB16F, body_texture_size, body_texture_size, body_texture_size);

    glGenFramebuffers(1, &frame_buffer);
    // glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

    glGenTextures(1, &particles_x_texture);
    glBindTexture(GL_TEXTURE_2D, particles_x_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, 1, N_MAX_PARTICLES);

    glGenTextures(1, &particles_x_dot_texture);
    glBindTexture(GL_TEXTURE_2D, particles_x_dot_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, 1, N_MAX_PARTICLES);

    glGenTextures(1, &body_x_dot_texture);
    glBindTexture(GL_TEXTURE_2D, body_x_dot_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, 1, N_MAX_BODIES);

    glGenTextures(1, &body_omega_texture);
    glBindTexture(GL_TEXTURE_2D, body_omega_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, 1, N_MAX_BODIES);

    glGenBuffers(1, &body_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, body_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, N_MAX_BODIES*sizeof(gpu_body_data), 0, GL_DYNAMIC_STORAGE_BIT);

    // glGenTextures(1, &capsules_texture);
    // glBindTexture(GL_TEXTURE_2D, capsules_texture);
    // glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, 3, N_MAX_CAPSULES);

    // glGenTextures(1, &collision_point_texture);
    // glBindTexture(GL_TEXTURE_2D, collision_point_texture);
    // glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, chunk_size, chunk_size);

    // glGenTextures(1, &collision_normal_texture);
    // glBindTexture(GL_TEXTURE_2D, collision_normal_texture);
    // glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, chunk_size, chunk_size);

    byte* blue_noise_data = stbi_load("HDR_RGBA_0.png", &blue_noise_w, &blue_noise_h, 0, 4);
    glGenTextures(1, &blue_noise_texture);
    glBindTexture(GL_TEXTURE_2D, blue_noise_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, blue_noise_w, blue_noise_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, blue_noise_data);
    stbi_image_free(blue_noise_data);
}

#define add_attribute_common(dim, gl_type, do_normalize, stride, offset_advance, divisor) \
    glEnableVertexAttribArray(layout_location);                         \
    glVertexAttribPointer(layout_location, dim, gl_type, do_normalize, stride, (void*) (offset)); \
    offset += offset_advance;                                           \
    glVertexAttribDivisor(layout_location++, divisor);

#define add_contiguous_attribute(dim, gl_type, do_normalize, stride, offset_advance) \
    add_attribute_common(dim, gl_type, do_normalize, stride, offset_advance, 0);

#define add_interleaved_attribute(dim, gl_type, do_normalize, stride, divisor) \
    add_attribute_common(dim, gl_type, do_normalize, stride, dim*gl_get_type_size(gl_type), divisor);

#include "generated_shader_list.h"

void draw_to_screen(GLuint texture)
{
    glUseProgram(fullscreen_texture_program);

    int texture_i = 0;
    int uniform_i = 0;

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    size_t offset = 0;
    int layout_location = 0;

    //buffer square coord data (bounding box of circle)
    real vb[] = {-1,-1,0,
        -1,+1,0,
        +1,+1,0,
        +1,-1,0};
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);

    add_contiguous_attribute(3, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}


// void draw_sprites(GLuint spritesheet_texture, sprite_render_info* sprites, int n_sprites, real_4x4 camera)
// {
//     GLuint sprite_buffer = gl_general_buffers[0];

//     glUseProgram(sprite_program);

//     glUniformMatrix4fv(0, 1, true, (GLfloat*) &camera);
//     glUniform1f(1, spritesheet_texture);

//     glBindBuffer(GL_ARRAY_BUFFER, sprite_buffer);

//     size_t offset = 0;
//     int layout_location = 0;

//     //buffer square coord data (bounding box of sprite)
//     real vb[] = {-1,-1,0,
//         -1,+1,0,
//         +1,+1,0,
//         +1,-1,0};
//     glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);

//     add_contiguous_attribute(3, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

//     //buffer sprite data
//     glBufferSubData(GL_ARRAY_BUFFER, offset, n_sprites*sizeof(sprites[0]), (void*) sprites);
//     add_interleaved_attribute(3, GL_FLOAT, false, sizeof(sprites[0]), 1); //center position
//     add_interleaved_attribute(1, GL_FLOAT, false, sizeof(sprites[0]), 1); //radius
//     add_interleaved_attribute(2, GL_FLOAT, false, sizeof(sprites[0]), 1); //lower texture coord
//     add_interleaved_attribute(2, GL_FLOAT, false, sizeof(sprites[0]), 1); //upper texture coordinate

//     glDisable(GL_DEPTH_TEST);
//     glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, n_sprites);
//     glEnable(GL_DEPTH_TEST);
// }

void draw_circles(circle_render_info* circles, int n_circles, real_4x4 camera)
{
    GLuint circle_buffer = gl_general_buffers[0];

    glUseProgram(circle_program);

    glUniformMatrix4fv(0, 1, true, (GLfloat*) &camera);
    float smoothness = 3.0/(window_height);
    glUniform1f(1, smoothness);

    glBindBuffer(GL_ARRAY_BUFFER, circle_buffer);

    size_t offset = 0;
    int layout_location = 0;

    //buffer square coord data (bounding box of circle)
    real vb[] = {-1,-1,0,
        -1,+1,0,
        +1,+1,0,
        +1,-1,0};
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);

    add_contiguous_attribute(3, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

    //buffer circle data
    glBufferSubData(GL_ARRAY_BUFFER, offset, n_circles*sizeof(circles[0]), (void*) circles);
    add_interleaved_attribute(3, GL_FLOAT, false, sizeof(circles[0]), 1); //center position
    add_interleaved_attribute(1, GL_FLOAT, false, sizeof(circles[0]), 1); //radius
    add_interleaved_attribute(4, GL_FLOAT, false, sizeof(circles[0]), 1); //RGBA color

    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, n_circles);
}

// void draw_rectangles(rectangle_render_info* rectangles, int n_rectangles, real_4x4* camera)
// {
//     GLuint rectangle_buffer = gl_general_buffers[0];

//     glUseProgram(rectangle_program);

//     glUniformMatrix4fv(0, 1, false, (GLfloat*) camera);
//     float smoothness = 3.0/(window_height);
//     glUniform1f(1, smoothness);

//     glBindBuffer(GL_ARRAY_BUFFER, rectangle_buffer);

//     size_t offset = 0;
//     int layout_location = 0;

//     //buffer square coord data (bounding box of circle)
//     real vb[] = {-1,-1,0,
//         -1,+1,0,
//         +1,+1,0,
//         +1,-1,0};
//     glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);

//     add_contiguous_attribute(3, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

//     //buffer rectangle data
//     glBufferSubData(GL_ARRAY_BUFFER, offset, n_rectangles*sizeof(rectangles[0]), (void*) rectangles);
//     add_interleaved_attribute(3, GL_FLOAT, false, sizeof(rectangles[0]), 1); //center position
//     add_interleaved_attribute(2, GL_FLOAT, false, sizeof(rectangles[0]), 1); //radius
//     add_interleaved_attribute(4, GL_FLOAT, false, sizeof(rectangles[0]), 1); //RGBA color

//     glDisable(GL_DEPTH_TEST);
//     glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, n_rectangles);
//     glEnable(GL_DEPTH_TEST);
// }

// struct line_render_element
// {
//     real_3 X;
// };

// void draw_round_line(memory_manager* manager, line_render_info* points, int n_points, real_4x4 camera)
// {
//     GLuint buffer = gl_general_buffers[0];

//     glUseProgram(round_line_program);

//     float smoothness = 3.0/(window_height);
//     glUniformMatrix4fv(0, 1, true, (GLfloat*) &camera); //t
//     glUniform1f(1, smoothness);

//     glBindBuffer(GL_ARRAY_BUFFER, buffer);

//     size_t offset = 0;
//     int layout_location = 0;

//     line_render_element* corners = (line_render_element*) reserve_block(manager, 2*n_points*sizeof(line_render_element)); //xyz, u
//     int n_corners = 0;

//     real R = 0.0;
//     for(int i = 0; i < n_points; i++)
//     {
//         if(points[i].r > R) R = points[i].r;
//     }
//     R *= 2.0; //TODO: this is a hack to fix non 2d lines

//     real_3 normal_dir = {0,0,1};

//     real_3 v1 = normalize(points[1].x-points[0].x);
//     real_3 side = normalize(cross(v1, normal_dir))*R;

//     corners[n_corners++].X = points[0].x-side-v1*R;
//     corners[n_corners++].X = points[0].x+side-v1*R;

//     for(int i = 1; i < n_points-1; i++)
//     {
//         if(points[i+1].x == points[i].x) continue;
//         real_3 v2 = normalize(points[i+1].x-points[i].x);
//         real_3 new_normal_dir = cross(v1, v2);
//         if(normsq(new_normal_dir) > 1.0e-18) new_normal_dir = normalize(new_normal_dir);
//         else new_normal_dir = {0, 0, 1};
//         normal_dir = (dot(new_normal_dir, normal_dir) > 0) ? new_normal_dir : -new_normal_dir;
//         real_3 Cto_corner = (v1-v2);
//         real Ccoshalftheta = dot(normalize(cross(v1, normal_dir)), Cto_corner);
//         side = Cto_corner*(R/Ccoshalftheta);
//         if(Ccoshalftheta == 0.0) side = normalize(cross(v1, normal_dir))*R;

//         corners[n_corners++].X = points[i].x-side;
//         corners[n_corners++].X = points[i].x+side;

//         v1 = v2; //NOTE: make sure this gets unrolled properly
//     }
//     //TODO: handle wenis seams

//     normal_dir = {0,0,1};
//     side = normalize(cross(v1, normal_dir))*R;
//     corners[n_corners++].X = points[n_points-1].x-side+v1*R;
//     corners[n_corners++].X = points[n_points-1].x+side+v1*R;

//     //buffer circle data
//     //TODO: check packing of the c structs
//     glBufferSubData(GL_ARRAY_BUFFER, offset, n_corners*sizeof(corners[0]), (void*) corners);
//     add_interleaved_attribute(3, GL_FLOAT, false, sizeof(corners[0]), 0); //vertex position

//     glUniform1i(2, 0);
//     glActiveTexture(GL_TEXTURE0);
//     glBindTexture(GL_TEXTURE_2D, round_line_texture);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

//     glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 2, n_points, GL_RGBA, GL_FLOAT, points);

//     glUniform1i(3, n_points);

//     glEnable(GL_DEPTH_TEST);
//     glDrawArrays(GL_TRIANGLE_STRIP, 0, n_corners);

//     unreserve_block(manager);
// }

void render_depth_prepass(GLuint materials_texture, real_3x3 camera_axes, real_3 camera_pos, int_3 size, int_3 origin)
{
    glUseProgram(render_depth_prepass_program);

    int texture_i = 0;
    int uniform_i = 0;

    static int frame_number = 0;
    glUniform1i(uniform_i++, frame_number++);

    //camera
    glUniformMatrix3fv(uniform_i++, 1, false, (GLfloat*) &camera_axes);
    glUniform3fv(uniform_i++, 1, (GLfloat*) &camera_pos);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, materials_texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, occupied_regions_texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform3iv(uniform_i++, 1, (GLint*) &size);
    glUniform3iv(uniform_i++, 1, (GLint*) &origin);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, blue_noise_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    size_t offset = 0;
    int layout_location = 0;

    GLuint vertex_buffer = gl_general_buffers[0];
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    real vb[] = {-1,-1,0,
        -1,+1,0,
        +1,+1,0,
        +1,-1,0};
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);
    add_contiguous_attribute(3, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, prepass_depth_texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);

    glViewport(0, 0, prepass_resolution_x, prepass_resolution_y);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
}

// void update_lightmap()
// {
//     glUseProgram(update_lightmap_program);

//     int texture_i = 0;
//     int uniform_i = 0;

//     static int frame_number = 0;
//     glUniform1i(uniform_i++, frame_number++);

//     //camera
//     glUniformMatrix3fv(uniform_i++, 1, false, (GLfloat*) &camera_axes);
//     glUniform3fv(uniform_i++, 1, (GLfloat*) &camera_pos);

//     glUniform1i(uniform_i++, texture_i);
//     glActiveTexture(GL_TEXTURE0+texture_i++);
//     glBindTexture(GL_TEXTURE_3D, materials_texture);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

//     glUniform1i(uniform_i++, texture_i);
//     glActiveTexture(GL_TEXTURE0+texture_i++);
//     glBindTexture(GL_TEXTURE_3D, occupied_regions_texture);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

//     glUniform3iv(uniform_i++, 1, (GLint*) &size);
//     glUniform3iv(uniform_i++, 1, (GLint*) &origin);

//     glUniform1i(uniform_i++, texture_i);
//     glActiveTexture(GL_TEXTURE0+texture_i++);
//     glBindTexture(GL_TEXTURE_2D, blue_noise_texture);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

//     size_t offset = 0;
//     int layout_location = 0;

//     GLuint vertex_buffer = gl_general_buffers[0];
//     glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
//     real vb[] = {-1,-1,0,
//         -1,+1,0,
//         +1,+1,0,
//         +1,-1,0};
//     glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);
//     add_contiguous_attribute(3, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

//     glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
//     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, prepass_depth_texture, 0);
//     glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);

//     glViewport(0, 0, prepass_resolution_x, prepass_resolution_y);

//     glDisable(GL_DEPTH_TEST);
//     glDisable(GL_BLEND);
//     glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
//     glEnable(GL_DEPTH_TEST);
//     glEnable(GL_BLEND);
// }

void render_voxels(GLuint materials_texture, real_3x3 camera_axes, real_3 camera_pos, int_3 size, int_3 origin)
{
    // render_depth_prepass(materials_texture, camera_axes, camera_pos, size, origin);

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
    glViewport(0, 0, resolution_x, resolution_y);

    glUseProgram(render_chunk_program);

    int texture_i = 0;
    int uniform_i = 0;

    //camera
    glUniformMatrix3fv(uniform_i++, 1, false, (GLfloat*) &camera_axes);
    glUniform3fv(uniform_i++, 1, (GLfloat*) &camera_pos);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, materials_texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, occupied_regions_texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform3iv(uniform_i++, 1, (GLint*) &size);
    glUniform3iv(uniform_i++, 1, (GLint*) &origin);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, prepass_depth_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    size_t offset = 0;
    int layout_location = 0;

    GLuint vertex_buffer = gl_general_buffers[0];
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    real vb[] = {-1,-1,0,
        -1,+1,0,
        +1,+1,0,
        +1,-1,0};
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);
    add_contiguous_attribute(3, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void render_chunk(chunk* c, real_3x3 camera_axes, real_3 camera_pos)
{
    load_chunk_to_gpu(c);
    render_voxels(materials_textures[current_materials_texture], camera_axes, camera_pos, {chunk_size, chunk_size, chunk_size}, {0,0,0});
}

void render_body(cpu_body_data* bc, gpu_body_data* bg, real_3x3 camera_axes, real_3 camera_pos)
{
    load_body_to_gpu(bc, bg);
    // glDisable(GL_DEPTH_TEST);
    render_voxels(body_materials_textures[current_body_materials_texture], apply_rotation(conjugate(bg->orientation), camera_axes), apply_rotation(conjugate(bg->orientation), camera_pos - bg->x)+bg->x_cm, bg->size, bg->materials_origin); //TODO: proper bounds and shit
    // glEnable(GL_DEPTH_TEST);
}

void simulate_chunk(chunk* c, int update_cells)
{
    glUseProgram(simulate_chunk_program);

    load_chunk_to_gpu(c);

    int texture_i = 0;
    int image_i = 0;
    int uniform_i = 0;

    int layer_number_uniform = uniform_i++;

    static int frame_number = 0;
    glUniform1i(uniform_i++, frame_number++);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, materials_textures[current_materials_texture]);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, active_regions_textures[current_active_regions_texture]);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // glBindImageTexture(image_i++, active_regions_textures[current_active_regions_texture], 0, true, 0, GL_READ_ONLY, GL_R32UI);

    glUniform1i(uniform_i++, image_i);
    glBindImageTexture(image_i++, active_regions_textures[1-current_active_regions_texture], 0, true, 0, GL_WRITE_ONLY, GL_R8UI);
    uint clear = 0;
    glClearTexImage(active_regions_textures[1-current_active_regions_texture], 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &clear);

    glUniform1i(uniform_i++, image_i);
    glBindImageTexture(image_i++, occupied_regions_texture, 0, true, 0, GL_WRITE_ONLY, GL_R8UI);
    // glCopyImageSubData(active_regions_textures[current_active_regions_texture], GL_TEXTURE_3D,
    //                    0,
    //                    0, 0, 0,
    //                    occupied_regions_texture, GL_TEXTURE_3D,
    //                    0,
    //                    0, 0, 0,
    //                    occupied_regions_size, occupied_regions_size, occupied_regions_size);

    //swap active regions textures
    current_active_regions_texture = 1-current_active_regions_texture;

    glUniform1i(uniform_i++, update_cells);

    size_t offset = 0;
    int layout_location = 0;

    GLuint vertex_buffer = gl_general_buffers[0];
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    real vb[] = {0,0,
        0,1,
        1,1,
        1,0};
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);
    add_contiguous_attribute(2, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

    real region_pos[2*32*32] = {};
    for(int i = 0; i < 32*32; i++)
    {
        region_pos[2*i+0] = i%32;
        region_pos[2*i+1] = (i/32)%32;
    }
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(region_pos), (void*) region_pos);
    add_interleaved_attribute(2, GL_FLOAT, false, 2*sizeof(region_pos[0]), 1);

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    glViewport(0, 0, chunk_size, chunk_size);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    for(int l = 0; l < chunk_size; l++)
    {
        glUniform1i(layer_number_uniform, l);

        glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D,
                               materials_textures[1-current_materials_texture], 0, l);

        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, 32*32);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    current_materials_texture = 1-current_materials_texture;
}

void simulate_particles(chunk* c, real_3* xs, real_3* x_dots, int n_particles)
{
    glUseProgram(simulate_particles_program);

    int texture_i = 0;
    int uniform_i = 0;

    static int frame_number = 0;
    glUniform1i(uniform_i++, frame_number++);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, materials_textures[current_materials_texture]);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, particles_x_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, particles_x_dot_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    size_t offset = 0;
    int layout_location = 0;

    GLuint vertex_buffer = gl_general_buffers[0];
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    real vb[] = {-1,-1,0,
        -1,+1,0,
        +1,+1,0,
        +1,-1,0};
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);
    add_contiguous_attribute(3, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    glViewport(0, 0, 1, n_particles);

    glBindTexture(GL_TEXTURE_2D, particles_x_texture);
    glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    0, 0,
                    1, n_particles,
                    GL_RGB, GL_FLOAT, xs);

    glBindTexture(GL_TEXTURE_2D, particles_x_dot_texture);
    glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    0, 0,
                    1, n_particles,
                    GL_RGB, GL_FLOAT, x_dots);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, particles_x_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, particles_x_dot_texture, 0);

    GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(len(buffers), buffers);

    glDisable(GL_BLEND);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glEnable(GL_BLEND);

    glGetTextureSubImage(particles_x_texture,
                         0,
                         0, 0, 0,
                         1, n_particles, 1,
                         GL_RGB,
                         GL_FLOAT,
                         sizeof(xs[0])*n_particles,
                         xs);

    glGetTextureSubImage(particles_x_dot_texture,
                         0,
                         0, 0, 0,
                         1, n_particles, 1,
                         GL_RGB,
                         GL_FLOAT,
                         sizeof(x_dots[0])*n_particles,
                         x_dots);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
}

void simulate_bodies(cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, int n_bodies)
{
    glUseProgram(simulate_body_program);

    int texture_i = 0;
    int image_i = 0;
    int uniform_i = 0;

    int layer_number_uniform = uniform_i++;

    static int frame_number = 0;
    glUniform1i(uniform_i++, frame_number++);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, materials_textures[current_materials_texture]);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, body_materials_textures[current_body_materials_texture]);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glUniform1i(uniform_i++, n_bodies);

    size_t offset = 0;
    int layout_location = 0;

    GLuint vertex_buffer = gl_general_buffers[0];
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    real vb[] = {-0,-0,0,
        -0,+1,0,
        +1,+1,0,
        +1,-0,0};
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);
    add_contiguous_attribute(3, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    glViewport(0, 0, body_texture_size, body_texture_size);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    GLenum buffers[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(len(buffers), buffers);

    for(int l = 0; l < body_texture_size; l++)
    {
        glUniform1i(layer_number_uniform, l);

        glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D,
                               body_materials_textures[1-current_body_materials_texture], 0, l);

        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, n_bodies);
    }

    glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, 0, 0, 0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    current_body_materials_texture = 1-current_body_materials_texture;
}

void simulate_body_physics(memory_manager* manager, cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, int n_bodies, render_data* rd)
{
    glUseProgram(simulate_body_physics_program);

    int texture_i = 0;
    int uniform_i = 0;

    static int frame_number = 0;
    glUniform1i(uniform_i++, frame_number++);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, materials_textures[current_materials_texture]);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, body_materials_textures[current_body_materials_texture]);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, body_forces_texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, body_shifts_texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, body_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(bodies_gpu[0])*n_bodies, bodies_gpu);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, body_buffer);

    size_t offset = 0;
    int layout_location = 0;

    GLuint vertex_buffer = gl_general_buffers[0];
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    real vb[] = {-1,-1,0,
        -1,+1,0,
        +1,+1,0,
        +1,-1,0};
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);
    add_contiguous_attribute(3, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    glViewport(0, 0, 1, n_bodies);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, body_x_dot_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, body_omega_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, particles_x_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, particles_x_dot_texture, 0);

    GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glDrawBuffers(len(buffers), buffers);

    glDisable(GL_BLEND);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glEnable(GL_BLEND);

    size_t out_size = n_bodies*sizeof(float)*3;
    byte* out_data = reserve_block(manager, 4*out_size);
    real_3* deltax_dots = (real_3*) out_data;
    real_3* deltaomegas     = (real_3*) (out_data+out_size);
    real_3* pseudo_x_dots = (real_3*) (out_data+2*out_size);
    real_3* pseudo_omegas     = (real_3*) (out_data+3*out_size);

    glGetTextureSubImage(body_x_dot_texture,
                         0,
                         0, 0, 0,
                         1, n_bodies, 1,
                         GL_RGB,
                         GL_FLOAT,
                         sizeof(deltax_dots[0])*n_bodies,
                         deltax_dots);

    glGetTextureSubImage(body_omega_texture,
                         0,
                         0, 0, 0,
                         1, n_bodies, 1,
                         GL_RGB,
                         GL_FLOAT,
                         sizeof(deltaomegas[0])*n_bodies,
                         deltaomegas);

    glGetTextureSubImage(particles_x_texture,
                         0,
                         0, 0, 0,
                         1, n_bodies, 1,
                         GL_RGB,
                         GL_FLOAT,
                         sizeof(pseudo_x_dots[0])*n_bodies,
                         pseudo_x_dots);

    glGetTextureSubImage(particles_x_dot_texture,
                         0,
                         0, 0, 0,
                         1, n_bodies, 1,
                         GL_RGB,
                         GL_FLOAT,
                         sizeof(pseudo_omegas[0])*n_bodies,
                         pseudo_omegas);

    for(int b = 0; b < n_bodies; b++)
    {
        // draw_circle(rd, x_dots[b], 1.0, {1,0,0,1});
        // draw_circle(rd, omegas[b], 1.0, {0,1,0,1});
        draw_circle(rd, bodies_gpu[b].x, 0.2, {0,1,0,1});
        draw_circle(rd, bodies_gpu[b].x+20*(deltax_dots[b]), 0.2, {1,0,0,1});
        bodies_gpu[b].x_dot += deltax_dots[b];
        // bodies_gpu[b].omega     += bodies_gpu[b].m*bodies_gpu[b].invI*cross(rs[b], deltax_dots[b]);
        bodies_gpu[b].omega     += deltaomegas[b];

        bodies_cpu[b].last_contact_point = deltaomegas[b];

        bodies_gpu[b].x += pseudo_x_dots[b];
        real half_angle = 0.5*norm(pseudo_omegas[b]);
        if(half_angle > 0.0001)
        {
            real_3 axis = normalize(pseudo_omegas[b]);
            quaternion rotation = (quaternion){cos(half_angle), sin(half_angle)*axis.x, sin(half_angle)*axis.y, sin(half_angle)*axis.z};
            bodies_gpu[b].orientation = rotation*bodies_gpu[b].orientation;
        }
        update_inertia(&bodies_cpu[b], &bodies_gpu[b]);
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, 0, 0);
    unreserve_block(manager);

    glDrawBuffers(1, buffers);
}

// void compute_body_bounding_box(gpu_body_data* b)
// {
//     real_3 bbl = {0,0,0};
//     real_3 bbu = {0,0,0};
//     for(int i = 0; i < 8; i++)
//     {
//         int_3 dir = {
//             (i>>0)&1,
//             (i>>1)&1,
//             (i>>2)&1
//         };
//         real_3 corner = apply_rotation(b->orientation, -b->x_cm+real_cast(multiply_components(b->size, dir)));
//         bbl = min_per_axis(bbl, corner);
//         bbu = max_per_axis(bbu, corner);
//     }
//     bbl += b->x;
//     bbu += b->x;
// }

void get_chunk_data(chunk* c, int_3 x, int_3 dim, uint16* data)
{
    glGetTextureSubImage(materials_textures[current_materials_texture],
                         0,
                         x.x, x.y, x.z,
                         dim.x, dim.y, dim.z,
                         GL_RED,
                         GL_UNSIGNED_SHORT,
                         dim.x*dim.y*dim.z*sizeof(uint16)*2,
                         data);
}

#endif
