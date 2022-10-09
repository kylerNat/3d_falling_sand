#ifndef CHUNK
#define CHUNK

#define chunk_size 512

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

#define body_texture_size 256
GLuint body_materials_textures[2];
int current_body_materials_texture = 0;

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
        int padding = 2;
        int size = 32+padding;
        int bodies_per_row = (body_texture_size-2*padding)/size;
        bg->materials_origin = {padding+size*(n_bodies%bodies_per_row),padding+size*((n_bodies/bodies_per_row)%bodies_per_row),padding+size*(n_bodies/sq(bodies_per_row))};
        n_bodies++;

        glBindTexture(GL_TEXTURE_3D, body_materials_textures[current_body_materials_texture]);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        bg->materials_origin.x, bg->materials_origin.y, bg->materials_origin.z,
                        bg->size.x, bg->size.y, bg->size.z,
                        GL_RED_INTEGER, GL_UNSIGNED_BYTE,
                        bc->materials);
        bc->storage_level = sm_gpu;
    }
}

#endif //CHUNK
