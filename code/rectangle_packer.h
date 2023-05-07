#ifndef RECTANGLE_PACKER
#define RECTANGLE_PACKER

struct bounding_box_2d
{
    int_2 l;
    int_2 u;
};

bool operator==(bounding_box_2d a, bounding_box_2d b)
{
    return (a.l==b.l && a.u==b.u);
}

bool is_inside(int_2 p, bounding_box_2d b)
{
    return (b.l.x <= p.x && p.x < b.u.x &&
            b.l.y <= p.y && p.y < b.u.y);
}

bool is_intersecting(bounding_box_2d a, bounding_box_2d b)
{
    //working with coordinates doubled to avoid rounding errors from integer division
    int_2 a_center = (a.l+a.u);
    int_2 a_diag = (a.u-a.l);
    int_2 b_center = (b.l+b.u);
    int_2 b_diag = (b.u+b.l);
    int_2 closest_a_point_to_b = a_center+multiply_components(a_diag, sign_per_axis(b_center-a_center));
    int_2 closest_b_point_to_a = b_center+multiply_components(b_diag, sign_per_axis(b_center-a_center));
    return is_inside(closest_a_point_to_b, {2*b.l, 2*b.u}) || is_inside(closest_b_point_to_a, {2*a.l, 2*a.u});
}

bounding_box_2d expand_to(bounding_box_2d b, int_2 p)
{
    b.l = min_per_axis(b.l, p);
    b.u = max_per_axis(b.u, p+(int_2){1,1});
    return b;
}

struct rectangle_space
{
    int_2 max_size;

    bounding_box_2d* free_regions;
    int n_free_regions;
};

bounding_box_2d add_region(rectangle_space* s, int_2 size)
{
    for(int r = 0; r < s->n_free_regions; r++)
    {
        bounding_box_2d b = s->free_regions[r];
        int_2 region_size = b.u-b.l;
        if(size.x <= region_size.x && size.y <= region_size.y)
        {
            s->free_regions[r] = s->free_regions[--s->n_free_regions];

            int_2 excess_size = region_size-size;
            int largest_axes[2] = {0};
            for(int i = 1; i < 2; i++)
            {
                if(excess_size[i] > excess_size[largest_axes[0]]) largest_axes[0] = i;
            }
            int i1 = (largest_axes[0]+1)%2;

            int_2 upper = b.u;
            for(int i = 0; i < 2; i++)
            {
                if(excess_size[largest_axes[i]] > 0)
                {
                    bounding_box_2d new_box = {b.l, upper};
                    new_box.l[largest_axes[i]] += size[largest_axes[i]];
                    upper[largest_axes[i]] -= excess_size[largest_axes[i]];
                    // log_output("adding free box: ", new_box.l, ", ", new_box.u, "\n");
                    s->free_regions[s->n_free_regions++] = new_box;
                }
                else break;
            }

            bounding_box_2d out = {b.l, b.l+size};
            return out;
        }
    }

    log_output("no regions found for box of size ", size, "\n");
    for(int r = 0; r < s->n_free_regions; r++)
    {
        bounding_box_2d b = s->free_regions[r];
        log_output(b.l, ", ", b.u, "\n");
    }
    return {{0,0},{0,0}};
}

void free_region(rectangle_space* s, bounding_box_2d b)
{
    bool merged = false;
    do
    {
        merged = false;
        for(int r = 0; r < s->n_free_regions; r++)
        {
            bounding_box_2d region = s->free_regions[r];
            bool ll[2];
            bool uu[2];
            bool lu[2];
            bool ul[2];
            for(int i = 0; i < 2; i++)
            {
                ll[i] = region.l[i] == b.l[i];
                uu[i] = region.u[i] == b.u[i];
                lu[i] = region.l[i] == b.u[i];
                ul[i] = region.u[i] == b.l[i];
            }

            for(int i = 0; i < 2; i++)
            {
                int i1 = (i+1)%2;
                int i2 = (i+2)%2;
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

#endif //RECTANGLE_PACKER
