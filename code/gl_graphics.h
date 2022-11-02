#ifndef GRAPHICS
#define GRAPHICS

#include <gl/gl.h>
#include <utils/logging.h>
#include <utils/misc.h>
#include <stb/stb_easy_font.h>

void STB_assert(int x)
{
    assert(x);
}
#define STBTT_assert(x) STB_assert(x)
#define STBRP_ASSERT(x) STB_assert(x)
#define STB_RECT_PACK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_rect_pack.h"
#include "stb/stb_truetype.h"

#include "graphics_common.h"
#include "game_common.h"
#include "chunk.h"

#include "materials.h"

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
GLuint body_chunks_buffer;

#define N_MAX_FORMS 4096
GLuint form_buffer;

GLuint form_materials_texture;

#define N_MAX_PARTICLES 4096
GLuint particle_buffer;

GLuint body_x_dot_texture;
GLuint body_omega_texture;

GLuint contact_point_textures[3];
GLuint contact_normal_textures[3];
GLuint contact_material_texture;

GLuint prepass_x_texture;
GLuint prepass_orientation_texture;
GLuint prepass_voxel_texture;
GLuint prepass_color_texture;

GLuint color_texture;
GLuint depth_texture;
GLuint normal_texture;

GLuint denoised_color_texture;

GLuint stencil_buffer;

int resolution_x = 1280;
int resolution_y = 720;

int prepass_resolution_x = resolution_x/2;
int prepass_resolution_y = resolution_y/2;

GLuint lightprobe_color_textures[2];
GLuint lightprobe_depth_textures[2];
#include "shaders/include/lightprobe_header.glsl" //lightprobe constants

int current_lightprobe_texture = 0;
int current_lightprobe_x_texture = 0;

GLuint lightprobe_x_textures[2];

GLuint spritesheet_texture;

GLuint blue_noise_texture;
int blue_noise_w;
int blue_noise_h;

#define font_resolution 1024

struct font_info
{
    GLuint texture;
    stbtt_packedchar* char_data;
    real size;
};

font_info init_font(memory_manager* manager, char* font_filename, real font_size)
{
    uint8* font_data = (uint8*) load_file_0_terminated(manager, font_filename);
    int error = 0;

    stbtt_pack_context spc = {};

    stbtt_pack_range ranges[] = {
        {
            .font_size = font_size,
            .first_unicode_codepoint_in_range = 0,
            .num_chars = 256,
            .chardata_for_range = (stbtt_packedchar*) permalloc(manager, sizeof(stbtt_packedchar)*256),
        },
    };

    uint8* pixels = (uint8*) reserve_block(manager, 4*sq(font_resolution));

    error = stbtt_PackBegin(&spc, pixels, font_resolution, font_resolution, 0, 1, 0);
    assert(error);


    stbtt_PackSetOversampling(&spc, 2, 2);
    error = stbtt_PackFontRanges(&spc, font_data, 0, ranges, len(ranges));
    assert(error);

    stbtt_PackEnd(&spc);

    GLuint font_texture;
    glGenTextures(1, &font_texture);
    glBindTexture(GL_TEXTURE_2D, font_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, font_resolution, font_resolution, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);

    unreserve_block(manager);

    return {font_texture, ranges[0].chardata_for_range, font_size};
}

