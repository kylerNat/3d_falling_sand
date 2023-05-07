#ifndef GRAPHICS
#define GRAPHICS

#define GLSL_VERSION "430"

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
#include "room.h"

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
            #ifdef SHOW_GL_NOTICES
            log_output("NOTICE: ");
            #else
            return;
            #endif
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
#define active_regions_size (room_size/active_regions_width)

GLuint active_regions_textures[2];
int current_active_regions_texture = 0;

GLuint occupied_regions_texture;
#define occupied_regions_width 16
#define occupied_regions_size (room_size/occupied_regions_width)

#define N_MAX_BODIES 4096
#define N_MAX_CONTACTS (N_MAX_BODIES*32)
#define N_MAX_JOINTS 4096
GLuint body_buffer;
GLuint body_chunks_buffer;
GLuint body_updates_buffer;
GLuint joint_updates_buffer;
GLuint contacts_buffer;
GLuint collision_grid_buffer;

GLuint world_cell_encoding_buffer;

GLuint joint_buffer;

#define N_MAX_FORMS 4096
GLuint editor_object_buffer;

GLuint form_materials_texture;

#define N_MAX_PARTICLES 4096
GLuint particle_buffer;

#define N_MAX_EXPLOSIONS 4096
GLuint explosion_buffer;

#define N_MAX_BEAMS 4096
GLuint beam_buffer;

#define N_MAX_RAYS 4096
GLuint rays_in_buffer;
GLuint rays_out_buffer;

GLuint body_x_dot_texture;
GLuint body_omega_texture;

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
#include "../shaders/include/lightprobe_header.glsl" //lightprobe constants

GLuint probe_rays_buffer;

int current_lightprobe_texture = 0;
int current_lightprobe_x_texture = 0;

GLuint lightprobe_x_textures[2];

GLuint spritesheet_texture;
rectangle_space spritesheet_space = {};

GLuint blue_noise_texture;
int blue_noise_w;
int blue_noise_h;

#define font_resolution 1024

struct font_info
{
    GLuint texture;
    stbtt_fontinfo info;
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
            .chardata_for_range = (stbtt_packedchar*) stalloc_clear(sizeof(stbtt_packedchar)*256),
        },
    };

    uint8* pixels = (uint8*) stalloc(4*sq(font_resolution));

    error = stbtt_PackBegin(&spc, pixels, font_resolution, font_resolution, 0, 1, 0);
    assert(error);

    stbtt_PackSetOversampling(&spc, 2, 2);
    // error = stbtt_PackFontRanges(&spc, font_data, 0, ranges, len(ranges));
    //expanding out stbtt_PackFontRanges so we can get out the stbtt_fontinfo
    stbtt_fontinfo info;
    int font_index = 0;
    {
        int i,j,n, return_value = 1;
        stbrp_rect    *rects;

        n = 0;
        for (i=0; i < len(ranges); ++i)
            n += ranges[i].num_chars;

        rects = (stbrp_rect *) STBTT_malloc(sizeof(*rects) * n, spc.user_allocator_context);
        assert(rects, "could not allocate font rects");

        info.userdata = spc.user_allocator_context;
        stbtt_InitFont(&info, font_data, stbtt_GetFontOffsetForIndex(font_data,font_index));

        n = stbtt_PackFontRangesGatherRects(&spc, &info, ranges, len(ranges), rects);

        stbtt_PackFontRangesPackRects(&spc, rects, n);

        return_value = stbtt_PackFontRangesRenderIntoRects(&spc, &info, ranges, len(ranges), rects);

        STBTT_free(rects, spc.user_allocator_context);
        error = return_value;
    }
    assert(error);

    stbtt_PackEnd(&spc);

    GLuint font_texture;
    glGenTextures(1, &font_texture);
    glBindTexture(GL_TEXTURE_2D, font_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, font_resolution, font_resolution, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);

    stunalloc(pixels);

    return {font_texture, info, ranges[0].chardata_for_range, font_size};
}

