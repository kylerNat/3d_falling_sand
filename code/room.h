#ifndef ROOM
#define ROOM

#include <zlib.h>
#include <lz4.h>

#define room_size 512

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

size_t compress_world_cells(world_cell* cells, byte* compressed_data)
{
    // //lz4 ///////////////////////////////////////
    // unsigned long raw_size = room_size*room_size*room_size*sizeof(world_cell);
    // //TODO: compressed data might be larger than uncompressed in some cases
    // int compressed_size = LZ4_compress_default((char*) cells, (char*) compressed_data, raw_size, raw_size);
    // return compressed_size;

    // //zlib ///////////////////////////////////////
    // unsigned long raw_size = room_size*room_size*room_size*sizeof(world_cell);
    // //TODO: compressed data might be larger than uncompressed in some cases
    // unsigned long compressed_size = raw_size;
    // int level = 9;
    // int error = compress2(compressed_data, &compressed_size, (byte*) cells, raw_size, level);
    // if(error != Z_OK)
    // {
    //     log_warning("ZLIB compression failed, error ", error);
    //     return 0;
    // }
    // return compressed_size;

    // //run length encoding in zyx order ///////////////////////////////////////

    // union {
    //     world_cell run_cell;
    //     uint32 run_cell_data;
    // };
    // run_cell = cells[0];
    // int run_length = 0;
    // byte* out = compressed_data;

    // for(int x = 0; x < room_size; x++)
    //     for(int y = 0; y < room_size; y++)
    //         for(int z = 0; z < room_size; z++)
    //         {
    //             union {
    //                 world_cell cell;
    //                 uint32 cell_data;
    //             };
    //             cell = cells[index_3D({x,y,z}, room_size)];
    //             if(cell_data != run_cell_data || run_length >= 255)
    //             {
    //                 *((uint32*) out) = cell_data;
    //                 out += sizeof(cell_data);
    //                 *out++ = run_length;

    //                 run_cell = cell;
    //                 run_length = 0;
    //             }
    //             else run_length++;
    //         }
    // if(run_length > 0)
    // {
    //     *((uint32*) out) = cell_data;
    //     out += sizeof(cell_data);
    //     *out++ = run_length;
    // }
    // return out - compressed_data;

    // run length encoding materials only ///////////////////////////////////////

    union {
        world_cell run_cell;
        uint32 run_cell_data;
    };
    run_cell = cells[0];
    int run_length = 0;
    byte* out = compressed_data;
    byte* start = out;

    for(int x = 0; x < room_size; x++)
        for(int y = 0; y < room_size; y++)
            for(int z = 0; z < room_size; z++)
            {
                union {
                    world_cell cell;
                    uint32 cell_data;
                };
                cell = cells[index_3D({x,y,z}, room_size)];
                if(cell.material != run_cell.material || run_length >= 255)
                {
                    *((uint8*) out) = run_cell.material;
                    out += sizeof(run_cell.material);
                    *out++ = run_length;

                    run_cell = cell;
                    run_length = 0;
                }
                else run_length++;
            }
    if(run_length > 0)
    {
        *((uint8*) out) = run_cell.material;
        out += sizeof(run_cell.material);
        *out++ = run_length;
    }
    unsigned long rle_size = out-start;
    return rle_size;

    // // rle+zlib ///////////////////////////////////////
    // unsigned long raw_size = room_size*room_size*room_size*sizeof(world_cell);

    // world_cell run_cell;
    // run_cell = cells[0];
    // int run_length = 0;
    // byte* out = compressed_data+raw_size/2;
    // byte* start = out;

    // for(int i = 0;; i++)
    // {
    //     world_cell cell;
    //     int x = i/(room_size*room_size);
    //     int y = (i/room_size)%room_size;
    //     int z = i%room_size;
    //     bool last_cell = i == room_size*room_size*room_size;
    //     if(!last_cell) cell = cells[index_3D({x,y,z}, room_size)];
    //     if(cell.data32 != run_cell.data32 || run_length >= 255 || last_cell)
    //     {
    //         *((uint8*) out) = run_cell.data32;
    //         out += sizeof(run_cell.data32);
    //         *out++ = run_length;

    //         if(last_cell) break;

    //         run_cell = cell;
    //         run_length = 0;
    //     }
    //     else run_length++;
    // }

    // unsigned long rle_size = out-start;

    // //TODO: compressed data might be larger than uncompressed in some cases
    // unsigned long compressed_size = raw_size/2;
    // int level = 9;
    // int error = compress2(compressed_data, &compressed_size, start, rle_size, level);
    // if(error != Z_OK)
    // {
    //     log_warning("ZLIB compression failed, error ", error);
    //     return 0;
    // }
    // return compressed_size;

    // // rle per channel+zlib ///////////////////////////////////////
    // unsigned long raw_size = room_size*room_size*room_size*sizeof(world_cell);

    // union {
    //     world_cell run_cell;
    //     uint32 run_cell_data;
    // };
    // run_cell = cells[0];
    // int run_length = 0;
    // byte* out = compressed_data+raw_size/2;
    // byte* start = out;

    // for(int j = 0; j < 4; j++)
    // {
    //     if(j == 1 || j == 2 || j == 3) continue;
    //     for(int i = 0;; i++)
    //     {
    //         union {
    //             world_cell cell;
    //             uint32 cell_data;
    //         };
    //         int x = i/(room_size*room_size);
    //         int y = (i/room_size)%room_size;
    //         int z = i%room_size;
    //         bool last_cell = i == room_size*room_size*room_size;
    //         if(!last_cell) cell = cells[index_3D({x,y,z}, room_size)];
    //         if(cell.data[j] != run_cell.data[j] || run_length >= 255 || last_cell)
    //         {
    //             *((uint8*) out) = run_cell.data[j];
    //             out += sizeof(run_cell.data[j]);
    //             *out++ = run_length;

    //             if(last_cell) break;

    //             run_cell = cell;
    //             run_length = 0;
    //         }
    //         else run_length++;
    //     }
    // }

    // unsigned long rle_size = out-start;

    // //TODO: compressed data might be larger than uncompressed in some cases
    // unsigned long compressed_size = raw_size/2;
    // int level = 9;
    // int error = compress2(compressed_data, &compressed_size, start, rle_size, level);
    // if(error != Z_OK)
    // {
    //     log_warning("ZLIB compression failed, error ", error);
    //     return 0;
    // }
    // return compressed_size;

    // // rle+lz4 ///////////////////////////////////////
    // unsigned long raw_size = room_size*room_size*room_size*sizeof(world_cell);

    // union {
    //     world_cell run_cell;
    //     uint32 run_cell_data;
    // };
    // run_cell = cells[0];
    // int run_length = 0;
    // byte* out = compressed_data+raw_size/2;
    // byte* start = out;

    // for(int x = 0; x < room_size; x++)
    //     for(int y = 0; y < room_size; y++)
    //         for(int z = 0; z < room_size; z++)
    //         {
    //             union {
    //                 world_cell cell;
    //                 uint32 cell_data;
    //             };
    //             cell = cells[index_3D({x,y,z}, room_size)];
    //             if(cell.depth != run_cell.depth || run_length >= 255)
    //             {
    //                 *((uint8*) out) = run_cell.depth;
    //                 out += sizeof(run_cell.material);
    //                 *out++ = run_length;

    //                 run_cell = cell;
    //                 run_length = 0;
    //             }
    //             else run_length++;
    //         }
    // if(run_length > 0)
    // {
    //     *((uint32*) out) = run_cell_data;
    //     out += sizeof(run_cell_data);
    //     *out++ = run_length;
    // }
    // unsigned long rle_size = out-start;

    // //TODO: compressed data might be larger than uncompressed in some cases
    // int compressed_size = LZ4_compress_default((char*) start, (char*) compressed_data, rle_size, raw_size/2);
    // return compressed_size;
}

#endif //ROOM