void gl_init_general_buffers(memory_manager* manager)
{
    glGenTextures(1, &prepass_x_texture);
    glBindTexture(GL_TEXTURE_2D, prepass_x_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, prepass_resolution_x, prepass_resolution_y);

    glGenTextures(1, &prepass_orientation_texture);
    glBindTexture(GL_TEXTURE_2D, prepass_orientation_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, prepass_resolution_x, prepass_resolution_y);

    glGenTextures(1, &prepass_voxel_texture);
    glBindTexture(GL_TEXTURE_2D, prepass_voxel_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16UI, prepass_resolution_x, prepass_resolution_y);

    glGenTextures(1, &prepass_color_texture);
    glBindTexture(GL_TEXTURE_2D, prepass_color_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, prepass_resolution_x, prepass_resolution_y);

    glGenTextures(len(lightprobe_color_textures), lightprobe_color_textures);
    for(int i = 0; i < len(lightprobe_color_textures); i++)
    {
        glBindTexture(GL_TEXTURE_2D, lightprobe_color_textures[i]);
        // glTexStorage2D(GL_TEXTURE_2D, 1, GL_R11F_G11F_B10F, lightprobe_resolution_x, lightprobe_resolution_y);
        glTexStorage2D(GL_TEXTURE_2D, 2, GL_RGBA16F, lightprobe_resolution_x, lightprobe_resolution_y);
        //TODO: can add mip maps to allow for rougher surfaces

        real_3 clear = {0.05,0.05,0.05};
        glClearTexImage(lightprobe_color_textures[i], 0, GL_RGB, GL_FLOAT, &clear);
    }

    glGenTextures(len(lightprobe_depth_textures), lightprobe_depth_textures);
    for(int i = 0; i < len(lightprobe_depth_textures); i++)
    {
        glBindTexture(GL_TEXTURE_2D, lightprobe_depth_textures[i]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG16F, lightprobe_resolution_x, lightprobe_resolution_y);

        real_3 clear = {0,0,0};
        glClearTexImage(lightprobe_depth_textures[i], 0, GL_RGB, GL_FLOAT, &clear);
    }

    glGenTextures(len(lightprobe_x_textures), lightprobe_x_textures);
    real_3* image = (real_3*) permalloc(manager, lightprobes_h*lightprobes_w*sizeof(real_3));
    for(int j = 0; j < lightprobes_w*lightprobes_h; j++)
    {
        image[j] = {(lightprobe_spacing/2)+lightprobe_spacing*(j%lightprobes_per_axis), (lightprobe_spacing/2)+lightprobe_spacing*((j/lightprobes_per_axis)%lightprobes_per_axis), (lightprobe_spacing/2)+lightprobe_spacing*(j/sq(lightprobes_per_axis))};
    }
    for(int i = 0; i < len(lightprobe_x_textures); i++)
    {
        glBindTexture(GL_TEXTURE_2D, lightprobe_x_textures[i]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, lightprobes_w, lightprobes_h);

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, lightprobes_w, lightprobes_h, GL_RGB, GL_FLOAT, image);
    }
    // unreserve_block(manager);

    glGenTextures(1, &denoised_color_texture);
    glBindTexture(GL_TEXTURE_2D, denoised_color_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, resolution_x, resolution_y);

    glGenTextures(1, &color_texture);
    glBindTexture(GL_TEXTURE_2D, color_texture);
    glTexStorage2D(GL_TEXTURE_2D, 8, GL_RGB32F, resolution_x, resolution_y);

    glGenTextures(1, &normal_texture);
    glBindTexture(GL_TEXTURE_2D, normal_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, resolution_x, resolution_y);

    // glGenRenderbuffers(1, &depth_buffer);
    // glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
    // glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, resolution_x, resolution_y);

    glGenTextures(1, &depth_texture);
    glBindTexture(GL_TEXTURE_2D, depth_texture);
    glTexStorage2D(GL_TEXTURE_2D, 8, GL_DEPTH_COMPONENT32, resolution_x, resolution_y);

    glGenRenderbuffers(1, &stencil_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, stencil_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, lightprobe_resolution_x, lightprobe_resolution_y);

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

    glGenTextures(1, &material_visual_properties_texture);
    glBindTexture(GL_TEXTURE_2D, material_visual_properties_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, 4, N_MAX_MATERIALS);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 3, N_MAX_MATERIALS, 0, GL_RGB, GL_FLOAT, materials);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, N_MAX_MATERIALS, GL_RGB, GL_FLOAT, material_visuals);

    glGenTextures(1, &material_physical_properties_texture);
    glBindTexture(GL_TEXTURE_2D, material_physical_properties_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, N_PHYSICAL_PROPERTIES, N_MAX_MATERIALS);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 3, N_MAX_MATERIALS, 0, GL_RGB, GL_FLOAT, materials);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, N_PHYSICAL_PROPERTIES, N_MAX_MATERIALS, GL_RED, GL_FLOAT, material_physicals);

    // glGenBuffers(1, &material_visual_properties_buffer);
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, material_visual_properties_buffer);
    // glBufferStorage(GL_SHADER_STORAGE_BUFFER, N_MAX_MATERIALS*sizeof(material_visual_info), 0, GL_DYNAMIC_STORAGE_BIT);

    // glGenBuffers(1, &material_physical_properties_buffer);
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, material_physical_properties_buffer);
    // glBufferStorage(GL_SHADER_STORAGE_BUFFER, N_MAX_MATERIALS*sizeof(material_physical_info), 0, GL_DYNAMIC_STORAGE_BIT);

    glGenTextures(len(materials_textures), materials_textures);
    for(int i = 0; i < len(materials_textures); i++)
    {
        glBindTexture(GL_TEXTURE_3D, materials_textures[i]);
        glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8UI, chunk_size, chunk_size, chunk_size);
    }

    glGenTextures(len(body_materials_textures), body_materials_textures);
    for(int i = 0; i < len(body_materials_textures); i++)
    {
        glBindTexture(GL_TEXTURE_3D, body_materials_textures[i]);
        glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8UI, body_texture_size, body_texture_size, body_texture_size);
    }

    glGenTextures(1, &form_materials_texture);
    glBindTexture(GL_TEXTURE_3D, form_materials_texture);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_R8UI, form_texture_size, form_texture_size, form_texture_size);

    glGenFramebuffers(1, &frame_buffer);
    // glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

    glGenTextures(1, &body_x_dot_texture);
    glBindTexture(GL_TEXTURE_2D, body_x_dot_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, 1, N_MAX_BODIES);

    glGenTextures(1, &body_omega_texture);
    glBindTexture(GL_TEXTURE_2D, body_omega_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, 1, N_MAX_BODIES);

    glGenTextures(3, contact_point_textures);
    for(int i = 0; i < 3; i++)
    {
        glBindTexture(GL_TEXTURE_2D, contact_point_textures[i]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, 1, N_MAX_BODIES);
    }

    glGenTextures(3, contact_normal_textures);
    for(int i = 0; i < 3; i++)
    {
        glBindTexture(GL_TEXTURE_2D, contact_normal_textures[i]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, 1, N_MAX_BODIES);
    }

    glGenTextures(1, &contact_material_texture);
    glBindTexture(GL_TEXTURE_2D, contact_material_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16I, 1, N_MAX_BODIES);

    glGenBuffers(1, &body_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, body_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, N_MAX_BODIES*sizeof(gpu_body_data), 0, GL_DYNAMIC_STORAGE_BIT);

    glGenBuffers(1, &body_chunks_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, body_chunks_buffer);
    int body_chunks_wide = body_texture_size/body_chunk_size;
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, body_chunks_wide*body_chunks_wide*body_chunks_wide, 0, GL_DYNAMIC_STORAGE_BIT);

    glGenBuffers(1, &form_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, form_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, N_MAX_FORMS*sizeof(gpu_form_data), 0, GL_DYNAMIC_STORAGE_BIT);

    glGenBuffers(1, &particle_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER,
                    sizeof(int)+N_MAX_PARTICLES*(sizeof(particle_data)+sizeof(int)),
                    0, GL_DYNAMIC_STORAGE_BIT);
    //not sure if it really matters but this puts 0 at the top of the stack
    int dead_particle_init[N_MAX_PARTICLES];
    for(int i = 0; i < N_MAX_PARTICLES; i++)
        dead_particle_init[i] = N_MAX_PARTICLES-1-i;
    int n_dead_particles = N_MAX_PARTICLES;
    particle_data particles[N_MAX_PARTICLES] = {};
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &n_dead_particles);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(int), N_MAX_PARTICLES*sizeof(particle_data), &particles);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(int)+N_MAX_PARTICLES*sizeof(particle_data),
                    N_MAX_PARTICLES*sizeof(int), dead_particle_init);

    glGenTextures(1, &spritesheet_texture);
    glBindTexture(GL_TEXTURE_2D, spritesheet_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8I, 4096, 4096);

    byte* blue_noise_data = stbi_load("HDR_RGBA_0.png", &blue_noise_w, &blue_noise_h, 0, 4);
    glGenTextures(1, &blue_noise_texture);
    glBindTexture(GL_TEXTURE_2D, blue_noise_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, blue_noise_w, blue_noise_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, blue_noise_data);
    stbi_image_free(blue_noise_data);
}