void gl_init_buffers()
{
    memory_manager* manager = get_context()->manager;

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
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, lightprobe_resolution_x, lightprobe_resolution_y);

        real_3 clear = {0.0,0.0,0.0};
        glClearTexImage(lightprobe_color_textures[i], 0, GL_RGB, GL_FLOAT, &clear);
    }

    glGenTextures(len(lightprobe_depth_textures), lightprobe_depth_textures);
    for(int i = 0; i < len(lightprobe_depth_textures); i++)
    {
        glBindTexture(GL_TEXTURE_2D, lightprobe_depth_textures[i]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG16F, lightprobe_depth_resolution_x, lightprobe_depth_resolution_y);

        real_2 clear = {lightprobe_spacing,0};
        glClearTexImage(lightprobe_depth_textures[i], 0, GL_RG, GL_FLOAT, &clear);
    }

    glGenTextures(len(lightprobe_x_textures), lightprobe_x_textures);
    real_3* image = (real_3*) stalloc(lightprobes_h*lightprobes_w*sizeof(real_3));
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

    glGenBuffers(1, &probe_rays_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, probe_rays_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, lightprobes_w*lightprobes_h*rays_per_lightprobe*4*8, 0, 0);

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

    glGenTextures(1, &round_line_texture);
    glBindTexture(GL_TEXTURE_2D, round_line_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 2, N_MAX_LINE_POINTS);

    int n_physical_properties = sizeof(materials_list->physical_properties)/sizeof(real);
    glGenTextures(1, &material_physical_properties_texture);
    glBindTexture(GL_TEXTURE_2D, material_physical_properties_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, n_physical_properties, N_MAX_MATERIALS);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 3, N_MAX_MATERIALS, 0, GL_RGB, GL_FLOAT, materials);

    int n_visual_properties = sizeof(materials_list->visual_properties)/sizeof(real);
    glGenTextures(1, &material_visual_properties_texture);
    glBindTexture(GL_TEXTURE_2D, material_visual_properties_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, (n_visual_properties+2)/3, N_MAX_MATERIALS);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 3, N_MAX_MATERIALS, 0, GL_RGB, GL_FLOAT, materials);

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
        glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8UI, room_size, room_size, room_size);
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

    glGenBuffers(1, &body_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, body_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, N_MAX_BODIES*sizeof(gpu_body_data), 0, GL_DYNAMIC_STORAGE_BIT);

    glGenBuffers(1, &body_chunks_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, body_chunks_buffer);
    int body_chunks_wide = body_texture_size/body_chunk_size;
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, body_chunks_wide*body_chunks_wide*body_chunks_wide, 0, GL_DYNAMIC_STORAGE_BIT);

    glGenBuffers(1, &body_updates_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, body_updates_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, N_MAX_BODIES*sizeof(body_update), 0, GL_CLIENT_STORAGE_BIT);

    glGenBuffers(1, &joint_updates_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, joint_updates_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, N_MAX_JOINTS*sizeof(joint_update), 0, GL_CLIENT_STORAGE_BIT);

    glGenBuffers(1, &contacts_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, contacts_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, N_MAX_CONTACTS*sizeof(contact_point), 0, GL_CLIENT_STORAGE_BIT);

    glGenBuffers(1, &collision_grid_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, collision_grid_buffer);
    size_t n_max_body_indices = 8*N_MAX_BODIES;
    size_t body_indices_size = sizeof(int)*n_max_body_indices;
    size_t collision_grid_size = sizeof(int_2)*collision_cell_size*collision_cell_size*collision_cell_size;
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, collision_grid_size+body_indices_size, 0, GL_DYNAMIC_STORAGE_BIT);

    glGenBuffers(1, &world_cell_encoding_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, world_cell_encoding_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, ((1+room_size*room_size)*sizeof(int)+2*room_size*room_size*room_size*sizeof(rle_run)), 0, GL_CLIENT_STORAGE_BIT|GL_DYNAMIC_STORAGE_BIT);

    glGenBuffers(1, &joint_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, joint_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, N_MAX_BODIES*sizeof(gpu_body_joint), 0, GL_DYNAMIC_STORAGE_BIT);

    glGenBuffers(1, &editor_object_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, editor_object_buffer);
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

    glGenBuffers(1, &explosion_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, explosion_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(int)+N_MAX_EXPLOSIONS*sizeof(explosion_data), 0, GL_DYNAMIC_STORAGE_BIT);

    glGenBuffers(1, &beam_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, beam_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(int)+N_MAX_BEAMS*sizeof(beam_data), 0, GL_DYNAMIC_STORAGE_BIT);

    glGenBuffers(1, &rays_in_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, rays_in_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, N_MAX_RAYS*sizeof(ray_in), 0, GL_DYNAMIC_STORAGE_BIT);

    glGenBuffers(1, &rays_out_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, rays_out_buffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, N_MAX_RAYS*sizeof(ray_out), 0, GL_CLIENT_STORAGE_BIT);

    glGenTextures(1, &spritesheet_texture);
    glBindTexture(GL_TEXTURE_2D, spritesheet_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, spritesheet_size, spritesheet_size);

    spritesheet_space = {
        .max_size = {spritesheet_size,spritesheet_size},
        .free_regions = (bounding_box_2d*) stalloc(sizeof(bounding_box_2d)*10000),
        .n_free_regions = 0,
    };

    spritesheet_space.free_regions[spritesheet_space.n_free_regions++] = {{0,0}, {spritesheet_size, spritesheet_size}};

    byte* blue_noise_data = stbi_load("data/HDR_RGBA_0.png", &blue_noise_w, &blue_noise_h, 0, 4);
    glGenTextures(1, &blue_noise_texture);
    glBindTexture(GL_TEXTURE_2D, blue_noise_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, blue_noise_w, blue_noise_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, blue_noise_data);
    stbi_image_free(blue_noise_data);
}

void init_material_properties_textures()
{
    int n_physical_properties = sizeof(materials_list->physical_properties)/sizeof(real);
    real* material_physicals = (real*) stalloc(sizeof(materials_list->physical_properties)*n_materials);
    for(int i = 0; i < n_materials; i++)
    {
        memcpy(material_physicals+n_physical_properties*i, materials_list[i].physical_properties, n_physical_properties*sizeof(real));
    }
    glBindTexture(GL_TEXTURE_2D, material_physical_properties_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, n_physical_properties, n_materials, GL_RED, GL_FLOAT, material_physicals);
    stunalloc(material_physicals);

    int n_visual_properties = sizeof(materials_list->visual_properties)/sizeof(real);
    real* material_visuals = (real*) stalloc(sizeof(materials_list->visual_properties)*n_materials);
    for(int i = 0; i < n_materials; i++)
    {
        memcpy(material_visuals+n_visual_properties*i, materials_list[i].visual_properties, n_visual_properties*sizeof(real));
    }
    glBindTexture(GL_TEXTURE_2D, material_visual_properties_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (n_visual_properties+2)/3, n_materials, GL_RGB, GL_FLOAT, material_visuals);
    stunalloc(material_visuals);
}

void update_material_properties_textures(int start_material, int end_material)
{
    int n_physical_properties = sizeof(materials_list->physical_properties)/sizeof(real);
    real* material_physicals = (real*) stalloc(sizeof(materials_list->physical_properties)*n_materials);
    for(int i = start_material; i < end_material; i++)
    {
        memcpy(material_physicals+n_physical_properties*i, materials_list[i].physical_properties, n_physical_properties*sizeof(real));
    }
    glBindTexture(GL_TEXTURE_2D, material_physical_properties_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, n_physical_properties, end_material-start_material, GL_RED, GL_FLOAT, material_physicals);
    stunalloc(material_physicals);

    int n_visual_properties = sizeof(materials_list->visual_properties)/sizeof(real);
    real* material_visuals = (real*) stalloc(sizeof(materials_list->visual_properties)*n_materials);
    for(int i = start_material; i < end_material; i++)
    {
        memcpy(material_visuals+n_visual_properties*i, materials_list[i].visual_properties, n_visual_properties*sizeof(real));
    }
    glBindTexture(GL_TEXTURE_2D, material_visual_properties_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (n_visual_properties+2)/3, end_material-start_material, GL_RGB, GL_FLOAT, material_visuals);
    stunalloc(material_visuals);

    glBindTexture(GL_TEXTURE_2D, 0);
}

bounding_box_2d load_sprite(char* filename)
{
    int_2 size = {};
    byte* image_data = stbi_load(filename, &size.x, &size.y, 0, 4);
    assert(image_data, " could not load ", filename, "\n");
    int_2 padding = (int_2){1,1};
    int_2 padded_size = size+2*padding;
    bounding_box_2d padded_sprite_region = add_region(&spritesheet_space, padded_size);
    bounding_box_2d sprite_region = {padded_sprite_region.l+padding, padded_sprite_region.u-padding};
    glBindTexture(GL_TEXTURE_2D, spritesheet_texture);
    uint clear = 0;
    glClearTexSubImage(spritesheet_texture, 0, padded_sprite_region.l.x, padded_sprite_region.l.y, 0, padded_size.x, padded_size.y, 1, GL_RGBA, GL_UNSIGNED_BYTE, &clear);
    glTexSubImage2D(GL_TEXTURE_2D, 0, sprite_region.l.x, sprite_region.l.y, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);
    return sprite_region;
}

