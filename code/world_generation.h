#ifndef WORLD_GENERATION
#define WORLD_GENERATION

#include "grid_tile.h"

void init_grid_tiles(tile_manager* tim)
{
    uint32 seed = 128998197;

    for(int n = 0; n < sq(tile_grid_size); n++)
    {
        int id = n+1;
        add_entry(tim->tile_table, id, {});
        tile_info* info = &tim->tile_table[id];
        info->id = id;
        info->storage_level = ts_ungenerated;

        int x = n%tile_grid_size;
        int y = n/tile_grid_size;
        info->neighbors[0] = 1+((x-1+tile_grid_size)%tile_grid_size)+tile_grid_size*y;
        info->neighbors[1] = 1+x+tile_grid_size*((y-1+tile_grid_size)%tile_grid_size);
        info->neighbors[2] = 1+((x+1)%tile_grid_size)+tile_grid_size*y;
        info->neighbors[3] = 1+x+tile_grid_size*((y+1)%tile_grid_size);

        // info->grid_pos = {x-tile_grid_size/2,y-tile_grid_size/2};

        info->n_max_layers = 0;
        info->layer = 0;
        info->n_layers = 0;

        info->biome_params = {randf(&seed)};
        info->perlin_gradient = sign_per_axis(rand_normal_2(&seed));
    }

    for(int i = 1; i < n_biome_parameters; i++)
    {
        for(int m = 0; m < sq(tile_grid_size); m++)
        {
            int id = m+1;
            tile_info* info = &tim->tile_table[id];
            info->biome_params[i] = 0.5*info->biome_params[i-1];
            for(int n = 0; n < 4; n++)
            {
                tile_info* neighbor = &tim->tile_table[info->neighbors[n]];
                info->biome_params[i] += 0.125*neighbor->biome_params[i-1];
            }
        }
    }

    // int n_iterations = 1000;
    // for(int i = 0; i < n_iterations; i++)
    // {
    //     for(int n = 0; n < sq(tile_grid_size); n++)
    //     {
    //         int id = n+1;
    //         tile_info* info = &tim->tile_table[id];
    //         biome_parameters laplacian = -1*info->biome_params;
    //         for(int n = 0; n < 4; n++)
    //         {
    //             tile_info* neighbor = &tim->tile_table[info->neighbors[n]];
    //             laplacian += 0.25*neighbor->biome_params;
    //         }
    //         biome_parameters D = {1.0, 0.5};

    //         real u = info->biome_params[0];
    //         real v = info->biome_params[1];

    //         for(int j = 0; j < n_biome_parameters; j++)
    //         {
    //             info->biome_params[j] += D[j]*laplacian[j];
    //         }
    //         real f = 0.0567;
    //         real k = 0.0649;
    //         biome_parameters R = {
    //             -u*v*v+f*(1-u),
    //             +u*v*v-(k+f)*v,
    //         };
    //         info->biome_params += R;
    //     }
    // }

    // for(int n = 0; n < sq(tile_grid_size); n++)
    // {
    //     int id = n+1;
    //     tile_info* info = &tim->tile_table[id];
    //     log_output(info->biome_params[0], ", ", info->biome_params[1], "\n");
    // }
}

