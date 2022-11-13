#include <dirent.h>
#include <stdio.h>
#include <utils/logging.h>

const char* shader_path = "../code/shaders/";

#define MAX_DEPTH 10

//TODO: is there a reason I didn't just use '\t'?
#define TAB 9 //tab character

#define isspace(a) ((a) == ' ' || (a) == TAB)

#define isnewline(a) ((a) == '\n' || (a) == '\r')
#define islowercase(a) (('a' <= (a) && (a) <= 'z'))
#define isuppercase(a) (('A' <= (a) && (a) <= 'Z'))
#define islowercasehex(a) (('a' <= (a) && (a) <= 'f'))
#define isuppercasehex(a) (('A' <= (a) && (a) <= 'F'))
#define iswordstart(a) (islowercase(a) || isuppercase(a) || (a) == '_')
#define iswordchar(a) (iswordstart(a) || ('0' <= (a) && (a) <= '9'))
#define isdigit(a) (('0' <= (a) && (a) <= '9'))
#define ishexdigit(a) (isdigit(a) || ('a' <= (a) && (a) <= 'f') || ('A' <= (a) && (a) <= 'F'))
#define isbit(a) (('0' <= (a) && (a) <= '9'))

void skip_whitespace(char*& code)
{
    while(*code && isspace(*code)) code++;
}

void skip_whitespace_and_newline(char*& code)
{
    while(*code && (isspace(*code) || isnewline(*code))) code++;
}

void skip_newline(char*& code)
{
    while(*code && isnewline(*code)) code++;
}

void goto_next_line(char*& code)
{
    while(*code++ != '\n');
}



#define CODE(source) #source

#define N_MAX_SHADERS 10 //per program
#define MAX_SHADER_NAME_LENGTH 1000
#define MAX_FILE_NAME_LENGTH 1000

char* program_name;

char shader_names[N_MAX_SHADERS][MAX_SHADER_NAME_LENGTH];
int n_shaders = 0;
bool writing_shader = false;
bool writing_program = false;

char* body;
char* header;

void start_shader(char* shader_type)
{
    sprintf(shader_names[n_shaders++], "%s_%s", program_name, shader_type);
    // log_output(shader_names[n_shaders-1], "\n");

    body += _sprintf_p(body, -1,
                    CODE(\n
                        GLuint %1$s = glCreateShader(%2$s);
                        assert(%1$s, "could not create %1$s, GL error", glGetError());
                        ), shader_names[n_shaders-1], shader_type);

    body += sprintf(body, "{const char* source = \"");
    writing_shader = true;
}

void end_shader()
{
    writing_shader = false;
    body += _sprintf_p(body, -1,
                    "\"; glShaderSource(%2$s, 1, &source, 0);}"
                    CODE(
                        glCompileShader(%2$s);

                        glGetShaderiv(%2$s, GL_COMPILE_STATUS, &error);
                        if(error == GL_FALSE)
                        {
                            GLsizei info_log_len = -1;
                            char* info_log = (char*) free_memory;
                            glGetShaderInfoLog(%2$s, available_memory, &info_log_len, info_log);
                            log_output("info log is ", info_log_len, " characters long\n");
                            log_error("could not compile %2$s:\n", info_log);
                        }
                        glAttachShader(%1$s, %2$s);
                        \n), program_name, shader_names[n_shaders-1]);
}

void start_program()
{
    header += sprintf(header,
                      CODE(\nGLuint %s;), program_name);

    body += _sprintf_p(body, -1,
                       CODE(
                           %1$s = glCreateProgram();
                           assert(%1$s, "could not create %1$s, GL error", glGetError());
                           ), program_name);
    n_shaders = 0;

    writing_program = true;
}

void end_program()
{
    writing_program = false;
    body += sprintf(body,
                    CODE(
                        glLinkProgram(%s);
                        ), program_name);

    for(int i = 0; i < n_shaders; i++)
    {
        body += _sprintf_p(body, -1,
                        CODE(
                            glDetachShader(%1$s, %2$s);
                            glDeleteShader(%2$s);
                            \n), program_name, shader_names[i]);
    }

    body += _sprintf_p(body, -1,
                    CODE(
                        glGetProgramiv(%1$s, GL_LINK_STATUS, &error);
                        if(error == 0)
                        {
                            char* info_log = (char*) free_memory;
                            glGetProgramInfoLog(%1$s, available_memory, 0, info_log);
                            log_error(info_log);
                        }
                        ), program_name);
}

void start_gl_init_programs()
{
    body += sprintf(body,
                    CODE(
                        void gl_init_programs(memory_manager* manager)
                        )
                    "{"
                    CODE(
                        int error;
                        size_t available_memory = current_block_unused(manager); //for error logging
                        byte* free_memory = reserve_block(manager, available_memory);
                        ));
}

void end_gl_init_programs()
{
    body += sprintf(body,
                    CODE(
                        unreserve_block(manager);
                        )
                    "}");
}

void process_file(char* filename, int depth, char* parent_filename)
{
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
    char* in = (char*) malloc(file_size+1);
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
                    process_file(include_filename, depth+1, filename);
                    continue;
                }
            }

            {
                const char command[] = "shader";
                if(strncmp(s+1, command, len(command)-1) == 0)
                {
                    while(*(++s) != ' ');
                    char* shader_type = s+1;
                    while(*(++s) != '\n');
                    *s++ = 0;
                    if(writing_shader) end_shader();
                    start_shader(shader_type);
                    continue;
                }
            }

            {
                const char command[] = "program";
                if(strncmp(s+1, command, len(command)-1) == 0)
                {
                    while(*(++s) != ' ');
                    char* new_program_name = s+1;
                    while(*(++s) != '\n');
                    *s++ = 0;
                    if(writing_shader) end_shader();
                    if(writing_program) end_program();

                    program_name = new_program_name;
                    start_program();
                    continue;
                }
            }
        }

        if(writing_shader)
        {
            if(*s == '\n')
            {
                *(body++) = '\\';
                *(body++) = 'n';
            }
            else
            {
                *(body++) = *s;
            }
        }

        s++;
    }

    // free(in);
}

int main(int n_args, char** args)
{
    const char* output_filename = "../code/generated_shader_list.h";

    FILE* output_file = fopen(output_filename, "w");

    char* body_start = (char*) malloc(1024*1024*1024);
    body = body_start;

    char* header_start = (char*) malloc(1024*1024*1024);
    header = header_start;

    start_gl_init_programs();

    DIR* dir = opendir(shader_path);
    dirent* entry;
    while((entry = readdir(dir)))
    {
        if(entry->d_type == DT_REG)
        {
            char filename[MAX_FILE_NAME_LENGTH];
            sprintf(filename, "%s%s", shader_path, entry->d_name);
            process_file(filename, 0, (char*) shader_path);
        }
    }

    closedir(dir);

    if(writing_shader) end_shader();
    if(writing_program) end_program();
    end_gl_init_programs();

    // log_output(header_start, "\n\n");
    // log_output(body_start, "\n\n");

    fwrite(header_start, 1, header-header_start, output_file);
    fwrite(body_start, 1, body-body_start, output_file);

    fclose(output_file);
    return 0;
}