void load_sprites()
{
    #define add_sprite(id, filename, n_frames) sprite_list[id] = load_sprite(filename); sprite_list[id].u.x = (sprite_list[id].u.x-sprite_list[id].l.x)/n_frames+sprite_list[id].l.x;
    #include "sprite_list.h"
    #undef add_sprite
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

char* shader_path = "shaders/";

#include "preprocess_shaders.h"

#define add_shader(program, filename) GLuint program;
#include "shader_list.h"

void gl_init_shaders()
{
    #define add_shader(program, filename) program = load_shader(filename);
    #include "shader_list.h"
}

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

void draw_to_screen_ssao(GLuint texture, GLuint dtexture, GLuint ntexture)
{
    glUseProgram(fullscreen_texture_ssao_program);

    int texture_i = 0;
    int uniform_i = 0;

    real fov = 120.0f*pi/180.0f; //TODO: set proper value
    glUniform1f(uniform_i++, fov);
    glUniform2f(uniform_i++, window_width, window_height);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, dtexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glGenerateMipmap(GL_TEXTURE_2D);
    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, ntexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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

void draw_sprites(sprite_render_info* sprites, int n_sprites, real_4x4 camera)
{
    GLuint sprite_buffer = gl_general_buffers[0];

    glUseProgram(sprite_program);

    int texture_i = 0;
    int uniform_i = 0;

    glUniformMatrix4fv(uniform_i++, 1, true, (GLfloat*) &camera);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, spritesheet_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //TODO: pixel AA
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindBuffer(GL_ARRAY_BUFFER, sprite_buffer);

    size_t offset = 0;
    int layout_location = 0;

    //buffer square coord data (bounding box of sprite)
    real vb[] = {-1,-1,0,
        -1,+1,0,
        +1,+1,0,
        +1,-1,0};
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);

    add_contiguous_attribute(3, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

    //buffer sprite data
    glBufferSubData(GL_ARRAY_BUFFER, offset, n_sprites*sizeof(sprites[0]), (void*) sprites);
    add_interleaved_attribute(3, GL_FLOAT, false, sizeof(sprites[0]), 1); //center position
    add_interleaved_attribute(2, GL_FLOAT, false, sizeof(sprites[0]), 1); //radius
    add_interleaved_attribute(4, GL_FLOAT, false, sizeof(sprites[0]), 1); //tint color
    add_interleaved_attribute(1, GL_FLOAT, false, sizeof(sprites[0]), 1); //theta
    add_interleaved_attribute(2, GL_FLOAT, false, sizeof(sprites[0]), 1); //lower texture coord
    add_interleaved_attribute(2, GL_FLOAT, false, sizeof(sprites[0]), 1); //upper texture coordinate

    glDisable(GL_DEPTH_TEST);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, n_sprites);
    glEnable(GL_DEPTH_TEST);
}

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

void draw_rectangles(rectangle_render_info* rectangles, int n_rectangles, real_4x4 camera)
{
    GLuint rectangle_buffer = gl_general_buffers[0];

    glUseProgram(rectangle_program);

    glUniformMatrix4fv(0, 1, false, (GLfloat*) &camera);
    // float smoothness = 3.0/(window_height);
    float smoothness = 3.0/(window_height);
    glUniform1f(1, smoothness);

    glBindBuffer(GL_ARRAY_BUFFER, rectangle_buffer);

    size_t offset = 0;
    int layout_location = 0;

    //buffer square coord data (bounding box of circle)
    real vb[] = {-1,-1,0,
        -1,+1,0,
        +1,+1,0,
        +1,-1,0};
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);

    add_contiguous_attribute(3, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

    //buffer rectangle data
    glBufferSubData(GL_ARRAY_BUFFER, offset, n_rectangles*sizeof(rectangles[0]), (void*) rectangles);
    add_interleaved_attribute(3, GL_FLOAT, false, sizeof(rectangles[0]), 1); //center position
    add_interleaved_attribute(2, GL_FLOAT, false, sizeof(rectangles[0]), 1); //radius
    add_interleaved_attribute(4, GL_FLOAT, false, sizeof(rectangles[0]), 1); //RGBA color

    glDisable(GL_DEPTH_TEST);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, n_rectangles);
    glEnable(GL_DEPTH_TEST);
}

struct line_render_element
{
    real_3 X;
};

void draw_round_line(memory_manager* manager, line_render_info* points, int n_points, real_4x4 camera)
{
    GLuint buffer = gl_general_buffers[0];

    glUseProgram(round_line_program);

    float smoothness = 3.0/(window_height);
    glUniformMatrix4fv(0, 1, true, (GLfloat*) &camera); //t
    glUniform1f(1, smoothness);

    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    size_t offset = 0;
    int layout_location = 0;

    line_render_element* corners = (line_render_element*) stalloc(2*n_points*sizeof(line_render_element)); //xyz, u
    int n_corners = 0;

    real R = 0.0;
    for(int i = 0; i < n_points; i++)
    {
        if(points[i].r > R) R = points[i].r;
    }
    R *= 2.0; //TODO: this is a hack to fix non 2d lines

    real_3 normal_dir = {0,0,1};

    real_3 v1 = normalize(points[1].x-points[0].x);
    real_3 side = normalize(cross(v1, normal_dir))*R;

    corners[n_corners++].X = points[0].x-side-v1*R;
    corners[n_corners++].X = points[0].x+side-v1*R;

    for(int i = 1; i < n_points-1; i++)
    {
        if(points[i+1].x == points[i].x) continue;
        real_3 v2 = normalize(points[i+1].x-points[i].x);
        real_3 new_normal_dir = cross(v1, v2);
        if(normsq(new_normal_dir) > 1.0e-18) new_normal_dir = normalize(new_normal_dir);
        else new_normal_dir = {0, 0, 1};
        normal_dir = (dot(new_normal_dir, normal_dir) > 0) ? new_normal_dir : -new_normal_dir;
        real_3 Cto_corner = (v1-v2);
        real Ccoshalftheta = dot(normalize(cross(v1, normal_dir)), Cto_corner);
        side = Cto_corner*(R/Ccoshalftheta);
        if(Ccoshalftheta == 0.0) side = normalize(cross(v1, normal_dir))*R;

        corners[n_corners++].X = points[i].x-side;
        corners[n_corners++].X = points[i].x+side;

        v1 = v2; //NOTE: make sure this gets unrolled properly
    }
    //TODO: handle wenis seams

    normal_dir = {0,0,1};
    side = normalize(cross(v1, normal_dir))*R;
    corners[n_corners++].X = points[n_points-1].x-side+v1*R;
    corners[n_corners++].X = points[n_points-1].x+side+v1*R;

    //buffer circle data
    glBufferSubData(GL_ARRAY_BUFFER, offset, n_corners*sizeof(corners[0]), (void*) corners);
    add_interleaved_attribute(3, GL_FLOAT, false, sizeof(corners[0]), 0); //vertex position

    glUniform1i(2, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, round_line_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 2, n_points, GL_RGBA, GL_FLOAT, points);

    glUniform1i(3, n_points);

    glEnable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, n_corners);

    stunalloc(corners);
}