struct sprite_info
{
    int_2 offset;
    int_2 size;
};

// sprite_info load_to_spritesheet(char* filename, GLuint texture)
// {
//     sprite_info out;
//     byte* image_data = stbi_load(filename, &out.size.x, &out.size.y, 0, 4);
//     glBindTexture(GL_TEXTURE_2D, texture);
//     glTexSubImage2D(GL_TEXTURE_2D, 0, out.offset.x, out.offset.y, out.size.x, out.size.y, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
//     stbi_image_free(image_data);
// }

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

void draw_debug_text(char* text, real x, real y)
{
    GLuint text_buffer = gl_general_buffers[0];

    glUseProgram(debug_text_program);

    glBindBuffer(GL_ARRAY_BUFFER, text_buffer);

    size_t offset = 0;
    int layout_location = 0;

    #define vertex_size (sizeof(float)*3+sizeof(uint8)*4)

    byte vb[4096*vertex_size] = {};
    int n_quads = stb_easy_font_print(x, -y, text, NULL, vb, sizeof(vb));
    glBufferSubData(GL_ARRAY_BUFFER, offset, 4*vertex_size*n_quads, (void*) vb);

    add_contiguous_attribute(3, GL_FLOAT, false, vertex_size, 0); //center position
    add_contiguous_attribute(4, GL_UNSIGNED_BYTE, false, vertex_size, 0); //radius

    // add_interleaved_attribute(3, GL_FLOAT, false, vertex_size, 1); //center position
    // add_interleaved_attribute(4, GL_UNSIGNED_BYTE, false, vertex_size, 1); //radius

    glDrawArrays(GL_QUADS, 0, 4*n_quads);

    #undef vertex_size
}

void draw_text(char* text, real x, real y, real_4 color, font_info font)
{
    GLuint text_buffer = gl_general_buffers[0];

    glUseProgram(draw_text_program);

    int texture_i = 0;
    int uniform_i = 0;

    real_2 resolution = {window_width, window_height};
    glUniform2fv(uniform_i++, 1, (GLfloat*) &resolution);
    glUniform4fv(uniform_i++, 1, (GLfloat*) &color);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, font.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindBuffer(GL_ARRAY_BUFFER, text_buffer);

    size_t offset = 0;
    int layout_location = 0;

    #define vertex_size (4*sizeof(real))

    real_4 vb[4096] = {};
    int n_verts = 0;

    real x_base = 0.5*x*window_height;
    real y_base = -0.5*y*window_height;

    real current_x = x_base;
    real current_y = y_base;
    for(char* c = text; *c; c++)
    {
        if(*c == '\n')
        {
            current_x = x_base;
            current_y += font.size;
            continue;
        }
        stbtt_aligned_quad quad = {};
        stbtt_GetPackedQuad(font.char_data, font_resolution, font_resolution, *c,
                            &current_x, &current_y, &quad, false);
        vb[n_verts++] = {quad.x0, quad.y0, quad.s0, quad.t0};
        vb[n_verts++] = {quad.x1, quad.y0, quad.s1, quad.t0};
        vb[n_verts++] = {quad.x1, quad.y1, quad.s1, quad.t1};
        vb[n_verts++] = {quad.x0, quad.y1, quad.s0, quad.t1};
    }
    glBufferSubData(GL_ARRAY_BUFFER, offset, vertex_size*n_verts, (void*) vb);

    add_contiguous_attribute(2, GL_FLOAT, false, vertex_size, 2*sizeof(float)); //xy
    add_contiguous_attribute(2, GL_FLOAT, false, vertex_size, 2*sizeof(float)); //st

    glDrawArrays(GL_QUADS, 0, n_verts);
}

