#define N_MAX_MATERIALS 256

struct material_visual_info
{
    //visual properties
    real_3 base_color;
    real_3 emission;
    real roughness;
    real metalicity;
    real opacity;

    real normal_type; //0 = voxel normals, 1 = display surface normals, intermediate values interpolate
    real _;
    real __;
};

struct material_physical_info
{
    //physical properties
    real density;
    real hardness;
    real sharpness;

    //thermal properties
    real melting_point;
    real boiling_point;
    real heat_capacity;

    //electrical properties
    real conductivity;

    real _;
    real __;

    //TODO: chance to disappear
};

material_visual_info material_visuals[N_MAX_MATERIALS] = {
    {}, //air
    {.base_color = {0.96, 0.84, 0.69}, .roughness = 1.0f, .metalicity = 0.0f}, //sand
    {.base_color = {0.2, 0.2, 0.8}, .emission = {0.5, 0.5, 1.0}, .roughness = 1.0f, .metalicity = 0.0f}, //water
    {.base_color = {1.0, 0.5, 0.5}, .roughness = 0.8f, .metalicity = 1.0f}, //coperish
    {.base_color = {0.8, 0.2, 0.8}, .roughness = 0.3f, .metalicity = 0.0f}, //purply
    {.base_color = {0.0, 0.0, 0.0}, .emission = {1,1,1}, .roughness = 1.0f, .metalicity = 0.0f}, //light
};

material_physical_info material_physicals[N_MAX_MATERIALS] = {
    {}, //air
    {.density = 0, .hardness = 0, .melting_point = 16, .boiling_point = 16}, //sand
    {.density = 0, .melting_point = 0, .boiling_point = 16}, //water
    {.density = 0, .hardness = 1, .melting_point = 16, .boiling_point = 16}, //coperish
    {.density = 0, .hardness = 1, .melting_point = 16, .boiling_point = 16}, //purply
    {.density = 0, .hardness = 1, .melting_point = 16, .boiling_point = 16}, //light
};
