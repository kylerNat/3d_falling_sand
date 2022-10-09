#ifndef FORM_DATA_BINDING
#define FORM_DATA_BINDING 0
#endif

struct form
{
    int materials_origin_x; int materials_origin_y; int materials_origin_z;
    int size_x; int size_y; int size_z;
    float x_cm_x; float x_cm_y; float x_cm_z;
    float x_x; float x_y; float x_z;
    float orientation_r; float orientation_x; float orientation_y; float orientation_z;

    int cell_material_id;
};

ivec3 form_materials_origin;
ivec3 form_size;
vec3  form_x_cm;
vec3  form_x;
vec4  form_orientation;

int form_cell_material_id;

layout(std430, binding = FORM_DATA_BINDING) buffer form_data
{
    form forms[];
};

void get_form_data(int f)
{
    form_materials_origin = ivec3(forms[f].materials_origin_x,
                                  forms[f].materials_origin_y,
                                  forms[f].materials_origin_z);
    form_size = ivec3(forms[f].size_x,
                      forms[f].size_y,
                      forms[f].size_z);

    form_x = vec3(forms[f].x_x, forms[f].x_y, forms[f].x_z);
    form_orientation = vec4(forms[f].orientation_r, forms[f].orientation_x, forms[f].orientation_y, forms[f].orientation_z);

    form_x_cm = vec3(forms[f].x_cm_x, forms[f].x_cm_y, forms[f].x_cm_z);

    form_cell_material_id = forms[f].cell_material_id;
}
