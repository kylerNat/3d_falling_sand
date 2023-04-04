#ifndef BODY_DATA_BINDING
#define BODY_DATA_BINDING 0
#endif

struct body
{
    int texture_lower_x; int texture_lower_y; int texture_lower_z;
    int texture_upper_x; int texture_upper_y; int texture_upper_z;
    int origin_to_lower_x; int origin_to_lower_y; int origin_to_lower_z;
    float x_cm_x; float x_cm_y; float x_cm_z;
    float x_x; float x_y; float x_z;
    float x_dot_x; float x_dot_y; float x_dot_z;
    float orientation_r; float orientation_x; float orientation_y; float orientation_z;
    float omega_x; float omega_y; float omega_z;

    float old_x_x; float old_x_y; float old_x_z;
    float old_orientation_r; float old_orientation_x; float old_orientation_y; float old_orientation_z;

    float m;

    float I_xx; float I_yx; float I_zx;
    float I_xy; float I_yy; float I_zy;
    float I_xz; float I_yz; float I_zz;

    float invI_xx; float invI_yx; float invI_zx;
    float invI_xy; float invI_yy; float invI_zy;
    float invI_xz; float invI_yz; float invI_zz;

    int boxl_x; int boxl_y; int boxl_z;
    int boxu_x; int boxu_y; int boxu_z;

    int cell_material_id;
    int is_mutating;
    int substantial;

    int brain_id;
};

layout(std430, binding = BODY_DATA_BINDING) buffer body_data
{
    body bodies[];
};

ivec3 body_texture_lower(int b)
{
    return ivec3(bodies[b].texture_lower_x, bodies[b].texture_lower_y, bodies[b].texture_lower_z);
}

ivec3 body_texture_upper(int b)
{
    return ivec3(bodies[b].texture_upper_x, bodies[b].texture_upper_y, bodies[b].texture_upper_z);
}

ivec3 body_origin_to_lower(int b)
{
    return ivec3(bodies[b].origin_to_lower_x, bodies[b].origin_to_lower_y, bodies[b].origin_to_lower_z);
}

vec3 body_x_cm(int b)
{
    return vec3(bodies[b].x_cm_x, bodies[b].x_cm_y, bodies[b].x_cm_z);
}

vec3 body_x(int b)
{
    return vec3(bodies[b].x_x, bodies[b].x_y, bodies[b].x_z);
}

vec3 body_x_dot(int b)
{
    return vec3(bodies[b].x_dot_x, bodies[b].x_dot_y, bodies[b].x_dot_z);
}

vec4 body_orientation(int b)
{
    return vec4(bodies[b].orientation_r, bodies[b].orientation_x, bodies[b].orientation_y, bodies[b].orientation_z);
}

vec3 body_omega(int b)
{
    return vec3(bodies[b].omega_x, bodies[b].omega_y, bodies[b].omega_z);
}

vec3 body_old_x(int b)
{
    return vec3(bodies[b].old_x_x, bodies[b].old_x_y, bodies[b].old_x_z);
}

vec4 body_old_orientation(int b)
{
    return vec4(bodies[b].old_orientation_r, bodies[b].old_orientation_x, bodies[b].old_orientation_y, bodies[b].old_orientation_z);
}

float body_m(int b)
{
    return bodies[b].m;
}

mat3 body_I(int b)
{
    return mat3(bodies[b].I_xx, bodies[b].I_yx, bodies[b].I_zx,
                bodies[b].I_xy, bodies[b].I_yy, bodies[b].I_zy,
                bodies[b].I_xz, bodies[b].I_yz, bodies[b].I_zz);
}

mat3 body_invI(int b)
{
    return mat3(bodies[b].invI_xx, bodies[b].invI_yx, bodies[b].invI_zx,
                bodies[b].invI_xy, bodies[b].invI_yy, bodies[b].invI_zy,
                bodies[b].invI_xz, bodies[b].invI_yz, bodies[b].invI_zz);
}

vec3 body_boxl(int b)
{
    return vec3(bodies[b].boxl_x, bodies[b].boxl_y, bodies[b].boxl_z);
}

vec3 body_boxu(int b)
{
    return vec3(bodies[b].boxu_x, bodies[b].boxu_y, bodies[b].boxu_z);
}

int body_cell_material_id(int b)
{
    return bodies[b].cell_material_id;
}

int body_is_mutating(int b)
{
    return bodies[b].is_mutating;
}

int body_is_substantial(int b)
{
    return bodies[b].substantial;
}

int body_brain_id(int b)
{
    return bodies[b].brain_id;
}
