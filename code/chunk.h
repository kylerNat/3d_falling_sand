#ifndef CHUNK
#define CHUNK

#define chunk_size 256

enum storage_mode
{
    sm_disk,
    sm_compressed,
    sm_memory,
    sm_gpu,
};

struct chunk
{
    uint16 materials[chunk_size*chunk_size*chunk_size];
    GLuint materials_texture;
    GLuint old_materials_texture;
    int storage_level;

    chunk* neighbors[6]; //UDLRFB
};

#define N_MAX_MATERIALS_TEXTURES 16
GLuint materials_textures[N_MAX_MATERIALS_TEXTURES];
int n_materials_textures = 0;

void load_chunk_to_gpu(chunk* c)
{
    //TODO: handle lower storage levels
    if(c->storage_level < sm_gpu)
    {
        c->materials_texture = materials_textures[n_materials_textures++];
        c->old_materials_texture = materials_textures[n_materials_textures++];

        glBindTexture(GL_TEXTURE_3D, c->materials_texture);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        0, 0, 0,
                        chunk_size, chunk_size, chunk_size,
                        GL_RED_INTEGER, GL_UNSIGNED_SHORT,
                        c->materials);
        c->storage_level = sm_gpu;
    }
}

void load_body_to_gpu(body* b)
{
    if(b->storage_level < sm_gpu)
    {
        b->materials_texture = materials_textures[n_materials_textures++];

        glBindTexture(GL_TEXTURE_3D, b->materials_texture);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        0, 0, 0,
                        b->size.x, b->size.y, b->size.z,
                        GL_RED_INTEGER, GL_UNSIGNED_SHORT,
                        b->materials);
        b->storage_level = sm_gpu;
    }
}

#endif //CHUNK
