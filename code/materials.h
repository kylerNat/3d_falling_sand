#ifndef MATERIALS
#define MATERIALS

#define N_MAX_MATERIALS 2048
//material_id's only go up to 256, but materials for cell type's can have different properties for each body

#define BASE_CELL_MAT 128
#define ROOM_TEMP 100

struct material_t
{
    char* name;
    uint32 id;

    union
    {
        struct
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

            //TODO: chance to disappear
        };
        real physical_properties[11];
    };

    union
    {
        struct
        {
            //visual properties
            real_3 base_color;
            real_3 emission;
            real roughness;
            real metalicity;
            real opacity;
            real refractive_index;
            real _;
            real __;
        };
        real visual_properties[12];
    };
};

GLuint material_visual_properties_texture;
GLuint material_physical_properties_texture;

material_t materials_list[N_MAX_MATERIALS];
int n_materials;

material_t base_material = {
    .name = "unknown",
    .id = 0,

    .density = 0.001,
    .hardness = 1,
    .friction = 10,
    .restitution = 0.5,

    .melting_point = 256,
    .boiling_point = 256,
    .heat_capacity = 1.0f,
    .thermal_conductivity = 0.05f,

    .base_color = {1, 1, 1},
    .roughness = 0.3f,
    .metalicity = 0.0f,
    .opacity = 1.0f
};

material_t base_cell_material = {
    .name = "unknown cell",
    .id = 0,

    .density = 0.001,
    .hardness = 1,
    .friction = 10,
    .restitution = 0.5,

    .melting_point = 256,
    .boiling_point = 256,
    .heat_capacity = 1.0f,
    .thermal_conductivity = 0.05f,

    .base_color = {1, 1, 1},
    .roughness = 0.3f,
    .metalicity = 0.0f,
    .opacity = 1.0f,
    .refractive_index = 1.0f,
};

uint32 str_to_id(char* str)
{
    union
    {
        uint32 id;
        char id_str[4];
    };
    memcpy(id_str, str, 4);
    return id;
}