void render_prepass(real_3x3 camera_axes, real_3 camera_pos, int_3 size, int_3 origin, gpu_body_data* bodies_gpu, int n_bodies)
{
    glUseProgram(render_prepass_program);

    int texture_i = 0;
    int uniform_i = 0;

    //camera
    glUniformMatrix3fv(uniform_i++, 1, false, (GLfloat*) &camera_axes);
    glUniform3fv(uniform_i++, 1, (GLfloat*) &camera_pos);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, material_visual_properties_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

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

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, body_materials_textures[current_body_materials_texture]);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform3iv(uniform_i++, 1, (GLint*) &size);
    glUniform3iv(uniform_i++, 1, (GLint*) &origin);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, lightprobe_color_textures[current_lightprobe_texture]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, lightprobe_depth_textures[current_lightprobe_texture]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, lightprobe_x_textures[current_lightprobe_texture]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, blue_noise_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    static int frame_number = 0;
    glUniform1i(uniform_i++, frame_number++);
    glUniform1i(uniform_i++, n_bodies++);

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
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, prepass_x_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, prepass_orientation_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, prepass_voxel_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, prepass_color_texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);

    GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glDrawBuffers(len(buffers), buffers);

    glViewport(0, 0, prepass_resolution_x, prepass_resolution_y);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, 0, 0);

    glDrawBuffers(1, buffers);
}

void render_world(real_3x3 camera_axes, real_3 camera_pos)
{
    glUseProgram(render_world_program);

    int texture_i = 0;
    int uniform_i = 0;

    static int frame_number = 0;
    glUniform1i(uniform_i++, frame_number++);

    //camera
    glUniformMatrix3fv(uniform_i++, 1, false, (GLfloat*) &camera_axes);
    glUniform3fv(uniform_i++, 1, (GLfloat*) &camera_pos);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, material_visual_properties_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, prepass_x_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, prepass_orientation_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, prepass_voxel_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, prepass_color_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform2f(uniform_i++, prepass_resolution_x, prepass_resolution_y);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, lightprobe_color_textures[current_lightprobe_texture]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, lightprobe_depth_textures[current_lightprobe_texture]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, lightprobe_x_textures[current_lightprobe_texture]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

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

    glDisable(GL_BLEND);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glEnable(GL_BLEND);
}

void load_form_to_gpu(cpu_form_data* fc, gpu_form_data* fg)
{
    static int n_forms = 0;
    if(fc->storage_level < sm_gpu)
    {
        int padding = 0;
        int size = 32;
        int forms_per_row = (form_texture_size-2*padding)/size;
        fg->materials_origin = {padding+size*(n_forms%forms_per_row),padding+size*((n_forms/forms_per_row)%forms_per_row),padding+size*(n_forms/sq(forms_per_row))};
        n_forms++;

        glBindTexture(GL_TEXTURE_3D, form_materials_texture);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        fg->materials_origin.x, fg->materials_origin.y, fg->materials_origin.z,
                        fg->size.x, fg->size.y, fg->size.z,
                        GL_RED_INTEGER, GL_UNSIGNED_BYTE,
                        fc->materials);
        fc->storage_level = sm_gpu;
    }
}

void reload_form_to_gpu(cpu_form_data* fc, gpu_form_data* fg)
{
    glBindTexture(GL_TEXTURE_3D, form_materials_texture);
    glTexSubImage3D(GL_TEXTURE_3D, 0,
                    fg->materials_origin.x, fg->materials_origin.y, fg->materials_origin.z,
                    fg->size.x, fg->size.y, fg->size.z,
                    GL_RED_INTEGER, GL_UNSIGNED_BYTE,
                    fc->materials);
}

void render_editor_voxels(real_3x3 camera_axes, real_3 camera_pos, genedit_window* gew)
{
    glUseProgram(render_editor_voxels_program);

    genome* gn = gew->active_genome;

    int texture_i = 0;
    int uniform_i = 0;

    //camera
    glUniformMatrix3fv(uniform_i++, 1, false, (GLfloat*) &camera_axes);
    glUniform3fv(uniform_i++, 1, (GLfloat*) &camera_pos);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, material_visual_properties_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, form_materials_texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, blue_noise_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    static int frame_number = 0;
    glUniform1i(uniform_i++, frame_number++);
    glUniform1i(uniform_i++, gn->n_forms);

    glUniform1i(uniform_i++, gew->active_form);
    glUniform3iv(uniform_i++, 1, (int*) &gew->active_cell);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, form_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(gn->forms_gpu[0])*gn->n_forms, gn->forms_gpu);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, form_buffer);

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

