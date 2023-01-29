#include <utils/misc.h>
#include "memory.h"

#include <dirent.h>
#include <stdio.h>
#include <utils/logging.h>

#define is_space(a) ((a) == ' ' || (a) == '\t')

#define is_newline(a) ((a) == '\n' || (a) == '\r')
#define is_lowercase(a) (('a' <= (a) && (a) <= 'z'))
#define is_uppercase(a) (('A' <= (a) && (a) <= 'Z'))
#define is_lowercasehex(a) (('a' <= (a) && (a) <= 'f'))
#define is_uppercasehex(a) (('A' <= (a) && (a) <= 'F'))
#define is_wordstart(a) (is_lowercase(a) || is_uppercase(a) || (a) == '_')
#define is_wordchar(a) (is_wordstart(a) || ('0' <= (a) && (a) <= '9'))
#define is_digit(a) (('0' <= (a) && (a) <= '9'))
#define is_hexdigit(a) (is_digit(a) || ('a' <= (a) && (a) <= 'f') || ('A' <= (a) && (a) <= 'F'))
#define is_bit(a) (('0' <= (a) && (a) <= '9'))

void skip_whitespace(char*& code)
{
    while(*code && is_space(*code)) code++;
}

local int line_number = 0;
local int file_number = 0;
local int max_file_number = 0;

void skip_whitespace_and_newline(char*& code)
{
    while(*code && (is_space(*code) || is_newline(*code)))
    {
        if(is_newline(*code)) line_number++;
        code++;
    }
}

void skip_newline(char*& code)
{
    while(*code && is_newline(*code))
    {
        line_number++;
        code++;
    }
}

void goto_next_line(char*& code)
{
    while(*code++ != '\n') line_number++;
}

#define CODE(source) #source

#define N_MAX_SHADERS 10 //max shader stages per program
#define MAX_SHADER_NAME_LENGTH 1024
#define MAX_FILE_NAME_LENGTH 1024
#define MAX_SHADER_SIZE 1024*1024

#define MAX_DEPTH 10

local char* program_name;
local char* shader_typename;

local GLuint current_program;
local GLuint shaders[N_MAX_SHADERS];
local char *shader_names[N_MAX_SHADERS];
local int n_shaders = 0;
local bool writing_shader = false;
local bool writing_program = false;

local char* shader_source;
local char* shader_source_start;

GLenum shader_type_from_typename(char* shader_typename)
{
    #define check_shader_type(SHADER_TYPE) const char SHADER_TYPE##_string[] = #SHADER_TYPE; if(strncmp(shader_typename, SHADER_TYPE##_string, len(SHADER_TYPE##_string)-1) == 0) return SHADER_TYPE;

    check_shader_type(GL_VERTEX_SHADER);
    check_shader_type(GL_TESS_CONTROL_SHADER);
    check_shader_type(GL_TESS_EVALUATION_SHADER);
    check_shader_type(GL_GEOMETRY_SHADER);
    check_shader_type(GL_FRAGMENT_SHADER);
    check_shader_type(GL_COMPUTE_SHADER);

    #undef check_shader_type

    return 0;
}

void start_shader(char* shader_typename)
{
    shader_names[n_shaders] = shader_typename;
    shaders[n_shaders] = glCreateShader(shader_type_from_typename(shader_typename));
    assert(shaders[n_shaders], "could not create shader of type", shader_typename);

    shader_source_start = (char*) stalloc(MAX_SHADER_SIZE);
    shader_source = shader_source_start;
    shader_source += sprintf(shader_source, "#version %s\n", GLSL_VERSION);

    writing_shader = true;
}

void end_shader()
{
    writing_shader = false;

    *shader_source++ = 0;
    glShaderSource(shaders[n_shaders], 1, &shader_source_start, 0);

    glCompileShader(shaders[n_shaders]);

    int error = 0;
    glGetShaderiv(shaders[n_shaders], GL_COMPILE_STATUS, &error);

    if(error == GL_FALSE)
    {
        char info_log[4096];
        GLsizei info_log_len = -1;
        glGetShaderInfoLog(shaders[n_shaders], sizeof(info_log), &info_log_len, info_log);

        char processed_info_log[4096];
        char* c = info_log;
        char* d = processed_info_log;
        bool line_start = true;
        while(*c)
        {
            if(*c == '0' && line_start)
            {
                d += sprintf(d, "%s", program_name);
                c++;
                line_start = false;
            }
            else
            {
                line_start = *c == '\n';
                *d++ = *c++;
            }
        }
        *d++ = 0;
        log_error("could not compile ", program_name, ":", shader_names[n_shaders], ":\n", processed_info_log);
    }
    glAttachShader(current_program, shaders[n_shaders]);

    n_shaders++;

    stunalloc(shader_source_start);
}