real_2 get_text_center(char* text, font_info font)
{
    int ascent, descent, line_gap = 0;
    stbtt_GetFontVMetrics(&font.info, &ascent, &descent, &line_gap);
    real scale = stbtt_ScaleForPixelHeight(&font.info, font.size);

    real current_x = 0;
    real current_y = 0;
    real max_x = 0;
    real max_y = 0;
    real min_y = 0;
    for(char* c = text; *c; c++)
    {
        if(*c == '\n')
        {
            current_x = 0;
            current_y += font.size;
            continue;
        }
        stbtt_aligned_quad quad = {};
        stbtt_GetPackedQuad(font.char_data, font_resolution, font_resolution, *c,
                            &current_x, &current_y, &quad, false);
        max_x = max(max_x, current_x);
        min_y = min(min_y, current_y+descent*scale);
        max_y = max(max_y, current_y+ascent*scale);
    }
    return {0.5f*max_x, 0.5f*(max_y+min_y)};
}

void draw_text(char* text, real x, real y, real_4 color, real_2 alignment, font_info font)
{
    GLuint text_buffer = gl_general_buffers[0];

    glUseProgram(draw_text_program);

    int texture_i = 0;
    int uniform_i = 0;

    // real_2 resolution = {16.0/9.0*window_height, window_height};
    // real_2 resolution = {1920, 1080};
    real_2 resolution = {window_width, window_height};
    // real_2 resolution = {1080.0f*window_width/window_height, 1080};
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

    // real x_base = 0.5*x*1080.0f;
    // real y_base = -0.5*y*1080.0f;
    real x_base = 0.5*x*window_height;
    real y_base = -0.5*y*window_height;

    real_2 center_offset = get_text_center(text, font);
    x_base -= (alignment.x+1)*center_offset.x;
    y_base += (alignment.y+1)*center_offset.y;

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

void draw_rings(ring_render_info* rings, int n_rings, real_4x4 camera, real_3x3 camera_axes, real_3 camera_pos)
{
    GLuint ring_buffer = gl_general_buffers[0];
    GLuint index_buffer = gl_general_buffers[1];

    glUseProgram(ring_program);

    int uniform_i = 0;

    glUniformMatrix4fv(uniform_i++, 1, true, (GLfloat*) &camera); //used for transforming bounding box to screenspace

    //used for actual raycast
    glUniformMatrix3fv(uniform_i++, 1, false, (GLfloat*) &camera_axes);
    glUniform3fv(uniform_i++, 1, (GLfloat*) &camera_pos);
    real fov = 120.0f*pi/180.0f; //TODO: set proper value
    glUniform1f(uniform_i++, fov);
    glUniform2f(uniform_i++, resolution_x, resolution_y);

    uint16 ib[] = {
        0,1,2,3,7,1,5,0,4,2,6,7,4,5,
    };
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(ib), (void*) ib);

    size_t offset = 0;
    int layout_location = 0;

    //buffer cube coord data (stretched to be bounding box of ring)
    real vb[] = {
        -1,-1,-1,
        -1,-1,+1,
        -1,+1,-1,
        -1,+1,+1,
        +1,-1,-1,
        +1,-1,+1,
        +1,+1,-1,
        +1,+1,+1,
    };
    glBindBuffer(GL_ARRAY_BUFFER, ring_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);

    add_contiguous_attribute(3, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

    //buffer ring data
    glBufferSubData(GL_ARRAY_BUFFER, offset, n_rings*sizeof(rings[0]), (void*) rings);
    add_interleaved_attribute(3, GL_FLOAT, false, sizeof(rings[0]), 1); //center position
    add_interleaved_attribute(1, GL_FLOAT, false, sizeof(rings[0]), 1); //ring radius
    add_interleaved_attribute(1, GL_FLOAT, false, sizeof(rings[0]), 1); //profile radius
    add_interleaved_attribute(3, GL_FLOAT, false, sizeof(rings[0]), 1); //axis
    add_interleaved_attribute(4, GL_FLOAT, false, sizeof(rings[0]), 1); //RGBA color

    glDrawElementsInstanced(GL_TRIANGLE_STRIP, len(ib), GL_UNSIGNED_SHORT, (void*) 0, n_rings);
}

void draw_spheres(sphere_render_info* spheres, int n_spheres, real_4x4 camera, real_3x3 camera_axes, real_3 camera_pos)
{
    GLuint sphere_buffer = gl_general_buffers[0];
    GLuint index_buffer = gl_general_buffers[1];

    glUseProgram(sphere_program);

    int uniform_i = 0;

    glUniformMatrix4fv(uniform_i++, 1, true, (GLfloat*) &camera); //used for transforming bounding box to screenspace

    //used for actual raycast
    glUniformMatrix3fv(uniform_i++, 1, false, (GLfloat*) &camera_axes);
    glUniform3fv(uniform_i++, 1, (GLfloat*) &camera_pos);
    real fov = 120.0f*pi/180.0f; //TODO: set proper value
    glUniform1f(uniform_i++, fov);
    glUniform2f(uniform_i++, resolution_x, resolution_y);

    uint16 ib[] = {
        0,1,2,3,7,1,5,0,4,2,6,7,4,5,
    };
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(ib), (void*) ib);

    size_t offset = 0;
    int layout_location = 0;

    //buffer cube coord data (stretched to be bounding box of sphere)
    real vb[] = {
        -1,-1,-1,
        -1,-1,+1,
        -1,+1,-1,
        -1,+1,+1,
        +1,-1,-1,
        +1,-1,+1,
        +1,+1,-1,
        +1,+1,+1,
    };
    glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);

    add_contiguous_attribute(3, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

    //buffer sphere data
    glBufferSubData(GL_ARRAY_BUFFER, offset, n_spheres*sizeof(spheres[0]), (void*) spheres);
    add_interleaved_attribute(3, GL_FLOAT, false, sizeof(spheres[0]), 1); //center position
    add_interleaved_attribute(1, GL_FLOAT, false, sizeof(spheres[0]), 1); //radius
    add_interleaved_attribute(4, GL_FLOAT, false, sizeof(spheres[0]), 1); //RGBA color

    glDrawElementsInstanced(GL_TRIANGLE_STRIP, len(ib), GL_UNSIGNED_SHORT, (void*) 0, n_spheres);
}