void pad_lightprobes()
{
    glUseProgram(pad_lightprobes_program);

    int texture_i = 0;
    int uniform_i = 0;

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, lightprobe_color_textures[1-current_lightprobe_texture]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, lightprobe_depth_textures[1-current_lightprobe_texture]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    size_t offset = 0;
    int layout_location = 0;

    GLuint vertex_buffer = gl_general_buffers[0];
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    // real vb[] = {0,0,0};
    real vb[] = {
        -1,-1,
        -1,+1,
        +1,+1,
        +1,-1};
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);
    add_contiguous_attribute(2, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightprobe_color_textures[current_lightprobe_texture], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, lightprobe_depth_textures[current_lightprobe_texture], 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    // glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencil_buffer);

    GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(len(buffers), buffers);

    glViewport(0, 0, lightprobe_resolution_x, lightprobe_resolution_y);

    glDisable(GL_BLEND);
    // glEnable(GL_STENCIL_TEST);
    // glStencilMask(0x00);
    // glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    // glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glEnable(GL_BLEND);
    // glDisable(GL_STENCIL_TEST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);

    glDrawBuffers(1, buffers);

    glEnable(GL_DEPTH_TEST);
    // glDisable(GL_STENCIL_TEST);
}

void update_lightprobes()
{
    glUseProgram(update_lightmap_program);

    // glCopyImageSubData(lightprobe_color_textures[current_lightprobe_texture], GL_TEXTURE_2D,
    //                    0,
    //                    0, 0, 0,
    //                    lightprobe_color_textures[1-current_lightprobe_texture], GL_TEXTURE_2D,
    //                    0,
    //                    0, 0, 0,
    //                    lightprobe_resolution_x, lightprobe_resolution_y, 1);

    // glCopyImageSubData(lightprobe_depth_textures[current_lightprobe_texture], GL_TEXTURE_2D,
    //                    0,
    //                    0, 0, 0,
    //                    lightprobe_depth_textures[1-current_lightprobe_texture], GL_TEXTURE_2D,
    //                    0,
    //                    0, 0, 0,
    //                    lightprobe_resolution_x, lightprobe_resolution_y, 1);

    int texture_i = 0;
    int uniform_i = 0;
    int image_i = 0;

    static int frame_number = 0;
    glUniform1i(uniform_i++, frame_number++);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, material_visual_properties_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

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
    glBindTexture(GL_TEXTURE_3D, occupied_regions_texture);
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
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, lightprobe_color_textures[current_lightprobe_texture]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, lightprobe_depth_textures[current_lightprobe_texture]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, lightprobe_x_textures[current_lightprobe_texture]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, blue_noise_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glUniform1i(uniform_i++, image_i);
    glBindImageTexture(image_i++, lightprobe_color_textures[1-current_lightprobe_texture], 0, true, 0, GL_WRITE_ONLY, GL_RGBA16F);

    glUniform1i(uniform_i++, image_i);
    glBindImageTexture(image_i++, lightprobe_depth_textures[1-current_lightprobe_texture], 0, true, 0, GL_WRITE_ONLY, GL_RG16F);

    glDispatchCompute(lightprobes_w/4,lightprobes_h/4,1);

    // current_lightprobe_texture = 1-current_lightprobe_texture;

    pad_lightprobes();
}

void draw_lightprobes(real_4x4 camera)
{
    glUseProgram(draw_lightprobes_program);

    int texture_i = 0;
    int uniform_i = 0;

    glUniformMatrix4fv(uniform_i++, 1, true, (GLfloat*) &camera);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, lightprobe_color_textures[current_lightprobe_texture]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glGenerateMipmap(GL_TEXTURE_2D);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, lightprobe_depth_textures[current_lightprobe_texture]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glGenerateMipmap(GL_TEXTURE_2D);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, lightprobe_x_textures[current_lightprobe_texture]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    const real phi = 0.5*(1.0+sqrt(5.0));
    const real a = 1.0/sqrt(phi+2);
    const real b = phi*a;

    #define TOTAL_VERTS (12+20*3+20*4*3+20*4*4*3+20*4*4*4*3)
    #define TOTAL_TRIANGLES (20*4*4*4*4)

    int n_verts = 12;
    real_3 vb[TOTAL_VERTS] = {
        {0, -a, -b}, //0
        {-b, 0, -a}, //1
        {-a, -b, 0}, //2

        {0, +a, -b}, //3
        {-b, 0, +a}, //4
        {+a, -b, 0}, //5

        {0, -a, +b}, //6
        {+b, 0, -a}, //7
        {-a, +b, 0}, //8

        {0, +a, +b}, //9
        {+b, 0, +a}, //10
        {+a, +b, 0}, //11
    };

    int n_elements = 3*20;
    uint16 ib[3*TOTAL_TRIANGLES] = {
        // 0,1,2,5,7,3,
        // 1,
        // 4,2,6,5,10,7,11,3,8,1,4,
        // 9,8,4,6,10,11,8
        0,1,2,0,2,5,0,5,7,0,7,3,0,3,1,
        1,4,2,4,2,6,2,6,5,6,5,10,5,10,7,10,7,11,7,11,3,11,3,8,3,8,1,8,1,4,
        9,8,4,9,4,6,9,6,10,9,10,11,9,11,8,
    };
    #undef TOTAL_VERTS
    #undef TOTAL_TRIANGLES

    //TODO: combine duplicated vertices
    for(int i = 0; i < 4; i++)
    {
        int original_n_elements = n_elements;
        for(int t = 0; t < original_n_elements; t+=3)
        {
            uint16 i0 = ib[t+0];
            uint16 i1 = ib[t+1];
            uint16 i2 = ib[t+2];

            uint16 i3 = n_verts++;
            uint16 i4 = n_verts++;
            uint16 i5 = n_verts++;
            vb[i3] = normalize(vb[i1]+vb[i2]);
            vb[i4] = normalize(vb[i2]+vb[i0]);
            vb[i5] = normalize(vb[i0]+vb[i1]);

            ib[t+0] = i3;
            ib[t+1] = i4;
            ib[t+2] = i5;

            ib[n_elements++] = i0;
            ib[n_elements++] = i4;
            ib[n_elements++] = i5;

            ib[n_elements++] = i3;
            ib[n_elements++] = i1;
            ib[n_elements++] = i5;

            ib[n_elements++] = i3;
            ib[n_elements++] = i4;
            ib[n_elements++] = i2;
        }
    }

    GLuint vertex_buffer = gl_general_buffers[0];
    GLuint index_buffer = gl_general_buffers[1];

    size_t offset = 0;
    int layout_location = 0;

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, offset, n_verts*sizeof(vb[0]), (void*) vb);
    add_contiguous_attribute(3, GL_FLOAT, false, 0, n_verts*sizeof(vb[0])); //vertex positions

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, n_elements*sizeof(ib[0]), (void*) ib);

    glDrawElementsInstanced(GL_TRIANGLES, n_elements, GL_UNSIGNED_SHORT, (void*) 0, lightprobes_w*lightprobes_h);
}

