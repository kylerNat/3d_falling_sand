#ifndef MATERIALS
#define MATERIALS

#define N_MAX_MATERIALS 2048
//material_id's only go up to 256, but materials for cell type's can have different properties for different materials

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

    //trigger info
    real n_triggers;
    real trigger_info[15]; //3 uint8's packed into a 24 bit uint then converted to a float

    //TODO: chance to disappear
};
#define N_PHYSICAL_PROPERTIES (sizeof(material_physical_info)/sizeof(float))

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

material_visual_info base_cell_visual = {.base_color = {1, 1, 1}, .roughness = 0.3f, .metalicity = 0.0f};
material_physical_info base_cell_physical = {.density = 0, .hardness = 1, .melting_point = 16, .boiling_point = 16};

GLuint material_visual_properties_texture;
GLuint material_physical_properties_texture;

#endif //MATERIALS
