#ifndef COLLISION_GRID_BINDING
#define COLLISION_GRID_BINDING 0
#endif

struct collision_cell
{
    int first_index;
    int n_bodies;
};

#define collision_cells_per_axis 16
#define collision_cell_size 32

layout(std430, binding = COLLISION_GRID_BINDING) buffer collision_grid_data
{
    collision_cell collision_grid[collision_cells_per_axis*collision_cells_per_axis*collision_cells_per_axis];
    int n_collision_indices;
    int collision_indices[];
};
