#ifndef BODY_DATA_BINDING
#define BODY_DATA_BINDING 0
#endif

struct body
{
    int materials_origin_x; int materials_origin_y; int materials_origin_z;
    int size_x; int size_y; int size_z;
    float x_cm_x; float x_cm_y; float x_cm_z;
    float x_x; float x_y; float x_z;
    float x_dot_x; float x_dot_y; float x_dot_z;
    float orientation_r; float orientation_x; float orientation_y; float orientation_z;
    float omega_x; float omega_y; float omega_z;

    float m;

    float I_xx; float I_yx; float I_zx;
    float I_xy; float I_yy; float I_zy;
    float I_xz; float I_yz; float I_zz;

    float invI_xx; float invI_yx; float invI_zx;
    float invI_xy; float invI_yy; float invI_zy;
    float invI_xz; float invI_yz; float invI_zz;

    int cell_material_id;
};

ivec3 body_materials_origin;
ivec3 body_size;
vec3  body_x_cm;
vec3  body_x;
vec3  body_x_dot;
vec4  body_orientation;
vec3  body_omega;

float body_m;
mat3 body_I;
mat3 body_invI;

int body_cell_material_id;

layout(std430, binding = BODY_DATA_BINDING) buffer body_data
{
    body bodies[];
};

void get_body_data(int b)
{
    body_materials_origin = ivec3(bodies[b].materials_origin_x,
                                  bodies[b].materials_origin_y,
                                  bodies[b].materials_origin_z);
    body_size = ivec3(bodies[b].size_x,
                      bodies[b].size_y,
                      bodies[b].size_z);

    body_x_cm = vec3(bodies[b].x_cm_x, bodies[b].x_cm_y, bodies[b].x_cm_z);
    body_x = vec3(bodies[b].x_x, bodies[b].x_y, bodies[b].x_z);
    body_x_dot = vec3(bodies[b].x_dot_x, bodies[b].x_dot_y, bodies[b].x_dot_z);
    body_orientation = vec4(bodies[b].orientation_r, bodies[b].orientation_x, bodies[b].orientation_y, bodies[b].orientation_z);
    body_omega = vec3(bodies[b].omega_x, bodies[b].omega_y, bodies[b].omega_z);

    body_m = bodies[b].m;
    body_I = mat3(bodies[b].I_xx, bodies[b].I_yx, bodies[b].I_zx,
                  bodies[b].I_xy, bodies[b].I_yy, bodies[b].I_zy,
                  bodies[b].I_xz, bodies[b].I_yz, bodies[b].I_zz);
    body_invI = mat3(bodies[b].invI_xx, bodies[b].invI_yx, bodies[b].invI_zx,
                     bodies[b].invI_xy, bodies[b].invI_yy, bodies[b].invI_zy,
                     bodies[b].invI_xz, bodies[b].invI_yz, bodies[b].invI_zz);

    body_cell_material_id = bodies[b].cell_material_id;
}
