#ifndef GRAPHICS
#define GRAPHICS

#include <gl/gl.h>
#include <utils/logging.h>
#include <utils/misc.h>

#include "graphics_common.h"
#include "game_common.h"
#include "chunk.h"

struct program_attribute
{
    GLint dim; //dimension
    GLenum type;
};

struct program_info
{
    GLuint program;
};

struct shader_source
{
    GLenum type;
    char* filename;
    char* source;
};

struct vi_attribute
{
    GLuint index;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride;
    GLvoid* pointer;
};

struct vi_buffer
{
    int n_attribs;
    vi_attribute* attribs;

    GLuint vertex_buffer;
    GLuint index_buffer;
    GLuint n_vertex_buffer;
    GLuint n_index_buffer;
};

#define DEBUG_SEVERITY_HIGH                           0x9146
#define DEBUG_SEVERITY_MEDIUM                         0x9147
#define DEBUG_SEVERITY_LOW                            0x9148

void APIENTRY gl_error_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
    #ifndef GL_SUPPRESS_WARNINGS
    switch(severity)
    {
        case DEBUG_SEVERITY_HIGH:
            // assert(0, "HIGH: ", message);
            log_warning("HIGH: ");
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

vi_buffer create_vertex_and_index_buffer(uint vb_size, void * vb_data,
                                         uint ib_size, void * ib_data,
                                         uint n_attribs, vi_attribute* attribs)
{
    vi_buffer out = {};
    glGenBuffers(1, &out.vertex_buffer);//TODO: maybe do this in bulk?
    glBindBuffer(GL_ARRAY_BUFFER, out.vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, vb_size*4, vb_data, GL_STATIC_DRAW);//TODO: use glNamedBufferData if available

    glGenBuffers(1, &out.index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ib_size*4, ib_data, GL_STATIC_DRAW);

    out.n_vertex_buffer = vb_size;
    out.n_index_buffer = ib_size;
    out.attribs = attribs;
    out.n_attribs = n_attribs;

    return out;
}

inline void bind_vertex_and_index_buffers(vi_buffer vi)
{
    glBindBuffer(GL_ARRAY_BUFFER, vi.vertex_buffer);
    // assert(pinfo.n_attribs >= vi.n_attribs, "number of attributes does not match");
    for(int i = 0; i < vi.n_attribs; i++)
    {
        //TODO: actually search through and match indices
        // assert(pinfo.attribs[i].index == vi.attribs[i].index);
        // assert(pinfo.attribs[i].size == vi.attribs[i].size);
        // assert(pinfo.attribs[i].type == vi.attribs[i].type);

        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i,
                              vi.attribs[i].size,
                              vi.attribs[i].type,
                              vi.attribs[i].normalized,
                              vi.attribs[i].stride,
                              vi.attribs[i].pointer);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vi.index_buffer);
}

GLuint init_shader(byte* free_memory, size_t available_memory, shader_source source)
{
    GLuint shader = glCreateShader(source.type);
    assert(shader, "could not create shader for ", source.filename);
    glShaderSource(shader, 1, &source.source, 0);
    glCompileShader(shader);

    // log_output("compiling shader:\n", source.source, "\n\n");
    int error;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &error);
    if(error == GL_FALSE)
    {
        int info_log_len = -1;
        char* info_log = (char*) free_memory;
        glGetShaderInfoLog(shader, available_memory, &info_log_len, info_log);
        log_output("info log is ", info_log_len, " characters long\n");
        log_error("could not compile shader ", source.filename, ":\n", info_log);
        //TODO: replace the filename in the info_log with the correct filename
        //TODO: don't abort immediately so all errors are displayed
    }

    return shader;
}

