#ifndef ROOM
#define ROOM

#include <zlib.h>
#include <lz4.h>

struct room_gpu_entry;

struct room
{
    uint16* materials;
    int storage_level;

    int_3 room_pos;

    room_gpu_entry* gpu_info;
};

#pragma pack(push, 1)
struct rle_run
{
    uint8 material;
    uint8 run_length;
};
#pragma pack(pop)

GLuint materials_textures[2];
int current_materials_texture = 0;

#define body_texture_size 256
#define body_chunk_size 4
GLuint body_materials_textures[2];
int current_body_materials_texture = 0;

struct room_gpu_entry
{
    room* c;
    int8_2 memory_pos;
};

int8_2 load_room_to_gpu(room* c)
{
    //TODO: handle lower storage levels

    int8_2 offset = {0,0};

    if(c->storage_level < sm_gpu)
    {
        glBindTexture(GL_TEXTURE_3D, materials_textures[current_materials_texture]);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        0, 0, 0,
                        room_size, room_size, room_size,
                        GL_RED_INTEGER, GL_UNSIGNED_SHORT,
                        c->materials);
        c->storage_level = sm_gpu;
    }
    return offset;
}

size_t compress_world_cells(rle_run* cells, size_t size, byte* compressed_data)
{
    // //lz4 ///////////////////////////////////////
    // unsigned long raw_size = room_size*room_size*room_size*sizeof(world_cell);
    // int compressed_size = LZ4_compress_default((char*) cells, (char*) compressed_data, raw_size, raw_size);
    // return compressed_size;

    //zlib ///////////////////////////////////////
    unsigned long raw_size = room_size*room_size*room_size*sizeof(world_cell);
    unsigned long compressed_size = raw_size;
    int level = 9;
    int error = compress2(compressed_data, &compressed_size, (byte*) cells, size, level);
    if(error != Z_OK)
    {
        log_warning("ZLIB compression failed, error ", error);
        return 0;
    }
    return compressed_size;
}

size_t decompress_world_cells(rle_run* cells, byte* compressed_data, size_t compressed_size)
{
    // //lz4 ///////////////////////////////////////
    // unsigned long raw_size = room_size*room_size*room_size*sizeof(world_cell);
    // int compressed_size = LZ4_compress_default((char*) cells, (char*) compressed_data, raw_size, raw_size);
    // return compressed_size;

    //zlib ///////////////////////////////////////
    unsigned long raw_size = room_size*room_size*room_size*sizeof(world_cell);
    unsigned long size = raw_size;
    int error = uncompress((byte*) cells, &size, compressed_data, (unsigned long) compressed_size);
    if(error != Z_OK)
    {
        log_warning("ZLIB decompression failed, error ", error);
        return 0;
    }
    return size;
}

#endif //ROOM
