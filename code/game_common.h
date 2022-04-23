#ifndef GAME_COMMON
#define GAME_COMMON

struct rational
{
    int n;
    int d;
};

struct particle
{
    real_3 x;
    real_3 x_dot;
};

struct body
{
    uint16* materials;
    int_3 size;
    real_3 x_cm;

    GLuint materials_texture;
    int storage_level;

    real_3 x;
    real_3 x_dot;
    real_3 omega;
    quaternion orientation;
};

struct collision_point
{
    real_3 x;
    real_3 normal;
    real depth;
};

#endif //GAME_COMMON