void draw_particles(real_4x4 camera)
{
    glUseProgram(draw_particles_program);

    int texture_i = 0;
    int uniform_i = 0;

    glUniformMatrix4fv(uniform_i++, 1, true, (GLfloat*) &camera);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_buffer);

    const real phi = 0.5*(1.0+sqrt(5.0));
    const real a = 1.0/sqrt(phi+2);
    const real b = phi*a;

    real_3 vb[] = {
        {-0.5,-0.5,-0.5}, //0
        {-0.5,-0.5,+0.5}, //1
        {-0.5,+0.5,-0.5}, //2
        {-0.5,+0.5,+0.5}, //3
        {+0.5,-0.5,-0.5}, //4
        {+0.5,-0.5,+0.5}, //5
        {+0.5,+0.5,-0.5}, //6
        {+0.5,+0.5,+0.5}, //7
    };

    uint16 ib[] = {
        0,1,2, 1,2,3,
        4,5,6, 5,6,7,

        0,1,4, 1,2,5,
        2,3,6, 3,6,7,
        0,2,4, 2,4,6,
        1,3,5, 3,5,7,
    };

    GLuint vertex_buffer = gl_general_buffers[0];
    GLuint index_buffer = gl_general_buffers[1];

    size_t offset = 0;
    int layout_location = 0;

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);
    add_contiguous_attribute(3, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(ib), (void*) ib);

    glDrawElementsInstanced(GL_TRIANGLES, len(ib), GL_UNSIGNED_SHORT, (void*) 0, N_MAX_PARTICLES);
}


void move_lightprobes()
{
    glUseProgram(move_lightprobes_program);

    int texture_i = 0;
    int uniform_i = 0;

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
    glBindTexture(GL_TEXTURE_2D, lightprobe_x_textures[current_lightprobe_x_texture]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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
    glViewport(0, 0, lightprobes_w, lightprobes_h);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightprobe_x_textures[1-current_lightprobe_x_texture], 0);

    GLenum buffers[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(len(buffers), buffers);

    glDisable(GL_BLEND);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glEnable(GL_BLEND);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    current_lightprobe_x_texture = 1-current_lightprobe_x_texture;
}

void denoise(GLuint dst_texture, GLuint src_texture)
{
    glUseProgram(denoise_program);

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst_texture, 0);

    int texture_i = 0;
    int uniform_i = 0;

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, src_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    // glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    // glGenerateMipmap(GL_TEXTURE_2D);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, depth_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glGenerateMipmap(GL_TEXTURE_2D);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, normal_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glGenerateMipmap(GL_TEXTURE_2D);

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
    glBindTexture(GL_TEXTURE_2D, material_physical_properties_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

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

        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               materials_textures[1-current_materials_texture], 0, l);

        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, 32*32);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    current_materials_texture = 1-current_materials_texture;
}

void simulate_chunk_atomic(chunk* c, int update_cells)
{
    glUseProgram(simulate_chunk_atomic_program);

    load_chunk_to_gpu(c);

    int texture_i = 0;
    int image_i = 0;
    int uniform_i = 0;

    static int frame_number = 0;
    glUniform1i(uniform_i++, frame_number++);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, material_physical_properties_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, materials_textures[current_materials_texture]);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, image_i);
    glBindImageTexture(image_i++, materials_textures[1-current_materials_texture],
                       0, true, 0, GL_READ_WRITE, GL_RGBA8UI);

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

    //swap active regions textures
    current_active_regions_texture = 1-current_active_regions_texture;

    glUniform1i(uniform_i++, update_cells);

    glDispatchCompute(32,32,32);

    current_materials_texture = 1-current_materials_texture;
}