GLuint init_program(memory_manager* manager, size_t n_shaders, shader_source* sources)
{
    GLuint program = glCreateProgram();
    assert(program, "could not create program, GL error ", glGetError());

    byte* free_memory = reserve_block(manager, n_shaders*sizeof(GLuint));
    size_t available_memory = current_block_unused(manager);

    GLuint* shaders = (GLuint*) free_memory;
    for(int i = 0; i < n_shaders; i++)
    {
        shaders[i] = init_shader(free_memory+sizeof(shaders[0])*i, available_memory-sizeof(shaders[0])*i, sources[i]);
        glAttachShader(program, shaders[i]);
    }

    glLinkProgram(program);

    int error;
    glGetProgramiv(program, GL_LINK_STATUS, &error);
    if(error == 0)
    {
        char* info_log = (char*) free_memory;
        glGetProgramInfoLog(program, available_memory, 0, info_log);
        log_error(info_log);
    }

    for(int i = 0; i < n_shaders; i++)
    {
        glDetachShader(program, shaders[i]);
        glDeleteShader(shaders[i]);
    }
    unreserve_block(manager);
    return program;
}

#define DEFAULT_HEADER "#version 440\n#define pi 3.14159265358979323846264338327950\n"
#define SHADER_SOURCE(source) #source

struct attribute_value
{
    GLuint index;
    int divisor;
    int stride;
    int offset_width;
};

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

#define N_MAX_PARTICLES 4096
GLuint particles_x_texture;
GLuint particles_x_dot_texture;

#define N_MAX_CAPSULES 4096
GLuint capsules_texture;

GLuint collision_point_texture;
GLuint collision_normal_texture;

void gl_init_general_buffers(memory_manager* manager)
{
    glGenBuffers(N_GENERAL_BUFFERS, gl_general_buffers);
    for(int i = 0; i < N_GENERAL_BUFFERS; i++)
    {
        glBindBuffer(GL_ARRAY_BUFFER, gl_general_buffers[i]);
        glBufferData(GL_ARRAY_BUFFER, GENERAL_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
    }

    // glGenTextures(1, &round_line_texture);
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, round_line_texture);
    // glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 2, N_MAX_LINE_POINTS);

    glGenTextures(N_MAX_MATERIALS_TEXTURES, materials_textures);
    for(int i = 0; i < N_MAX_MATERIALS_TEXTURES; i++)
    {
        glBindTexture(GL_TEXTURE_3D, materials_textures[i]);
        glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGB16I, chunk_size, chunk_size, chunk_size);
    }

    glGenFramebuffers(1, &frame_buffer);
    // glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

    glGenTextures(1, &particles_x_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, particles_x_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, 1, N_MAX_PARTICLES);

    glGenTextures(1, &particles_x_dot_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, particles_x_dot_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, 1, N_MAX_PARTICLES);

    // glGenTextures(1, &capsules_texture);
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, capsules_texture);
    // glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, 3, N_MAX_CAPSULES);

    glGenTextures(1, &collision_point_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, collision_point_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, chunk_size, chunk_size);

    glGenTextures(1, &collision_normal_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, collision_normal_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, chunk_size, chunk_size);
}



#define add_attribute_common(dim, gl_type, do_normalize, stride, offset_advance, divisor) \
    glEnableVertexAttribArray(layout_location);                         \
    glVertexAttribPointer(layout_location, dim, gl_type, do_normalize, stride, (void*) (offset)); \
    offset += offset_advance;                                           \
    glVertexAttribDivisor(layout_location++, divisor);

#define add_contiguous_attribute(dim, gl_type, do_normalize, stride, offset_advance) \
    add_attribute_common(dim, gl_type, do_normalize, stride, offset_advance, 0);

#define add_interleaved_attribute(dim, gl_type, do_normalize, stride, divisor)  \
    add_attribute_common(dim, gl_type, do_normalize, stride, dim*gl_get_type_size(gl_type), divisor);

#define define_program(program_name, shader_list) \
    program_info pinfo_##program_name = {};
#include "gl_shader_list.h"
#undef define_program

void gl_load_programs(memory_manager* manager)
{
    #define define_program(program_name, shader_list)                   \
        {                                                               \
            shader_source sources[] = {NOPAREN shader_list};            \
            pinfo_##program_name.program = init_program(manager, len(sources), sources); \
        }
    #include "gl_shader_list.h"
    #undef define_program
}

