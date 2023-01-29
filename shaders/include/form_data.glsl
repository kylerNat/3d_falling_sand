#ifndef FORM_DATA_BINDING
#define FORM_DATA_BINDING 0
#endif

struct form
{
    int materials_origin_x; int materials_origin_y; int materials_origin_z;
    int lower_x; int lower_y; int lower_z;
    int upper_x; int upper_y; int upper_z;
    int x_origin_x; int x_origin_y; int x_origin_z;
    float x_x; float x_y; float x_z;
    float orientation_r; float orientation_x; float orientation_y; float orientation_z;

    int cell_material_id;
    int is_mutating;
};

ivec3 form_materials_origin;
ivec3 form_lower;
ivec3 form_upper;
ivec3 form_size;
ivec3 form_x_origin;
vec3  form_x;
vec4  form_orientation;

int form_cell_material_id;
int form_is_mutating;

layout(std430, binding = FORM_DATA_BINDING) buffer form_data
{
    form forms[];
};

void get_form_data(int f)
{
    form_materials_origin = ivec3(forms[f].materials_origin_x,
                                  forms[f].materials_origin_y,
                                  forms[f].materials_origin_z);
    ivec3 form_lower = ivec3(forms[f].lower_x,
                             forms[f].lower_y,
                             forms[f].lower_z);
    ivec3 form_upper = ivec3(forms[f].upper_x,
                             forms[f].upper_y,
                             forms[f].upper_z);
    ivec3 form_size = form_upper-form_lower;

    form_x = vec3(forms[f].x_x, forms[f].x_y, forms[f].x_z);
    form_orientation = vec4(forms[f].orientation_r, forms[f].orientation_x, forms[f].orientation_y, forms[f].orientation_z);

    form_x_origin = ivec3(forms[f].x_origin_x, forms[f].x_origin_y, forms[f].x_origin_z);

    form_cell_material_id = forms[f].cell_material_id;
    form_is_mutating = forms[f].is_mutating;
}
