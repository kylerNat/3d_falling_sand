#define         mat(vox) (vox.r)
#define       depth(vox) (int(vox.g&0x1F))
#define       phase(vox) ((vox.g>>5)&0x3)
#define   transient(vox) (vox.g>>7)
#define        temp(vox) (vox.b&0xF)
#define        trig(vox) (vox.b>>4)
#define    colorvar(vox) (vox.a&0x3F)
#define    electric(vox) (vox.a>>6)

#define MAX_DEPTH 32
#define SURF_DEPTH 16

#define phase_solid  0
#define phase_sand   1
#define phase_liquid 2
#define phase_gas    3

#define trig_none     0
#define trig_always   1
#define trig_hot      2
#define trig_cold     3
#define trig_electric 4
#define trig_contact  5

#define act_none      0
#define act_grow      1
#define act_die       2
#define act_heat      3
#define act_chill     4
#define act_electrify 5
#define act_explode   6
#define act_spray     7

#define BASE_CELL_MAT 128

#ifdef MATERIAL_PHYSICAL_PROPERTIES
float density(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(0,material_id), 0).r;
}

float hardness(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(1,material_id), 0).r;
}

float sharpness(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(2,material_id), 0).r;
}

float melting_point(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(3,material_id), 0).r;
}

float boiling_point(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(4,material_id), 0).r;
}

float heat_capacity(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(5,material_id), 0).r;
}

float conductivity(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(6,material_id), 0).r;
}

float n_triggers(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(7,material_id), 0).r;
}

uint trigger_info(uint material_id, uint trigger_id)
{
    return uint(texelFetch(material_physical_properties, ivec2(8+trigger_id,material_id), 0).r);
}

#endif

vec3 unnormalized_gradient(usampler3D materials, ivec3 p)
{
    // ivec2 d = ivec2(1,0);
    // vec3 gradient = vec3(
    //     depth(texelFetch(materials, p-d.xyy, 0))-depth(texelFetch(materials, p+d.xyy, 0)),
    //     depth(texelFetch(materials, p-d.yxy, 0))-depth(texelFetch(materials, p+d.yxy, 0)),
    //     depth(texelFetch(materials, p-d.yyx, 0))-depth(texelFetch(materials, p+d.yyx, 0))+0.001f
    //     );

    vec3 gradient = vec3(
        depth(texelFetch(materials, p+ivec3(-1,0,0), 0))-depth(texelFetch(materials, p+ivec3(+1,0,0), 0)),
        depth(texelFetch(materials, p+ivec3(0,-1,0), 0))-depth(texelFetch(materials, p+ivec3(0,+1,0), 0)),
        depth(texelFetch(materials, p+ivec3(0,0,-1), 0))-depth(texelFetch(materials, p+ivec3(0,0,+1), 0))+0.001);
    return gradient;
}