void draw_line_3ds(line_3d_render_info* lines, int n_lines, real_4x4 camera, real_3x3 camera_axes, real_3 camera_pos)
{
    GLuint line_buffer = gl_general_buffers[0];
    GLuint index_buffer = gl_general_buffers[1];

    glUseProgram(line_3d_program);

    int uniform_i = 0;

    glUniformMatrix4fv(uniform_i++, 1, true, (GLfloat*) &camera); //used for transforming bounding box to screenspace

    //used for actual raycast
    glUniformMatrix3fv(uniform_i++, 1, false, (GLfloat*) &camera_axes);
    glUniform3fv(uniform_i++, 1, (GLfloat*) &camera_pos);
    real fov = 120.0f*pi/180.0f; //TODO: set proper value
    glUniform1f(uniform_i++, fov);
    glUniform2f(uniform_i++, resolution_x, resolution_y);

    uint16 ib[] = {
        0,1,2,3,7,1,5,0,4,2,6,7,4,5,
    };
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(ib), (void*) ib);

    size_t offset = 0;
    int layout_location = 0;

    //buffer cube coord data (stretched to be bounding box of line)
    real vb[] = {
        -1,-1,-1,
        -1,-1,+1,
        -1,+1,-1,
        -1,+1,+1,
        +1,-1,-1,
        +1,-1,+1,
        +1,+1,-1,
        +1,+1,+1,
    };
    glBindBuffer(GL_ARRAY_BUFFER, line_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);

    add_contiguous_attribute(3, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

    //buffer line data
    glBufferSubData(GL_ARRAY_BUFFER, offset, n_lines*sizeof(lines[0]), (void*) lines);
    add_interleaved_attribute(3, GL_FLOAT, false, sizeof(lines[0]), 1); //center position
    add_interleaved_attribute(1, GL_FLOAT, false, sizeof(lines[0]), 1); //length
    add_interleaved_attribute(1, GL_FLOAT, false, sizeof(lines[0]), 1); //radius
    add_interleaved_attribute(3, GL_FLOAT, false, sizeof(lines[0]), 1); //axis
    add_interleaved_attribute(4, GL_FLOAT, false, sizeof(lines[0]), 1); //RGBA color

    glDrawElementsInstanced(GL_TRIANGLE_STRIP, len(ib), GL_UNSIGNED_SHORT, (void*) 0, n_lines);
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

void render_editor_voxels(editor_data* ed)
{
    glUseProgram(render_editor_voxels_program);

    int texture_i = 0;
    int uniform_i = 0;

    //camera
    glUniformMatrix3fv(uniform_i++, 1, false, (GLfloat*) &ed->rd.camera_axes);
    glUniform3fv(uniform_i++, 1, (GLfloat*) &ed->rd.camera_pos);

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

    //load form materials to gpu
    ed->texture_space.n_free_regions = 0;
    ed->texture_space.free_regions[ed->texture_space.n_free_regions++] = {{0,0,0}, ed->texture_space.max_size};

    gpu_object_data* objects_gpu = (gpu_object_data*) stalloc(ed->n_objects*sizeof(gpu_object_data));
    int n_visible_objects = 0;
    int selected_object_index = -1;
    for(int i = 0; i < ed->n_objects; i++)
    {
        editor_object* o = ed->objects+i;

        if(o->is_world)
        {
            int_3 size = dim(o->modified_region);
            if(size.x > 0 && size.y > 0 && size.z > 0)
            {
                glBindTexture(GL_TEXTURE_3D, materials_textures[current_materials_texture]);

                uint8* modified_materials = stalloc(axes_product(size));
                for(int z = 0; z < size.z; z++)
                    for(int y = 0; y < size.y; y++)
                        for(int x = 0; x < size.x; x++)
                        {
                            modified_materials[index_3D({x,y,z}, size)] = o->materials[index_3D((int_3){x,y,z}+o->modified_region.l-o->region.l, dim(o->region))];
                        }
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexSubImage3D(GL_TEXTURE_3D, 0,
                                o->modified_region.l.x, o->modified_region.l.y, o->modified_region.l.z,
                                size.x, size.y, size.z,
                                GL_RED_INTEGER, GL_UNSIGNED_BYTE,
                                modified_materials);
                stunalloc(modified_materials);

                int_3 pos = o->modified_region.l;
                int_3 active_data_size = {(int(pos.x)%16+size.x+15)/16, (int(pos.y)%16+size.y+15)/16, (int(pos.z)%16+size.z+15)/16};
                uint* active_data = (uint*) stalloc(axes_product(active_data_size)*sizeof(uint));
                for(int i = 0; i < active_data_size.x*active_data_size.y*active_data_size.z; i++)
                    active_data[i] = {0xFFFFFFFF};
                glBindTexture(GL_TEXTURE_3D, active_regions_textures[current_active_regions_texture]);
                glTexSubImage3D(GL_TEXTURE_3D, 0,
                                pos.x/16, pos.y/16, pos.z/16,
                                active_data_size.x, active_data_size.y, active_data_size.z,
                                GL_RED_INTEGER, GL_UNSIGNED_INT,
                                active_data);
                stunalloc(active_data);

                glBindTexture(GL_TEXTURE_3D, form_materials_texture);

                o->modified_region = {{room_size, room_size, room_size}, {0,0,0}};
            }
            continue;
        }
        if(!o->visible) continue;

        if(o == ed->selected_object) selected_object_index = n_visible_objects;

        int_3 size = dim(o->region);

        if(size == (int_3){0,0,0}) continue;

        gpu_object_data* object_gpu = objects_gpu+n_visible_objects++;
        *object_gpu = {
            .texture_region = add_region(&ed->texture_space, size),
            .origin_to_lower = o->region.l,
            .x = o->x,
            .orientation = o->orientation,
            .tint = o->tint,
            .highlight = o->highlight,
        };

        assert((object_gpu->texture_region.u != (int_3){0,0,0})); //TODO: defrag if add_region fails

        //TODO: only update based on modified_region
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        object_gpu->texture_region.l.x, object_gpu->texture_region.l.y, object_gpu->texture_region.l.z,
                        size.x, size.y, size.z,
                        GL_RED_INTEGER, GL_UNSIGNED_BYTE,
                        o->materials);
        o->modified_region = {{room_size, room_size, room_size}, {0,0,0}};
    }

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, blue_noise_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    static int frame_number = 0;
    glUniform1i(uniform_i++, frame_number++);
    glUniform1i(uniform_i++, n_visible_objects);

    glUniform1f(uniform_i++, pi*120.0f/180.0f);

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

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, editor_object_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(objects_gpu[0])*n_visible_objects, objects_gpu);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, editor_object_buffer);

    stunalloc(objects_gpu);

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

