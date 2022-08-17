#define N_MAX_MATERIALS 256

struct material_info
{
    real_3 base_color;
    real_3 emission;
    real roughness;
    real metalicity;
    real _;
    //opacity, absorbtion, normal type,
};

material_info materials[N_MAX_MATERIALS] = {
    {}, //air
    {.base_color = {0.96, 0.84, 0.69}, .roughness = 1.0f, .metalicity = 0.0f}, //sand
    {.base_color = {0.2, 0.2, 0.8}, .emission = {30, 30, 30}, .roughness = 1.0f, .metalicity = 0.0f}, //water
    {.base_color = {1.0, 0.5, 0.5}, .roughness = 0.8f, .metalicity = 1.0f}, //
    {.base_color = {0.8, 0.2, 0.8}, .roughness = 0.3f, .metalicity = 0.0f}, //
};
