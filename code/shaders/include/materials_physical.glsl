#define         mat(vox) (vox.r)
#define       depth(vox) (int(vox.g&0x1F))
#define       phase(vox) (vox.g>>6)
#define   transient(vox) (vox.b&0x3)
#define        temp(vox) ((vox.b>>2)&0xF)
#define    electric(vox) (vox.b>>6)
#define    colorvar(vox) (vox.a)

#define MAX_DEPTH 32
#define SURF_DEPTH 16

#define phase_solid  0
#define phase_sand   1
#define phase_liquid 2
#define phase_gas    3

#ifdef MATERIAL_PHYSICAL_PROPERTIES

float density(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(0,material_id), 0).r;
}

float hardness(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(0,material_id), 0).g;
}

float sharpness(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(0,material_id), 0).b;
}

float melting_point(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(1,material_id), 0).r;
}

float boiling_point(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(1,material_id), 0).g;
}

float heat_capacity(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(1,material_id), 0).b;
}

float conductivity(uint material_id)
{
    return texelFetch(material_physical_properties, ivec2(2,material_id), 0).r;
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
