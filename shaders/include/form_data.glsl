#ifndef FORM_DATA_BINDING
#define FORM_DATA_BINDING 0
#endif

struct form
{
    int lower_x; int lower_y; int lower_z;
    int upper_x; int upper_y; int upper_z;

    int origin_to_lower_x; int origin_to_lower_y; int origin_to_lower_z;

    float x_x; float x_y; float x_z;
    float orientation_r; float orientation_x; float orientation_y; float orientation_z;

    int cell_material_id;
};

ivec3 form_materials_origin;
ivec3 form_lower;
ivec3 form_upper;
ivec3 form_size;
vec3  form_x;
vec4  form_orientation;

int form_cell_material_id;

layout(std430, binding = FORM_DATA_BINDING) buffer form_data
{
    form forms[];
};

void get_form_data(int f)
{
    ivec3 form_lower = ivec3(forms[f].lower_x,
                             forms[f].lower_y,
                             forms[f].lower_z);
    ivec3 form_upper = ivec3(forms[f].upper_x,
                             forms[f].upper_y,
                             forms[f].upper_z);
    ivec3 form_size = form_upper-form_lower;

    form_x = vec3(forms[f].x_x, forms[f].x_y, forms[f].x_z);
    form_orientation = vec4(forms[f].orientation_r, forms[f].orientation_x, forms[f].orientation_y, forms[f].orientation_z);

    form_cell_material_id = forms[f].cell_material_id;
}