void mipmap_chunk(int min_level, int max_level)
{
    glUseProgram(mipmap_chunk_program);

    int texture_i = 0;
    int image_i = 0;
    int uniform_i = 0;

    int layer_number_uniform = uniform_i++;

    int mip_level_uniform = uniform_i++;

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

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    for(int mip_level = min_level; mip_level < max_level; mip_level++)
    {
        glUniform1i(mip_level_uniform, mip_level);

        uint mip_scale = (1<<mip_level);
        glViewport(0, 0, chunk_size/mip_scale, chunk_size/mip_scale);
        log_output("level ", mip_level, ": ", mip_scale, "\n");

        for(int l = 0; l < chunk_size/mip_scale; l++)
        {
            log_output("   layer ", l, "\n");
            glUniform1i(layer_number_uniform, l);

            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                      materials_textures[current_materials_texture], mip_level, l);

            glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, 32*32);
        }
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
}

void simulate_particles()
{
    glUseProgram(simulate_particles_program);

    int texture_i = 0;
    int uniform_i = 0;
    int image_i = 0;

    static int frame_number = 0;
    glUniform1i(uniform_i++, frame_number++);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, material_physical_properties_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, materials_textures[current_materials_texture]);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, image_i);
    glBindImageTexture(image_i++, materials_textures[current_materials_texture], 0, true, 0, GL_WRITE_ONLY, GL_RGBA8UI);

    glUniform1i(uniform_i++, image_i);
    glBindImageTexture(image_i++, active_regions_textures[current_active_regions_texture], 0, true, 0, GL_WRITE_ONLY, GL_R8UI);

    glUniform1i(uniform_i++, image_i);
    glBindImageTexture(image_i++, occupied_regions_texture, 0, true, 0, GL_WRITE_ONLY, GL_R8UI);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_buffer);

    glDispatchCompute(N_MAX_PARTICLES/1024,1,1);
}

void simulate_bodies(memory_manager* manager, cpu_body_data* bodies_cpu, gpu_body_data* bodies_gpu, int n_bodies, gpu_form_data* forms_gpu, int n_forms)
{
    glUseProgram(simulate_body_program);

    int texture_i = 0;
    int image_i = 0;
    int uniform_i = 0;

    static int frame_number = 0;
    glUniform1i(uniform_i++, frame_number++);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, material_physical_properties_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

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

    glUniform1i(uniform_i++, image_i);
    glBindImageTexture(image_i++, body_materials_textures[1-current_body_materials_texture], 0, true, 0, GL_WRITE_ONLY, GL_RGBA8UI);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, form_materials_texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glUniform1i(uniform_i++, n_bodies);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, body_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(bodies_gpu[0])*n_bodies, bodies_gpu);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, body_buffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, form_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(forms_gpu[0])*n_forms, forms_gpu);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, form_buffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, particle_buffer);

    uint* body_chunks = (uint*) reserve_block(manager, n_bodies*8*4*sizeof(uint));
    uint n_body_chunks = 0;
    for(int b = 0; b < n_bodies; b++)
    {
        for(int x = 0; x < bodies_gpu[b].size.x; x+=body_chunk_size)
            for(int y = 0; y < bodies_gpu[b].size.y; y+=body_chunk_size)
                for(int z = 0; z < bodies_gpu[b].size.z; z+=body_chunk_size)
                {
                    body_chunks[4*n_body_chunks+1] = b;
                    body_chunks[4*n_body_chunks+2] = bodies_gpu[b].materials_origin.x+x;
                    body_chunks[4*n_body_chunks+3] = bodies_gpu[b].materials_origin.y+y;
                    body_chunks[4*n_body_chunks+4] = bodies_gpu[b].materials_origin.z+z;
                    n_body_chunks++;
                }
    }
    body_chunks[0] = n_body_chunks;

    unreserve_block(manager);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, body_chunks_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (1+4*n_body_chunks)*sizeof(uint), body_chunks);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, body_chunks_buffer);

    glDispatchCompute(n_body_chunks,1,1);

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

    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, contact_point_textures[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,GL_TEXTURE_2D, contact_point_textures[1], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT2,GL_TEXTURE_2D, contact_point_textures[2], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT3,GL_TEXTURE_2D, contact_normal_textures[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT4,GL_TEXTURE_2D, contact_normal_textures[1], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT5,GL_TEXTURE_2D, contact_normal_textures[2], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT6,GL_TEXTURE_2D, contact_material_texture, 0);

    GLenum buffers[] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3,
        GL_COLOR_ATTACHMENT4,
        GL_COLOR_ATTACHMENT5,
        GL_COLOR_ATTACHMENT6,
    };
    glDrawBuffers(len(buffers), buffers);

    glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
    glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);
    glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
    glDisable(GL_BLEND);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glEnable(GL_BLEND);

    size_t out_size = n_bodies*sizeof(float)*3;
    size_t material_out_size = n_bodies*sizeof(int16)*3*2;
    byte* out_data = reserve_block(manager, 6*out_size+material_out_size);
    real_3* contact_points = (real_3*) out_data;
    real_3* contact_normals = (real_3*) (out_data+3*out_size);
    int16* contact_materials = (int16*) (out_data+6*out_size);

    for(int i = 0; i < 3; i++)
    {
        glGetTextureSubImage(contact_point_textures[i],
                             0,
                             0, 0, 0,
                             1, n_bodies, 1,
                             GL_RGB,
                             GL_FLOAT,
                             out_size,
                             contact_points+n_bodies*i);

        glGetTextureSubImage(contact_normal_textures[i],
                             0,
                             0, 0, 0,
                             1, n_bodies, 1,
                             GL_RGB,
                             GL_FLOAT,
                             out_size,
                             contact_normals+n_bodies*i);
    }

    glGetTextureSubImage(contact_material_texture,
                         0,
                         0, 0, 0,
                         1, n_bodies, 1,
                         GL_RGB_INTEGER,
                         GL_SHORT,
                         material_out_size,
                         contact_materials);

    for(int b = 0; b < n_bodies; b++)
    {
        for(int i = 0; i < 3; i++)
        {
            if(bodies_cpu[b].contact_locked[i]) continue;
            bodies_cpu[b].contact_points[i] = contact_points[b+n_bodies*i];
            bodies_cpu[b].contact_pos[i] = apply_rotation(conjugate(bodies_gpu[b].orientation), contact_points[b+n_bodies*i]-bodies_gpu[b].x);
            // if(contact_normals[b+n_bodies*i] != contact_normals[b+n_bodies*i])
            // {
            //     assert(false, "bad contact normal");
            // }
            bodies_cpu[b].contact_depths[i] = norm(contact_normals[b+n_bodies*i])-32;
            bodies_cpu[b].contact_normals[i] = normalize_or_zero(contact_normals[b+n_bodies*i]);
            bodies_cpu[b].contact_materials[i] = contact_materials[3*b+i];
            // draw_circle(rd, contact_points[b+n_bodies*i], 0.3, {bodies_cpu[b].contact_depths[i] > 1,bodies_cpu[b].contact_depths[i] <= 1,0,1});
            // draw_circle(rd, contact_points[b+n_bodies*i]+0.1*contact_normals[b+n_bodies*i], 0.2, {1,0,0.5*contact_materials[3*b+i],1});
        }
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT6, GL_TEXTURE_2D, 0, 0);

    glDrawBuffers(1, buffers);

    unreserve_block(manager);
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