void cast_lightprobes()
{
    glUseProgram(cast_probes_program);

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

    int ssbo_i = 0;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, probe_rays_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_i++, probe_rays_buffer);

    glDispatchCompute(lightprobes_w,lightprobes_h,1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void update_lightprobes()
{
    {
        glUseProgram(update_lightprobe_color_program);

        int texture_i = 0;
        int uniform_i = 0;
        int image_i = 0;

        static int frame_number = 0;
        glUniform1i(uniform_i++, frame_number++);

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

        glUniform1i(uniform_i++, image_i);
        glBindImageTexture(image_i++, lightprobe_color_textures[1-current_lightprobe_texture], 0, true, 0, GL_READ_WRITE, GL_RGBA16F);

        int ssbo_i = 0;

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, probe_rays_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_i++, probe_rays_buffer);

        glDispatchCompute(lightprobes_w,lightprobes_h,1);
    }

    {
        glUseProgram(update_lightprobe_depth_program);

        int texture_i = 0;
        int uniform_i = 0;
        int image_i = 0;

        static int frame_number = 0;
        glUniform1i(uniform_i++, frame_number++);

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

        glUniform1i(uniform_i++, image_i);
        glBindImageTexture(image_i++, lightprobe_depth_textures[1-current_lightprobe_texture], 0, true, 0, GL_READ_WRITE, GL_RG16F);

        int ssbo_i = 0;

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, probe_rays_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_i++, probe_rays_buffer);

        glDispatchCompute(lightprobes_w,lightprobes_h,1);
    }

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    current_lightprobe_texture = 1-current_lightprobe_texture;
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glGenerateMipmap(GL_TEXTURE_2D);

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_2D, lightprobe_depth_textures[current_lightprobe_texture]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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
    for(int i = 0; i < 3; i++)
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

void draw_beams(real_4x4 camera, real_3x3 camera_axes, world* w)
{
    glUseProgram(draw_beams_program);

    int texture_i = 0;
    int uniform_i = 0;

    glUniformMatrix4fv(uniform_i++, 1, true, (GLfloat*) &camera);
    glUniformMatrix3fv(uniform_i++, 1, false, (GLfloat*) &camera_axes);
    glUniform1i(uniform_i++, w->n_beams);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, beam_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, beam_buffer);

    const real phi = 0.5*(1.0+sqrt(5.0));
    const real a = 1.0/sqrt(phi+2);
    const real b = phi*a;

    real_2 vb[] = {
        {0,-1},
        {0,+1},
        {1,-1},
        {1,+1},
    };

    GLuint vertex_buffer = gl_general_buffers[0];

    size_t offset = 0;
    int layout_location = 0;

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vb), (void*) vb);
    add_contiguous_attribute(2, GL_FLOAT, false, 0, sizeof(vb)); //vertex positions

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, len(vb), w->n_beams);
}

void move_lightprobes()
{
    glUseProgram(move_lightprobes_program);

    int texture_i = 0;
    int uniform_i = 0;

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

void cast_rays(world* w)
{
    if(w->n_rays == 0) return;
    glUseProgram(cast_rays_program);

    int texture_i = 0;
    int image_i = 0;
    int uniform_i = 0;
    int buffer_i = 0;

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
    glBindTexture(GL_TEXTURE_3D, occupied_regions_texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, w->n_rays);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, rays_in_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ray_in)*w->n_rays, w->rays_in);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_i++, rays_in_buffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, rays_out_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_i++, rays_out_buffer);

    glDispatchCompute(w->n_rays,1,1);

    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT|GL_SHADER_STORAGE_BARRIER_BIT);

    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ray_out)*w->n_rays, w->rays_out);

    w->n_rays = 0;
}

void update_beams(world* w)
{ //TODO: merge this into the general ray casts
    if(w->n_beams == 0) return;
    glUseProgram(update_beams_program);

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
    glBindTexture(GL_TEXTURE_3D, occupied_regions_texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, image_i);
    glBindImageTexture(image_i++, active_regions_textures[current_active_regions_texture], 0, true, 0, GL_WRITE_ONLY, GL_R8UI);

    glUniform1i(uniform_i++, w->n_beams);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, beam_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(beam_data)*w->n_beams, w->beams);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, beam_buffer);

    glDispatchCompute(w->n_beams,1,1);

    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT|GL_SHADER_STORAGE_BARRIER_BIT);
}

void simulate_world_cells(world* w, int update_cells)
{
    glUseProgram(simulate_chunk_atomic_program);

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

    glUniform1i(uniform_i++, w->n_explosions);
    glUniform1i(uniform_i++, w->n_beams);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_buffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, explosion_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(explosion_data)*w->n_explosions, w->explosions);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, explosion_buffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, beam_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, beam_buffer);

    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT|GL_TEXTURE_UPDATE_BARRIER_BIT);

    glDispatchCompute(32,32,32);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    current_materials_texture = 1-current_materials_texture;
}

void edit_world(world* w)
{
    glUseProgram(edit_world_program);

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

    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT|GL_TEXTURE_UPDATE_BARRIER_BIT);

    glDispatchCompute(32,32,32);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    current_materials_texture = 1-current_materials_texture;
}

size_t encode_world_cells(rle_run* data)
{
    glUseProgram(encode_world_cells_program);

    int texture_i = 0;
    int image_i = 0;
    int uniform_i = 0;
    int ssbo_i = 0;

    glUniform1i(uniform_i++, texture_i);
    glActiveTexture(GL_TEXTURE0+texture_i++);
    glBindTexture(GL_TEXTURE_3D, materials_textures[current_materials_texture]);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, world_cell_encoding_buffer);
    int clear = 0;
    glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32I, 0, sizeof(int), GL_RED_INTEGER, GL_INT, &clear);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_i++, world_cell_encoding_buffer);

    glDispatchCompute(64,64,1);

    glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32I, 0, sizeof(int), GL_RED_INTEGER, GL_INT, &clear);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(encode_world_cells_scan_program);
    glDispatchCompute(64,64,1);

    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

    int runs_size = 0;
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(int)*room_size*room_size, sizeof(int), &runs_size);
    uint data_size = 4*runs_size;
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(int)*(room_size*room_size+1), data_size, data);
    return data_size;
}

