#ifndef CHUNK
#define CHUNK

#define chunk_size 512

enum storage_mode
{
    sm_disk,
    sm_compressed,
    sm_memory,
    sm_gpu,
};

struct chunk_gpu_entry;

struct chunk
{
    uint16* materials;
    int storage_level;

    int_3 chunk_pos;

    chunk_gpu_entry* gpu_info;
};

GLuint materials_textures[2];
int current_materials_texture = 0;

#define body_texture_size 128
GLuint body_materials_textures[2];
int current_body_materials_texture = 0;

GLuint body_forces_texture;
GLuint body_shifts_texture;

struct chunk_gpu_entry
{
    chunk* c;
    int8_2 memory_pos;
};

int8_2 load_chunk_to_gpu(chunk* c)
{
    //TODO: handle lower storage levels

    int8_2 offset = {0,0};

    if(c->storage_level < sm_gpu)
    {
        glBindTexture(GL_TEXTURE_3D, materials_textures[current_materials_texture]);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        0, 0, 0,
                        chunk_size, chunk_size, chunk_size,
                        GL_RED_INTEGER, GL_UNSIGNED_SHORT,
                        c->materials);
        c->storage_level = sm_gpu;
    }
    return offset;
}

int n_bodies = 0;
void load_body_to_gpu(cpu_body_data* bc, gpu_body_data* bg)
{
    // static bool has_loaded = false;
    // if(has_loaded) return;
    // has_loaded = true;
    if(bc->storage_level < sm_gpu)
    {
        bc->materials_texture = body_materials_textures[1];
        bg->materials_origin = {1+16*(n_bodies%8),1+16*((n_bodies/8)%8),1+16*(n_bodies/64)};
        n_bodies++;

        glBindTexture(GL_TEXTURE_3D, bc->materials_texture);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        bg->materials_origin.x, bg->materials_origin.y, bg->materials_origin.z,
                        bg->size.x, bg->size.y, bg->size.z,
                        GL_RED_INTEGER, GL_UNSIGNED_SHORT,
                        bc->materials);
        bc->storage_level = sm_gpu;
    }
}

#endif //CHUNK
