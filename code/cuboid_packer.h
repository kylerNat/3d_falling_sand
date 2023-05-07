#ifndef CUBOID_PACKER
#define CUBOID_PACKER

#include "bounding_box.h"

struct cuboid_space
{
    int_3 max_size;

    bounding_box* free_regions;
    int n_free_regions;
};

bounding_box add_region(cuboid_space* s, int_3 size)
{
    for(int r = 0; r < s->n_free_regions; r++)
    {
        bounding_box b = s->free_regions[r];
        int_3 region_size = b.u-b.l;
        if(size.x <= region_size.x && size.y <= region_size.y && size.z <= region_size.z)
        {
            s->free_regions[r] = s->free_regions[--s->n_free_regions];

            int_3 excess_size = region_size-size;
            int largest_axes[3] = {0};
            for(int i = 1; i < 3; i++)
            {
                if(excess_size[i] > excess_size[largest_axes[0]]) largest_axes[0] = i;
            }
            int i1 = (largest_axes[0]+1)%3;
            int i2 = (largest_axes[0]+2)%3;
            if(excess_size[i1] >= excess_size[i2])
            {
                largest_axes[1] = i1;
                largest_axes[2] = i2;
            }
            else
            {
                largest_axes[1] = i2;
                largest_axes[2] = i1;
            }

            int_3 upper = b.u;
            for(int i = 0; i < 3; i++)
            {
                if(excess_size[largest_axes[i]] > 0)
                {
                    bounding_box new_box = {b.l, upper};
                    new_box.l[largest_axes[i]] += size[largest_axes[i]];
                    upper[largest_axes[i]] -= excess_size[largest_axes[i]];
                    // log_output("adding free box: ", new_box.l, ", ", new_box.u, "\n");
                    s->free_regions[s->n_free_regions++] = new_box;
                }
                else break;
            }

            bounding_box out = {b.l, b.l+size};
            return out;
        }
    }

    log_output("no regions found for box of size ", size, "\n");
    for(int r = 0; r < s->n_free_regions; r++)
    {
        bounding_box b = s->free_regions[r];
        log_output(b.l, ", ", b.u, "\n");
    }
    return {{0,0,0},{0,0,0}};
}

void free_region(cuboid_space* s, bounding_box b)
{
    bool merged = false;
    do
    {
        merged = false;
        for(int r = 0; r < s->n_free_regions; r++)
        {
            bounding_box region = s->free_regions[r];
            bool ll[3];
            bool uu[3];
            bool lu[3];
            bool ul[3];
            for(int i = 0; i < 3; i++)
            {
                ll[i] = region.l[i] == b.l[i];
                uu[i] = region.u[i] == b.u[i];
                lu[i] = region.l[i] == b.u[i];
                ul[i] = region.u[i] == b.l[i];
            }

            for(int i = 0; i < 3; i++)
            {
                int i1 = (i+1)%3;
                int i2 = (i+2)%3;
                if(ll[i1] && uu[i1] && ll[i2] && uu[i2] && (lu[i] || ul[i]))
                {
                    if(lu[i]) b.u[i] = region.u[i];
                    else      b.l[i] = region.l[i];

                    s->free_regions[r] = s->free_regions[--s->n_free_regions];
                    merged = true;
                    // log_output("merge\n");
                    break;
                }
            }
            if(merged) break;
        }
    }
    while(merged);

    s->free_regions[s->n_free_regions++] = b;
}

//this would ruin references to things so it's better to just let whatever code is using this to do it manually
// //just resets everything and re-adds occupied_regions
// void defrag_cuboid_space(cuboid_space* s)
// {
//     s->free_regions[0] = s->max_size;
//     s->n_free_regions = 1;

//     int n_occupied_regions = s->n_occupied_regions;
//     s->n_occupied_regions = 0;
//     for(int r = 0; r < n_occupied_regions; r++)
//     {
//         add_region(s, s->occupied_regions[r].size);
//     }
// }

#endif //CUBOID_PACKER