void generate_tile(tile_manager* tim, tile_info* info)
{
    assert(info->storage_level = ts_ungenerated);
    int n_lists_needed = 1;
    info->layer = get_layer_list_memory(tim, n_lists_needed);
    info->storage_level = ts_memory;
    info->n_max_layers = n_lists_needed*layer_list_len;
    info->n_layers = 0;
    {
        tile_layer* layer = &info->layer[info->n_layers++];
        layer->element = ele_dirt;
        layer->storage_level = ls_memory;
        layer->tile = get_tile_memory(tim);
        memset(layer->tile, 0, sizeof(grid_tile));

        uint32 seed = 38173*info->id;

        real_2 neighbor_gradient[4];
        real_2 next_neighbor_gradient[4*4];

        real neighbor_heights[4];
        int wavelength = n_biome_parameters-2;
        int wavelength_2 = n_biome_parameters-1;
        real next_neighbor_height[4*4];
        for(int n = 0; n < 4; n++)
        {
            tile_info* neighbor = &tim->tile_table[info->neighbors[n]];
            neighbor_heights[n] = neighbor->biome_params[wavelength];
            neighbor_gradient[n] = neighbor->perlin_gradient;
            for(int nn = 0; nn < 4; nn++)
            {
                tile_info* next_neighbor = &tim->tile_table[neighbor->neighbors[nn]];
                next_neighbor_height[4*n+nn] = next_neighbor->biome_params[wavelength];
                next_neighbor_gradient[4*n+nn] = next_neighbor->perlin_gradient;
            }
        }
        real center_height = info->biome_params[wavelength];
        real_2 center_gradient = info->perlin_gradient;

        for(int y = 0; y < grid_tile_size; y++)
            for(int x = 0; x < grid_tile_size; x++)
            {
                #if 0
                int x_neighbor_index = 0+2*(x >= grid_tile_size/2);
                int y_neighbor_index = 1+2*(y >= grid_tile_size/2);
                real u = abs(((real) x-grid_tile_size/2)/(grid_tile_size));
                real v = abs(((real) y-grid_tile_size/2)/(grid_tile_size));
                u = 3*u*u-2*u*u*u;
                v = 3*v*v-2*v*v*v;
                real edge_height = lerp(neighbor_heights[y_neighbor_index],
                                        0.5*(next_neighbor_height[4*y_neighbor_index+x_neighbor_index]
                                             +next_neighbor_height[y_neighbor_index+4*x_neighbor_index]),
                                        u);
                real middle_height = lerp(center_height,
                                          neighbor_heights[x_neighbor_index],
                                          u);
                real height = lerp(middle_height, edge_height, v);
                // height = lerp(info->biome_params[wavelength_2], height,
                //               pow(max(abs((real) x-grid_tile_size/2),
                //                       abs((real) y-grid_tile_size/2))/(grid_tile_size/2),
                //                   1.0));
                #else //perlin noise
                int x_neighbor_index = 2;
                int y_neighbor_index = 3;
                real u = (((real) x)/(grid_tile_size));
                real v = (((real) y)/(grid_tile_size));
                real_2 uv = {u, v};
                // u = abs(u);
                // v = abs(v);
                u = 3*u*u-2*u*u*u;
                v = 3*v*v-2*v*v*v;
                // u = 1;
                // v = 0;

                real edge_height = lerp(dot(neighbor_gradient[y_neighbor_index], uv-(real_2){0,1}),
                                        dot(0.5*(next_neighbor_gradient[4*y_neighbor_index+x_neighbor_index]
                                                 +next_neighbor_gradient[y_neighbor_index+4*x_neighbor_index]),
                                            uv-(real_2){1,1}),
                                        u);
                real middle_height = lerp(dot(center_gradient, uv),
                                          dot(neighbor_gradient[x_neighbor_index], uv-(real_2){1,0}),
                                          u);
                real height = lerp(middle_height, edge_height, v);
                height = clamp(1.0f+0.1f*height, 0.0, 10.0);
                #endif

                //clamping for steppy terrain
                // real step_size = 0.01;
                // height = 0.5*height+0.5+floor(height/step_size)*step_size;

                top(x,y,layer->tile) = 100+((int) 1000*height);
                top(x,y,layer->tile) += 0.5*rand_normal(&seed);
            }
    }
    if(info->id == 8+8*16+1)
    {
        tile_layer* layer = &info->layer[info->n_layers++];
        layer->element = ele_water;
        layer->storage_level = ls_memory;
        layer->tile = get_tile_memory(tim);
        memset(layer->tile, 0, sizeof(grid_tile));

        uint32 seed = 38173*info->id;
        for(int y = 0; y < grid_tile_size; y++)
            for(int x = 0; x < grid_tile_size; x++)
            {
                bottom(x,y,layer->tile) = 100+1*rand_normal(&seed);//cos(x)*cos(y);
                top(x,y,layer->tile) = bottom(x,y,layer->tile)+10+1*rand_normal(&seed);
            }
    }
}

// {
//     w.memory_tiles[w.n_memory_tiles] = id;
//     w.tiles_last_used[w.n_memory_tiles] = 0;
//     w.n_memory_tiles++;

//     for(int l = n_elements-1; l >= 0; l--)
//     {
//         if(l == ele_water || l == ele_dirt)
//         {
//             assert(n_used_grid_tiles < w.max_grid_tiles, "n_used_grid_tiles is ", n_used_grid_tiles, "which exceeds the limit of ", w.max_grid_tiles);
//             info->storage_level[l] = sm_memory;
//             info->data[l].tile = &w.grid_tiles[n_used_grid_tiles];
//             info->nonzero[l] = true;
//             w.grid_tile_usage[n_used_grid_tiles] = 1;
//             n_used_grid_tiles++;
//             assert(n_used_grid_tiles < w.max_grid_tiles);
//         }
//         else
//         {
//             info->storage_level[l] = sm_compressed;
//             info->data[l].compressed = 0;
//             info->data[l].compressed_size = 0;
//             info->nonzero[l] = false;
//             continue;
//         }

//         if(l == ele_dirt)
//         {
//             for(int x = 0; x < grid_tile_size; x++)
//                 for(int y = 0; y < grid_tile_size; y++)
//                 {
//                     info->data[l].tile->tops[x+grid_tile_size*y] = 200+(randui(&w.seed)%10)-5;
//                     info->data[l].tile->bottoms[x+grid_tile_size*y] = 10;
//                 }
//             for(int m = 0; m < len(mounds); m++)
//             {
//                 int mound_x = (mounds[m].x-grid_tile_size*info->grid_pos.x);
//                 int mound_y = (mounds[m].y-grid_tile_size*info->grid_pos.y);
//                 for(int x = 0; x < grid_tile_size; x++)
//                     for(int y = 0; y < grid_tile_size; y++)
//                     {
//                         int rsq = (sq(x-mound_x)+sq(y-mound_y));
//                         int height = 100*exp(-0.005*rsq);
//                         if(info->data[l].tile->tops[x+grid_tile_size*y]  > height) info->data[l].tile->tops[x+grid_tile_size*y] -= height;
//                         else info->data[l].tile->tops[x+grid_tile_size*y] = 0;
//                     }
//             }
//         }
//         if(l == ele_water)
//         {
//             for(int x = 0; x < grid_tile_size; x++)
//                 for(int y = 0; y < grid_tile_size; y++)
//                 {
//                     int global_x = (x+grid_tile_size*info->grid_pos.x);
//                     int global_y = (y+grid_tile_size*info->grid_pos.y);
//                     int r_sq = sq(global_x)+sq(global_y);
//                     if(r_sq > sq(100)) continue;
//                     int dirt_floor = info->data[ele_dirt].tile->tops[x+grid_tile_size*y];
//                     info->data[l].tile->bottoms[x+grid_tile_size*y] = dirt_floor;
//                     info->data[l].tile->tops[x+grid_tile_size*y] = max(100+100*exp(-r_sq/100), dirt_floor);
//                 }
//         }
//     }
// }

#endif //WORLD_GENERATION
