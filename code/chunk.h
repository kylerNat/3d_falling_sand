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
#define body_chunk_size 4
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

void load_body_to_gpu(cuboid_space* body_space, cpu_body_data* bc, gpu_body_data* bg)
{
    if(bc->storage_level < sm_gpu)
    {
        int_3 size = bg->material_bounds.u-bg->material_bounds.l;
        bg->texture_region = add_region(body_space, size);

        glBindTexture(GL_TEXTURE_3D, body_materials_textures[current_body_materials_texture]);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        bg->texture_region.l.x, bg->texture_region.l.y, bg->texture_region.l.z,
                        size.x, size.y, size.z,
                        GL_RED_INTEGER, GL_UNSIGNED_BYTE,
                        bc->materials);
        bc->storage_level = sm_gpu;
    }
}

void load_empty_body_to_gpu(cuboid_space* body_space, cpu_body_data* bc, gpu_body_data* bg, int_3 size)
{
    if(bc->storage_level < sm_gpu)
    {
        int pad = 1;
        int_3 padding = (int_3){pad,pad,pad};
        int_3 padded_size = size+2*padding;
        bg->texture_region = add_region(body_space, padded_size);
        bg->material_bounds = {};

        bc->storage_level = sm_gpu;
    }
}

void resize_body_storage(cuboid_space* body_space, cpu_body_data* body_cpu, gpu_body_data* body_gpu, int_3 offset, int_3 new_size)
{
    if(new_size == body_gpu->texture_region.u-body_gpu->texture_region.l) return;

    bounding_box new_region = add_region(body_space, new_size);

    assert(new_region.u != ((int_3){0,0,0}));

    uint clear = 0;
    glClearTexSubImage(body_materials_textures[current_body_materials_texture], 0, new_region.l.x, new_region.l.y, new_region.l.z, new_size.x, new_size.y, new_size.z, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, &clear);

    int_3 src_pos = body_gpu->texture_region.l+body_gpu->material_bounds.l;
    int_3 dst_pos = new_region.l+offset;
    int_3 old_size = body_gpu->material_bounds.u-body_gpu->material_bounds.l;
    glCopyImageSubData(body_materials_textures[current_body_materials_texture], GL_TEXTURE_3D, 0,
                       src_pos.x, src_pos.y, src_pos.z,
                       body_materials_textures[current_body_materials_texture], GL_TEXTURE_3D, 0,
                       dst_pos.x, dst_pos.y, dst_pos.z,
                       old_size.x, old_size.y, old_size.z);

    free_region(body_space, body_gpu->texture_region);

    body_gpu->texture_region = new_region;
    body_gpu->form_offset += offset;
    body_gpu->material_bounds.u += offset;
    body_gpu->material_bounds.l += offset;
};

#endif //CHUNK