void start_program()
{
    current_program = glCreateProgram();
    assert(current_program, "could not create ", program_name, ", GL error ", glGetError());
    n_shaders = 0;

    writing_shader = false;
    writing_program = true;
}

void end_program()
{
    writing_program = false;
    glLinkProgram(current_program);

    for(int i = 0; i < n_shaders; i++)
    {
        glDetachShader(current_program, shaders[i]);
        glDeleteShader(shaders[i]);
    }

    int error = 0;
    glGetProgramiv(current_program, GL_LINK_STATUS, &error);
    if(error == 0)
    {
        char info_log[4096];
        glGetProgramInfoLog(current_program, sizeof(info_log), 0, info_log);

        char processed_info_log[4096];
        char* c = info_log;
        char* d = processed_info_log;
        bool line_start = true;
        while(*c)
        {
            if(*c == '0' && line_start)
            {
                d += sprintf(d, "%s", program_name);
                c++;
                line_start = false;
            }
            else
            {
                line_start = *c == '\n';
                *d++ = *c++;
            }
        }
        *d++ = 0;

        log_error("could not compile ", program_name, "\n", processed_info_log);
    }
}

void insert_line_directive()
{
    if(writing_shader) shader_source += sprintf(shader_source, "#line %d %d\n", line_number, file_number);
}

void process_file(char* filename, int depth, char* parent_filename)
{
    file_number = max_file_number++;
    line_number = 0;
    insert_line_directive();

    if(depth > MAX_DEPTH)
    {
        log_output("error: max #include depth(", MAX_DEPTH, ") exceeded\n");
        exit(1);
    }

    // log_output("processing ", filename, "\n");

    FILE* file = fopen(filename, "r");
    if(!file)
    {
        log_output(parent_filename, ":0:0: error: could not open ", filename, "\n");
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* in = (char*) dynamic_alloc(file_size+1);
    file_size = fread(in, 1, file_size, file);
    fclose(file);
    in[file_size] = 0;

    char* s = in;

    skip_whitespace_and_newline(s);

    while(*(s))
    {
        if(*s == '#')
        {
            {
                const char command[] = "include";
                if(strncmp(s+1, command, len(command)-1) == 0)
                {
                    while(*(++s) != '"');
                    char include_filename[MAX_FILE_NAME_LENGTH];
                    char* include_filename_source_start = s+1;
                    while(*(++s) != '"');
                    size_t include_filename_length = s-include_filename_source_start-1;
                    *s++ = 0;
                    sprintf(include_filename, "%s%s", shader_path, include_filename_source_start);
                    int old_line_number = line_number;
                    int old_file_number = file_number;
                    process_file(include_filename, depth+1, filename);
                    line_number = old_line_number+1;
                    file_number = old_file_number;
                    insert_line_directive();
                    continue;
                }
            }

            {
                const char command[] = "shader";
                if(strncmp(s+1, command, len(command)-1) == 0)
                {
                    while(*(++s) != ' ');
                    char* shader_typename = s+1;
                    while(*(++s) != '\n');
                    *s++ = 0;
                    line_number++;
                    if(writing_shader) end_shader();
                    start_shader(shader_typename);
                    continue;
                }
            }
        }

        if(writing_shader)
        {
            *(shader_source++) = *s;
        }

        if(is_newline(*s)) line_number++;

        s++;
    }

    dynamic_free(in);
}

GLuint load_shader(char *filename)
{
    char filepath[1024];
    sprintf(filepath, "%s%s", shader_path, filename);

    program_name = filename;
    start_program();

    max_file_number = 0;
    process_file(filename, 0, shader_path);

    if(writing_shader) end_shader();
    end_program();

    return current_program;
}
