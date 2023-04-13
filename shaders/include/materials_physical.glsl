#ifdef UINT_PACKED_MATERIALS
#define         mat(vox) (vox&0xFF)
#define       depth(vox) (int((vox>>8)&0x1F))
#define       phase(vox) ((vox>>13)&0x3)
#define   transient(vox) ((vox>>15)&0x1)
#define        temp(vox) ((vox>>16)&0xFF)
#define        volt(vox) ((vox>>24)&0xF)
#define        trig(vox) ((vox>>30))
#define        flow(vox) ((vox>>30))
#else
#define         mat(vox) (vox.r)
#define       depth(vox) (int(vox.g&0x1F))
#define       phase(vox) ((vox.g>>5)&0x3) //in world
#define   transient(vox) (vox.g>>7) //in world
#define    selected(vox) (vox.a) //in editor
#define        temp(vox) (vox.b)
#define        volt(vox) (vox.a&0xF)
#define        flow(vox) (vox.a>>4) //in world
#endif

#define signed_depth(vox) ((mat(vox) == 0) ? (1+depth(vox)) : (-depth(vox)))
#define opaque_signed_depth(vox) (opacity(mat(vox)) == 1 ? 1+depth(vox) : -depth(vox))

#define solidity(vox) ((mat(vox)==0?0:1|(phase(vox)==phase_solid||phase(vox)==phase_sand ? 2:0))*(1-transient(vox)))

#define MAX_DEPTH 16

#define phase_gas    0
#define phase_solid  1
#define phase_sand   2
#define phase_liquid 3

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

#define room_temp 100

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

float friction(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(3,material_id), 0).r;
}

float restitution(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(4,material_id), 0).r;
}

float melting_point(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(5,material_id), 0).r;
}

float boiling_point(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(6,material_id), 0).r;
}

float heat_capacity(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(7,material_id), 0).r;
}

float thermal_conductivity(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(8,material_id), 0).r+0.001;
}

float conductivity(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(9,material_id), 0).r;
}

float work_function(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(10,material_id), 0).r;
}

int growth_time(uint material_id)
{
    return int(texelFetch(material_physical_properties, ivec2(11,material_id), 0).r);
}

float n_triggers(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(12,material_id), 0).r;
}

uint trigger_info(uint material_id, uint trigger_id)
{
    return uint(texelFetch(material_physical_properties, ivec2(13+trigger_id,material_id), 0).r);
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
        signed_depth(texelFetch(materials, p+ivec3(+1,0,0), 0))-signed_depth(texelFetch(materials, p+ivec3(-1,0,0), 0)),
        signed_depth(texelFetch(materials, p+ivec3(0,+1,0), 0))-signed_depth(texelFetch(materials, p+ivec3(0,-1,0), 0)),
        signed_depth(texelFetch(materials, p+ivec3(0,0,+1), 0))-signed_depth(texelFetch(materials, p+ivec3(0,0,-1), 0))+0.001);
    return gradient;
}