void draw_sprites(GLuint spritesheet_texture, sprite_render_info* sprites, int n_sprites, real_4x4 camera)
{
    GLuint sprite_buffer = gl_general_buffers[0];

    glUseProgram(pinfo_sprite.program);

    glUniformMatrix4fv(0, 1, true, (GLfloat*) &camera);
    glUniform1f(1, spritesheet_texture);

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
    add_interleaved_attribute(1, GL_FLOAT, false, sizeof(sprites[0]), 1); //radius
    add_interleaved_attribute(2, GL_FLOAT, false, sizeof(sprites[0]), 1); //lower texture coord
    add_interleaved_attribute(2, GL_FLOAT, false, sizeof(sprites[0]), 1); //upper texture coordinate

    glDisable(GL_DEPTH_TEST);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, n_sprites);
    glEnable(GL_DEPTH_TEST);
}

void draw_circles(circle_render_info* circles, int n_circles, real_4x4 camera)
{
    GLuint circle_buffer = gl_general_buffers[0];

    glUseProgram(pinfo_circle.program);

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

void draw_rectangles(rectangle_render_info* rectangles, int n_rectangles, real_4x4* camera)
{
    GLuint rectangle_buffer = gl_general_buffers[0];

    glUseProgram(pinfo_rectangle.program);

    glUniformMatrix4fv(0, 1, false, (GLfloat*) camera);
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

    glUseProgram(pinfo_round_line.program);

    float smoothness = 3.0/(window_height);
    glUniformMatrix4fv(0, 1, true, (GLfloat*) &camera); //t
    glUniform1f(1, smoothness);

    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    size_t offset = 0;
    int layout_location = 0;

    line_render_element* corners = (line_render_element*) reserve_block(manager, 2*n_points*sizeof(line_render_element)); //xyz, u
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
    //TODO: check packing of the c structs
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

    unreserve_block(manager);
}

void render_voxels(GLuint materials_texture, real_3x3 camera_axes, real_3 camera_pos, int_3 size)
{
    glUseProgram(pinfo_render_chunk.program);

    int texture_i = 0;
    int uniform_i = 0;

    //camera
    glUniformMatrix3fv(uniform_i++, 1, false, (GLfloat*) &camera_axes);
    glUniform3fv(uniform_i++, 1, (GLfloat*) &camera_pos);

    glUniform1i(uniform_i++, texture_i++);
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, materials_texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    // glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform3iv(uniform_i++, 1, (GLint*) &size);

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
    render_voxels(c->materials_texture, camera_axes, camera_pos, {chunk_size, chunk_size, chunk_size});
}

void render_body(body* b, real_3x3 camera_axes, real_3 camera_pos)
{
    load_body_to_gpu(b);
    render_voxels(b->materials_texture, apply_rotation(conjugate(b->orientation), camera_axes), apply_rotation(conjugate(b->orientation), camera_pos - b->x)+b->x_cm, b->size);
}

void simulate_chunk(chunk* c)
{
    glUseProgram(pinfo_simulate_chunk.program);

    load_chunk_to_gpu(c);

    int texture_i = 0;
    int uniform_i = 0;

    int layer_number_uniform = uniform_i++;

    static int frame_number = 0;
    glUniform1i(uniform_i++, frame_number++);

    glUniform1i(uniform_i++, texture_i++);
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, c->materials_texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

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

    glViewport(0, 0, chunk_size, chunk_size);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

    for(int l = 0; l < chunk_size; l++)
    {
        glUniform1i(layer_number_uniform, l);

        glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D,
                               c->old_materials_texture, 0, l);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    auto temp = c->materials_texture;
    c->materials_texture = c->old_materials_texture;
    c->old_materials_texture = temp;
}

void simulate_particles(chunk* c, real_3* xs, real_3* x_dots, int n_particles)
{
    glUseProgram(pinfo_simulate_particles.program);

    int texture_i = 0;
    int uniform_i = 0;

    static int frame_number = 0;
    glUniform1i(uniform_i++, frame_number++);

    glUniform1i(uniform_i++, texture_i++);
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, c->materials_texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i++);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, particles_x_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glUniform1i(uniform_i++, texture_i++);
    glActiveTexture(GL_TEXTURE0 + 2);
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

void collide_body(memory_manager* manager, chunk* c, body* b, collision_point* collisions, int* n_collisions, int n_max_collisions)
{
    load_body_to_gpu(b);
    glUseProgram(pinfo_collide_body.program);

    int texture_i = 0;
    int uniform_i = 0;

    static int frame_number = 0;
    glUniform1i(uniform_i++, frame_number++);

    glUniform1i(uniform_i++, texture_i++);
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, c->materials_texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glUniform1i(uniform_i++, texture_i++);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_3D, b->materials_texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glUniform3fv(uniform_i++, 1, (GLfloat*) &b->x);
    real_3x3 body_axes = apply_rotation(b->orientation, real_identity_3(1));
    real_3x3 inv_body_axes = transpose(body_axes); //body_axes should be SO(3) so the transpose should be the inverse.
    glUniformMatrix3fv(uniform_i++, 1, false, (GLfloat*) &inv_body_axes);

    glUniform3fv(uniform_i++, 1, (GLfloat*) &b->x_cm);

    real_3 bbl = {0,0,0};
    real_3 bbu = {0,0,0};
    for(int i = 0; i < 8; i++)
    {
        int_3 dir = {
            (i>>0)&1,
            (i>>1)&1,
            (i>>2)&1
        };
        real_3 corner = apply_rotation(b->orientation, -b->x_cm+real_cast(multiply_components(b->size, dir)));
        bbl = min_per_axis(bbl, corner);
        bbu = max_per_axis(bbu, corner);
    }
    bbl += b->x;
    bbu += b->x;

    glUniform3fv(uniform_i++, 1, (GLfloat*) &bbl);
    glUniform3fv(uniform_i++, 1, (GLfloat*) &bbu);

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
    int_2 output_size = {bbu.x-bbl.x+1.0, bbu.y-bbl.y+1.0};
    glViewport(0, 0, output_size.x, output_size.y);
    glClearColor(0,0,0,0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, collision_point_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, collision_normal_texture, 0);

    GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(len(buffers), buffers);

    glDisable(GL_BLEND);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glEnable(GL_BLEND);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);

    size_t point_buffer_size  = output_size.x*output_size.y*sizeof(real_4);
    size_t normal_buffer_size = output_size.x*output_size.y*sizeof(real_3);
    byte* memory = reserve_block(manager, point_buffer_size+normal_buffer_size);
    real_4* collision_point_buffer = (real_4*) memory;
    real_3* collision_normal_buffer = (real_3*) memory+point_buffer_size;

    glGetTextureSubImage(collision_point_texture,
                         0,
                         0, 0, 0,
                         output_size.x, output_size.y, 1,
                         GL_RGBA,
                         GL_FLOAT,
                         output_size.x*output_size.y*sizeof(real_4),
                         collision_point_buffer);

    glGetTextureSubImage(collision_normal_texture,
                         0,
                         0, 0, 0,
                         output_size.x, output_size.y, 1,
                         GL_RGB,
                         GL_FLOAT,
                         output_size.x*output_size.y*sizeof(real_4),
                         collision_normal_buffer);
    *n_collisions = 0;
    for(int y = 0; y < output_size.y; y++)
        for(int x = 0; x < output_size.x; x++)
        {
            int i = x+y*output_size.x;
            if(collision_point_buffer[i].x > 0.0)
            {
                // log_output("collision ", *n_collisions, ": ", collision_point_buffer[i].x, ", ", collision_point_buffer[i].y, ", ", collision_point_buffer[i].z, "\n");
                collisions[*n_collisions].x      =  collision_point_buffer[i].xyz;
                collisions[*n_collisions].depth  =  collision_point_buffer[i].w;
                collisions[*n_collisions].normal = collision_normal_buffer[i];
                if(++(*n_collisions) > n_max_collisions) goto collision_find_end;
            }
        }
collision_find_end:;
    unreserve_block(manager);
}

void get_chunk_data(chunk* c, int_3 x, int_3 dim, uint16* data)
{
    glGetTextureSubImage(c->materials_texture,
                         0,
                         x.x, x.y, x.z,
                         dim.x, dim.y, dim.z,
                         GL_RED,
                         GL_UNSIGNED_SHORT,
                         dim.x*dim.y*dim.z*sizeof(uint16)*2,
                         data);
}

#endif
