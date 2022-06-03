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

struct chunk_gpu_entry;

struct chunk
{
    uint16* materials;
    int storage_level;

    int_3 chunk_pos;

    chunk_gpu_entry* gpu_info;
};

#define gpu_n_chunks_wide 4
#define MAX_GPU_CHUNKS (gpu_n_chunks_wide*gpu_n_chunks_wide)
#define materials_textures_size (chunk_size*gpu_n_chunks_wide)
GLuint materials_textures[2];
int current_materials_texture = 0;

GLuint body_materials_textures[2];
int n_body_materials_textures = 0;

struct chunk_gpu_entry
{
    chunk* c;
    int8_2 memory_pos;
};

chunk_gpu_entry chunk_storage_offsets[MAX_GPU_CHUNKS];
int n_stored_chunks = 0;
int chunk_storage_offsets_start = 0;

int8_2 load_chunk_to_gpu(chunk* c)
{
    //TODO: handle lower storage levels

    int8_2 offset = {0,0};

    if(c->storage_level < sm_gpu)
    {
        if(n_stored_chunks >= MAX_GPU_CHUNKS) //unload oldest chunk
        {
            chunk_gpu_entry* oldest_entry = &chunk_storage_offsets[chunk_storage_offsets_start];
            offset = oldest_entry->memory_pos;
            oldest_entry->c->storage_level--;
            oldest_entry->c->gpu_info = 0;
            oldest_entry->c = c;
            c->gpu_info = oldest_entry;
            chunk_storage_offsets_start++;

            //TODO: copy unloaded chunk back to cpu memory
        }
        else
        {
            if(n_stored_chunks >= 1)
            {
                chunk_gpu_entry* latest_entry = &chunk_storage_offsets[chunk_storage_offsets_start+n_stored_chunks-1];
                offset = latest_entry->memory_pos;
                offset.x += 1;
                if(offset.x >= gpu_n_chunks_wide)
                {
                    offset.x %= gpu_n_chunks_wide;
                    offset.y += 1;
                    offset.y %= gpu_n_chunks_wide;
                }
            }
            chunk_storage_offsets[chunk_storage_offsets_start+(n_stored_chunks++)] = {c, offset};
        }
        // log_output("offset: ", offset.x, ", ", offset.y, "\n");

        glBindTexture(GL_TEXTURE_3D, materials_textures[current_materials_texture]);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        chunk_size*offset.x, chunk_size*offset.y, 0,
                        chunk_size, chunk_size, chunk_size,
                        GL_RED_INTEGER, GL_UNSIGNED_SHORT,
                        c->materials);
        c->storage_level = sm_gpu;
    }
    return offset;
}

void load_body_to_gpu(cpu_body_data* bc, gpu_body_data* bg)
{
    if(bg->storage_level < sm_gpu)
    {
        bc->materials_texture = body_materials_textures[n_body_materials_textures++];
        bg->materials_origin = {0,0,0};

        glBindTexture(GL_TEXTURE_3D, bc->materials_texture);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        0, 0, 0,
                        bg->size.x, bg->size.y, bg->size.z,
                        GL_RED_INTEGER, GL_UNSIGNED_SHORT,
                        bc->materials);
        bc->storage_level = sm_gpu;
    }
}

#endif //CHUNK