void init_materials_list()
{
    n_materials = 0;

    {
        material_t* mat = materials_list+n_materials++;
        *mat = base_material;

        mat->name = "nothing";
        mat->id = str_to_id("NONE");

        //physical
        mat->density = 0.0;
        mat->hardness = 0;
        mat->friction = 0;
        mat->restitution = 0.5;

        mat->heat_capacity = 0.0;
        mat->thermal_conductivity = 0.01;

        //visual
        mat->base_color = {0,0,0};
        mat->roughness = 0.0f;
        mat->metalicity = 0.0f;
        mat->opacity = 0.0;
        mat->refractive_index = 1.0f;
    }

    {
        material_t* mat = materials_list+n_materials++;
        *mat = base_material;

        mat->name = "water";
        mat->id = str_to_id("AQUA");

        //physical
        mat->density = 0.001;
        mat->hardness = 1;
        mat->friction = 10;
        mat->restitution = 0.5;

        mat->melting_point =  -1;
        mat->boiling_point = 130;
        mat->heat_capacity = 10.0;
        mat->thermal_conductivity = 1.0;

        //visual
        mat->base_color = {0.2, 0.2, 0.8};
        mat->roughness = 1.0f;
        mat->metalicity = 0.0f;
        mat->opacity = 0.01;
        mat->refractive_index = 1.33;
    }

    {
        material_t* mat = materials_list+n_materials++;
        *mat = base_material;

        mat->name = "sand";
        mat->id = str_to_id("SAND");

        //physical
        mat->density = 0.001;
        mat->hardness = 0;
        mat->friction = 1;
        mat->restitution = 0.5;

        mat->melting_point = 256;
        mat->boiling_point = 256;
        mat->heat_capacity = 1.0;
        mat->thermal_conductivity = 0.05;

        //visual
        mat->base_color = {0.96, 0.84, 0.69};
        mat->roughness = 1.0f;
        mat->metalicity = 0.0f;
        mat->opacity = 1;
    }

    {
        material_t* mat = materials_list+n_materials++;
        *mat = base_material;

        mat->name = "wall";
        mat->id = str_to_id("WALL");

        //physical
        mat->density = 0.001;
        mat->hardness = 1;
        mat->friction = 1;
        mat->restitution = 0.5;

        mat->melting_point = 200;
        mat->boiling_point = 256;
        mat->heat_capacity = 1.0;
        mat->thermal_conductivity = 0.05;

        //visual
        mat->base_color = {1.0, 0.5, 0.5};
        mat->roughness = 0.8f;
        mat->metalicity = 1.0f;
        mat->opacity = 1;
    }

    {
        material_t* mat = materials_list+n_materials++;
        *mat = base_material;

        mat->name = "light";
        mat->id = str_to_id("LAMP");

        //physical
        mat->density = 0.001;
        mat->hardness = 1;
        mat->friction = 1;
        mat->restitution = 0.5;

        mat->melting_point = 256;
        mat->boiling_point = 256;
        mat->heat_capacity = 1.0;
        mat->thermal_conductivity = 0.05;

        //visual
        mat->base_color = {0,0,0};
        mat->emission = {1,1,1};
        mat->roughness = 1.0f;
        mat->metalicity = 0.0f;
        mat->opacity = 1;
    }

    {
        material_t* mat = materials_list+n_materials++;
        *mat = base_material;

        mat->name = "red light";
        mat->id = str_to_id("RLMP");

        //physical
        mat->density = 0.001;
        mat->hardness = 1;
        mat->friction = 1;
        mat->restitution = 0.5;

        mat->melting_point = 256;
        mat->boiling_point = 256;
        mat->heat_capacity = 1.0;
        mat->thermal_conductivity = 0.05;

        //visual
        mat->base_color = {0,0,0};
        mat->emission = {1,0,0};
        mat->roughness = 1.0f;
        mat->metalicity = 0.0f;
        mat->opacity = 1;
    }

    {
        material_t* mat = materials_list+n_materials++;
        *mat = base_material;

        mat->name = "green light";
        mat->id = str_to_id("GLMP");

        //physical
        mat->density = 0.001;
        mat->hardness = 1;
        mat->friction = 1;
        mat->restitution = 0.5;

        mat->melting_point = 256;
        mat->boiling_point = 256;
        mat->heat_capacity = 1.0;
        mat->thermal_conductivity = 0.05;

        //visual
        mat->base_color = {0,0,0};
        mat->emission = {0,1,0};
        mat->roughness = 1.0f;
        mat->metalicity = 0.0f;
        mat->opacity = 1;
    }

    {
        material_t* mat = materials_list+n_materials++;
        *mat = base_material;

        mat->name = "blue light";
        mat->id = str_to_id("BLMP");

        //physical
        mat->density = 0.001;
        mat->hardness = 1;
        mat->friction = 1;
        mat->restitution = 0.5;

        mat->melting_point = 256;
        mat->boiling_point = 256;
        mat->heat_capacity = 1.0;
        mat->thermal_conductivity = 0.05;

        //visual
        mat->base_color = {0,0,0};
        mat->emission = {0,0,1};
        mat->roughness = 1.0f;
        mat->metalicity = 0.0f;
        mat->opacity = 1;
    }

    {
        material_t* mat = materials_list+n_materials++;
        *mat = base_material;

        mat->name = "glass";
        mat->id = str_to_id("GLSS");

        //physical
        mat->density = 0.001;
        mat->hardness = 1;
        mat->friction = 1;
        mat->restitution = 0.5;

        mat->melting_point = 200;
        mat->boiling_point = 256;
        mat->heat_capacity = 1.0;
        mat->thermal_conductivity = 0.05;

        //visual
        mat->base_color = {0.2,0.5,0.2};
        mat->roughness = 1.0f;
        mat->metalicity = 0.0f;
        mat->opacity = 0.01f;
        mat->refractive_index = 1.5f;
    }
}

int get_material_index(char* material_id)
{
    int id = str_to_id(material_id);
    for(int i = 0; i < N_MAX_MATERIALS; i++)
        if(materials_list[i].id == id)
        {
            return i;
        }
    return -1;
}

#endif //MATERIALS
