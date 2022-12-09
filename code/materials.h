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
    real refractive_index;

    real normal_type; //0 = voxel normals, 1 = display surface normals, intermediate values interpolate
    real __;
};

struct material_physical_info
{
    //physical properties
    real density;
    real hardness;
    real sharpness;

    real friction;
    real restitution;

    //thermal properties
    real melting_point;
    real boiling_point;
    real heat_capacity;
    real thermal_conductivity;

    //electrical properties
    real conductivity;
    real work_function;

    //trigger info
    real growth_time;
    real n_triggers;
    real trigger_info[15]; //3 uint8's packed into a 24 bit uint then converted to a float

    //TODO: chance to disappear
};
#define N_PHYSICAL_PROPERTIES (sizeof(material_physical_info)/sizeof(float))

material_visual_info material_visuals[N_MAX_MATERIALS] = {
    {.opacity = 0, .refractive_index = 1}, //air
    {.base_color = {0.96, 0.84, 0.69}, .roughness = 1.0f, .metalicity = 0.0f, .opacity = 1}, //sand
    {.base_color = {0.2, 0.2, 0.8}, /*.emission = {0.05, 0.05, 0.1},*/ .roughness = 1.0f, .metalicity = 0.0f, .opacity = 0.01, .refractive_index=1.33}, //water
    {.base_color = {1.0, 0.5, 0.5}, .roughness = 0.8f, .metalicity = 1.0f, .opacity = 1}, //coperish
    {.base_color = {0.8, 0.2, 0.8}, .roughness = 0.3f, .metalicity = 0.0f, .opacity = 1}, //purply
    {.base_color = {0.0, 0.0, 0.0}, .emission = {1,1,1}, .roughness = 1.0f, .metalicity = 0.0f, .opacity = 1}, //light
    {.base_color = {0.5, 0.9, 0.5}, .roughness = 1.0f, .metalicity = 0.0f, .opacity = 0.5}, //gas
    {.base_color = {0.2, 0.5, 0.2}, .roughness = 1.0f, .metalicity = 0.0f, .opacity = 0.01, .refractive_index = 1.5}, //glass
};

material_physical_info material_physicals[N_MAX_MATERIALS] = {
    {.heat_capacity = 0.0, .thermal_conductivity = 0.01}, //air
    {.density = 0.001, .hardness = 0, .friction = 1, .restitution = 0.5,
     .melting_point = 256, .boiling_point = 256, .heat_capacity = 1.0, .thermal_conductivity = 0.05}, //sand
    {.density = 0.001, .hardness = 1, .friction = 10, .restitution = 0.5,
     .melting_point =  -1, .boiling_point = 130, .heat_capacity = 10.0, .thermal_conductivity = 1.0}, //water
    {.density = 0.001, .hardness = 1, .friction = 1, .restitution = 0.5,
     .melting_point = 200, .boiling_point = 256, .heat_capacity = 10.0, .thermal_conductivity = 0.1,
     .conductivity = 10.0, }, //coperish
    {.density = 0.001, .hardness = 1, .friction = 1, .restitution = 0.5,
     .melting_point = 254, .boiling_point = 256, .heat_capacity=100.0, .thermal_conductivity = 1.0,
     .conductivity = 10.0, }, //purply
    {.density = 0.001, .hardness = 1, .friction = 1, .restitution = 0.5,
     .melting_point = 256, .boiling_point = 256, .heat_capacity = 1.0, .thermal_conductivity = 0.1}, //light
    {.density = 0.001, .hardness = 1, .friction = 1, .restitution = 0.5,
     .melting_point = -1, .boiling_point = -1, .heat_capacity = 10.0, .thermal_conductivity = 1.0}, //gas
    {.density = 0.001, .hardness = 1, .friction = 1, .restitution = 0.5,
     .melting_point = 200, .boiling_point = 256, .heat_capacity = 10.0, .thermal_conductivity = 1.0}, //glass
};

material_visual_info base_cell_visual = {.base_color = {1, 1, 1}, .roughness = 0.3f, .metalicity = 0.0f, .opacity = 1.0f};
material_physical_info base_cell_physical = {.density = 0.001, .hardness = 1, .friction = 10, .restitution = 0.5, .melting_point = 200, .boiling_point = 250, .growth_time = 1};

GLuint material_visual_properties_texture;
GLuint material_physical_properties_texture;

#endif //MATERIALS
