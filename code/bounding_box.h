#ifndef BOUNDING_BOX
#define BOUNDING_BOX

struct bounding_box
{
    int_3 l;
    int_3 u;
};

bool operator==(bounding_box a, bounding_box b)
{
    return (a.l==b.l && a.u==b.u);
}

bool is_inside(int_3 p, bounding_box b)
{
    return (b.l.x <= p.x && p.x < b.u.x &&
            b.l.y <= p.y && p.y < b.u.y &&
            b.l.z <= p.z && p.z < b.u.z);
}

bool is_intersecting(bounding_box a, bounding_box b)
{
    //working with coordinates doubled to avoid rounding errors from integer division
    int_3 a_center = (a.l+a.u);
    int_3 a_diag = (a.u-a.l);
    int_3 b_center = (b.l+b.u);
    int_3 b_diag = (b.u+b.l);
    int_3 closest_a_point_to_b = a_center+multiply_components(a_diag, sign_per_axis(b_center-a_center));
    int_3 closest_b_point_to_a = b_center+multiply_components(b_diag, sign_per_axis(b_center-a_center));
    return is_inside(closest_a_point_to_b, {2*b.l, 2*b.u}) || is_inside(closest_b_point_to_a, {2*a.l, 2*a.u});
}

bounding_box expand_to(bounding_box b, int_3 p)
{
    b.l = min_per_axis(b.l, p);
    b.u = max_per_axis(b.u, p+(int_3){1,1,1});
    return b;
}

int_3 dim(bounding_box b)
{
    return b.u-b.l;
}

#endif //BOUNDING_BOX