void decode_world_cells(rle_run* data, int size)
{
    glUseProgram(decode_world_cells_scan_program);

    int texture_i = 0;
    int image_i = 0;
    int uniform_i = 0;
    int ssbo_i = 0;

    glUniform1i(uniform_i++, image_i);
    glBindImageTexture(image_i++, materials_textures[current_materials_texture],
                       0, true, 0, GL_READ_WRITE, GL_RGBA8UI);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, world_cell_encoding_buffer);
    int next_range_clear = 0;
    glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32I, 0, sizeof(int), GL_RED_INTEGER, GL_INT, &next_range_clear);
    int total_runs_size = size/4;
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(int)*room_size*room_size, sizeof(int), &total_runs_size);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(int)*(room_size*room_size+1), size, data);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_i++, world_cell_encoding_buffer);

    int n_work_groups = 3;

    glDispatchCompute(n_work_groups,1,1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(decode_world_cells_program);
    glDispatchCompute(n_work_groups,1,1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(decode_world_cells_write_program);
    glDispatchCompute(n_work_groups,1,1);

    glBindTexture(GL_TEXTURE_3D, active_regions_textures[current_active_regions_texture]);
    uint clear = 0x1;
    glClearTexImage(active_regions_textures[current_active_regions_texture], 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &clear);

    glFinish();

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

// //the RLE functions above are faster and smaller so there's no reason to use these
// void download_world_cells(world_cell* data)
// {
//     glActiveTexture(GL_TEXTURE0);
//     glBindTexture(GL_TEXTURE_3D, materials_textures[1-current_materials_texture]);
//     glGetTexImage(GL_TEXTURE_3D, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, data);
//     glBindTexture(GL_TEXTURE_3D, 0);
// }
// void upload_world_cells(world_cell* data)
// {
//     glActiveTexture(GL_TEXTURE0);
//     glBindTexture(GL_TEXTURE_3D, materials_textures[current_materials_texture]);
//     glTexSubImage3D(GL_TEXTURE_3D, 0,
//                     0, 0, 0,
//                     room_size, room_size, room_size,
//                     GL_RED_INTEGER, GL_UNSIGNED_SHORT,
//                     data);
//     glBindTexture(GL_TEXTURE_3D, 0);
// }

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

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void load_bodies_to_gpu(world* w)
{
    //clear body space and reallocate things,
    //TODO: have some persistance so only regions with body cells that have changed need to be reuploaded
    w->body_space.n_free_regions = 0;
    w->body_space.free_regions[w->body_space.n_free_regions++] = {{0,0,0}, w->body_space.max_size};

    for(int b = 0; b < w->n_bodies; b++)
    {
        gpu_body_data* body_gpu = w->bodies_gpu+b;
        cpu_body_data* body_cpu = w->bodies_cpu+b;

        int_3 size = dim(body_cpu->region);
        body_gpu->texture_region = add_region(&w->body_space, size);
        body_gpu->origin_to_lower = body_cpu->region.l;

        // log_output("loading body ", body_cpu->id, " to gpu in region (", body_gpu->texture_region.l, "," , body_gpu->texture_region.u, ")\n");

        glBindTexture(GL_TEXTURE_3D, body_materials_textures[current_body_materials_texture]);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        body_gpu->texture_region.l.x, body_gpu->texture_region.l.y, body_gpu->texture_region.l.z,
                        size.x, size.y, size.z,
                        GL_RGBA_INTEGER, GL_UNSIGNED_BYTE,
                        body_cpu->materials);
    }
}

void find_body_contacts(memory_manager* manager, world* w)
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

    glUniform1i(uniform_i++, w->n_bodies);

    glUniform1i(uniform_i++, w->n_beams);

    int ssbo_i = 0;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, body_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(w->bodies_gpu[0])*w->n_bodies, w->bodies_gpu);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_i++, body_buffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, body_updates_buffer);
    int clear = 0;
    glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32I, 0, sizeof(body_update)*w->n_bodies, GL_RED_INTEGER, GL_INT, &clear);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_i++, body_updates_buffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_i++, particle_buffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, beam_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_i++, beam_buffer);

    int n_contacts = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, contacts_buffer);
    glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32I, 0, sizeof(int), GL_RED_INTEGER, GL_INT, &n_contacts);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_i++, contacts_buffer);

    const int n_max_body_indices = 1024*1024;
    size_t body_indices_size = sizeof(int)*n_max_body_indices;
    size_t collision_grid_size = sizeof(int_2)*collision_cells_per_axis*collision_cells_per_axis*collision_cells_per_axis;

    byte* memory = stalloc(collision_grid_size+sizeof(int)+body_indices_size);
    byte* data = memory;

    int_2* collision_grid = (int_2*) data;
    data += collision_grid_size;

    int* n_body_indices = (int*) data;
    *n_body_indices = 0;
    data += sizeof(int);

    int* body_indices = (int*) data;
    data += body_indices_size;

    for(int z = 0; z < collision_cells_per_axis; z++)
        for(int y = 0; y < collision_cells_per_axis; y++)
            for(int x = 0; x < collision_cells_per_axis; x++)
            {
                int index = index_3D({x,y,z}, collision_cells_per_axis);
                collision_cell* cell = &w->collision_grid[index];
                int start_index = (*n_body_indices);
                for(int i = 0; i < cell->n_bodies; i++)
                {
                    body_indices[(*n_body_indices)++] = cell->body_indices[i];
                }
                collision_grid[index] = {start_index, cell->n_bodies};
            }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, collision_grid_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, collision_grid_size+(1+(*n_body_indices))*sizeof(int), collision_grid);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_i++, collision_grid_buffer);

    stunalloc(memory);

    uint* body_chunks = (uint*) stalloc(w->n_bodies*8*4*sizeof(uint));
    uint n_body_chunks = 0;
    for(int b = 0; b < w->n_bodies; b++)
    {
        cpu_body_data* body_cpu = &w->bodies_cpu[b];
        gpu_body_data* body_gpu = &w->bodies_gpu[b];

        const int pad = 1;
        int_3 padding = {pad, pad, pad};
        int_3 lower = body_gpu->texture_region.l;
        int_3 upper = body_gpu->texture_region.u;

        for(int x = lower.x; x <= upper.x; x+=body_chunk_size)
        for(int y = lower.y; y <= upper.y; y+=body_chunk_size)
        for(int z = lower.z; z <= upper.z; z+=body_chunk_size)
        {
            body_chunks[4*n_body_chunks+0] = b;
            body_chunks[4*n_body_chunks+1] = x;
            body_chunks[4*n_body_chunks+2] = y;
            body_chunks[4*n_body_chunks+3] = z;
            n_body_chunks++;
        }
    }

    glUniform1i(uniform_i++, n_body_chunks);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, body_chunks_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (1+4*n_body_chunks)*sizeof(uint), body_chunks);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_i++, body_chunks_buffer);

    stunalloc(body_chunks);

    glDispatchCompute(n_body_chunks,1,1);

    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    current_body_materials_texture = 1-current_body_materials_texture;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, contacts_buffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &w->n_contacts);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(int), sizeof(contact_point)*w->n_contacts, w->contacts);
}