// voxel_data get_voxel_data(int_3 x)
// {
//     int_2 data[27];
//     int_3 dim = {3,3,3};
//     glGetTextureSubImage(materials_textures[current_materials_texture],
//                          0,
//                          x.x-1, x.y-1, x.z-1,
//                          dim.x, dim.y, dim.z,
//                          GL_RG_INTEGER,
//                          GL_INT,
//                          sizeof(data),
//                          data);

//     real_3 gradient = {
//         data[2+1*3+1*9].y-data[0+1*3+1*9].y,
//         data[1+2*3+1*9].y-data[1+0*3+1*9].y,
//         data[1+1*3+2*9].y-data[1+1*3+0*9].y,
//     };

//     voxel_data out = {
//         .material = data[13].x,
//         .distance = data[13].y,
//         .normal = normalize(gradient),
//     };
//     return out;
// }

voxel_data get_voxel_data(int_3 x)
{
    int_2 data[27];
    int_3 dim = {3,3,3};
    x = clamp_per_axis(x, 1, chunk_size-2);

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    // glViewport(0, 0, chunk_size, chunk_size);

    for(int z = 0; z < dim.z; z++)
    {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  materials_textures[current_materials_texture], 0, x.z+z);
        // glReadPixels(x.x, x.y, dim.x, dim.y, GL_RG_INTEGER, GL_INT, data+9*z);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    real_3 gradient = {
        data[2+1*3+1*9].y-data[0+1*3+1*9].y,
        data[1+2*3+1*9].y-data[1+0*3+1*9].y,
        data[1+1*3+2*9].y-data[1+1*3+0*9].y+0.01,
    };

    voxel_data out = {
        .material = data[13].x,
        .distance = data[13].y,
        .normal = normalize(gradient),
    };
    return out;
}

// voxel_data get_voxel_data(int_3 x)
// {
//     glUseProgram(get_voxel_data_program);

//     int texture_i = 0;
//     int uniform_i = 0;

//     glUniform1i(uniform_i++, texture_i);
//     glActiveTexture(GL_TEXTURE0+texture_i++);
//     glBindTexture(GL_TEXTURE_3D, materials_textures[current_materials_texture]);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

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
//     glViewport(0, 0, 1, n_particles);

//     glBindTexture(GL_TEXTURE_2D, particles_x_texture);
//     glTexSubImage2D(GL_TEXTURE_2D,
//                     0,
//                     0, 0,
//                     1, n_particles,
//                     GL_RGB, GL_FLOAT, xs);

//     glBindTexture(GL_TEXTURE_2D, particles_x_dot_texture);
//     glTexSubImage2D(GL_TEXTURE_2D,
//                     0,
//                     0, 0,
//                     1, n_particles,
//                     GL_RGB, GL_FLOAT, x_dots);

//     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, particles_x_texture, 0);
//     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, particles_x_dot_texture, 0);

//     GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
//     glDrawBuffers(len(buffers), buffers);

//     glDisable(GL_BLEND);
//     glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
//     glEnable(GL_BLEND);

//     glGetTextureSubImage(particles_x_texture,
//                          0,
//                          0, 0, 0,
//                          1, n_particles, 1,
//                          GL_RGB,
//                          GL_FLOAT,
//                          sizeof(xs[0])*n_particles,
//                          xs);

//     glGetTextureSubImage(particles_x_dot_texture,
//                          0,
//                          0, 0, 0,
//                          1, n_particles, 1,
//                          GL_RGB,
//                          GL_FLOAT,
//                          sizeof(x_dots[0])*n_particles,
//                          x_dots);

//     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
//     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
// }

#endif