// void simulate_body_physics(memory_manager* manager, world* w, render_data* rd)
// {
//     glUseProgram(simulate_body_physics_program);

//     int texture_i = 0;
//     int image_i = 0;
//     int uniform_i = 0;

//     static int frame_number = 0;
//     glUniform1i(uniform_i++, frame_number++);

//     glUniform1i(uniform_i++, texture_i);
//     glActiveTexture(GL_TEXTURE0+texture_i++);
//     glBindTexture(GL_TEXTURE_2D, material_physical_properties_texture);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

//     glUniform1i(uniform_i++, texture_i);
//     glActiveTexture(GL_TEXTURE0+texture_i++);
//     glBindTexture(GL_TEXTURE_3D, materials_textures[current_materials_texture]);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
//     glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

//     glUniform1i(uniform_i++, image_i);
//     glBindImageTexture(image_i++, body_materials_textures[current_body_materials_texture], 0, true, 0, GL_READ_WRITE, GL_RGBA8UI);

//     int ssbo_i = 0;

//     glBindBuffer(GL_SHADER_STORAGE_BUFFER, body_buffer);
//     // glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(bodies_gpu[0])*n_bodies, bodies_gpu);
//     glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_i++, body_buffer);

//     size_t offset = 0;
//     int layout_location = 0;

//     glBindBuffer(GL_SHADER_STORAGE_BUFFER, body_updates_buffer);
//     glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_i++, body_updates_buffer);

//     glDispatchCompute(w->n_bodies,1,1);

//     glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT|GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

//     size_t body_updates_size = sizeof(body_update)*w->n_bodies;

//     byte* memory = stalloc(body_updates_size);
//     byte* data = memory;

//     body_update* body_updates = (body_update*) data;
//     data += body_updates_size;

//     glBindBuffer(GL_SHADER_STORAGE_BUFFER, body_updates_buffer);
//     glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(body_update)*w->n_bodies, body_updates);

//     int n_bodies = w->n_bodies;
//     for(int b = n_bodies-1; b >= 0; b--)
//     {
//         cpu_body_data* body_cpu = &w->bodies_cpu[b];
//         gpu_body_data* body_gpu = &w->bodies_gpu[b];
//         if(body_updates[b].n_fragments == 0 || body_updates[b].m < 0.0015)
//         {
//             body_gpu->substantial = false;
//             if(body_cpu->form_id == 0)
//             {
//                 delete_body(w, body_cpu->id);
//                 continue;
//             }
//         }
//         else
//         {
//             body_gpu->substantial = true;
//             body_gpu->m = body_updates[b].m;
//             real_3 new_x_cm = body_updates[b].x_cm;
//             real_3 deltax_cm = new_x_cm-body_gpu->x_cm;
//             body_gpu->x_cm = new_x_cm;
//             body_gpu->x += apply_rotation(body_gpu->orientation, deltax_cm);
//             body_cpu->I = {
//                 body_updates[b].I_xx, body_updates[b].I_xy, body_updates[b].I_xz,
//                 body_updates[b].I_xy, body_updates[b].I_yy, body_updates[b].I_yz,
//                 body_updates[b].I_xz, body_updates[b].I_yz, body_updates[b].I_zz,
//             };
//             //parallel axis theorem to recenter inertia about center of mass
//             body_cpu->I -= real_identity_3(body_gpu->m*(0.4*sq(0.5)+normsq(body_gpu->x_cm)));
//             for(int i = 0; i < 3; i++)
//                 for(int j = 0; j < 3; j++)
//                     body_cpu->I[i][j] -= -body_gpu->m*body_gpu->x_cm[i]*body_gpu->x_cm[j];
//             body_cpu->invI = inverse(body_cpu->I);
//             update_inertia(body_cpu, body_gpu);

//             body_gpu->fragment_id = 1;
//         }

//         body_gpu->material_bounds.l = body_updates[b].lower;
//         body_gpu->material_bounds.u = body_updates[b].upper;

//         for(int i = 1; i < body_updates[b].n_fragments; i++)
//         {
//             int bi = new_body_index(w);
//             cpu_body_data* new_body_cpu = &w->bodies_cpu[bi];
//             gpu_body_data* new_body_gpu = &w->bodies_gpu[bi];

//             if(i == 1) body_cpu->first_fragment_index = w->n_body_fragments;
//             body_cpu->n_fragments++;
//             w->body_fragments[w->n_body_fragments++] = new_body_cpu->id;

//             new_body_gpu->brain_id = body_gpu->brain_id;
//             new_body_cpu->genome_id = body_cpu->genome_id;
//             new_body_cpu->form_id = body_cpu->form_id;
//             new_body_cpu->is_root = body_cpu->is_root;
//             new_body_cpu->root = body_cpu->root;
//             new_body_cpu->I = body_cpu->I;
//             new_body_cpu->invI = body_cpu->invI;
//             new_body_cpu->storage_level = sm_gpu;
//             //TODO: actually figure out which fragment should keep the form, need to check based on joints

//             *new_body_gpu = *body_gpu;
//             new_body_gpu->fragment_id = i+1;
//             int_3 size = dim(body_gpu->texture_region);
//             new_body_gpu->texture_region = add_region(&w->body_space, size);
//             new_body_gpu->form_origin += body_gpu->texture_region.l-new_body_gpu->texture_region.l;
//             new_body_gpu->form_lower = {0,0,0};
//             new_body_gpu->form_upper = {0,0,0};

//             int_3 src_pos = body_gpu->texture_region.l;
//             int_3 dst_pos = new_body_gpu->texture_region.l;
//             glCopyImageSubData(body_materials_textures[current_body_materials_texture], GL_TEXTURE_3D, 0,
//                                src_pos.x, src_pos.y, src_pos.z,
//                                body_materials_textures[current_body_materials_texture], GL_TEXTURE_3D, 0,
//                                dst_pos.x, dst_pos.y, dst_pos.z,
//                                size.x, size.y, size.z);
//         }
//     }

//     stunalloc(memory);
// }

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
    x = clamp_per_axis(x, 1, room_size-2);

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    // glViewport(0, 0, room_size, room_size);

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

#endif
