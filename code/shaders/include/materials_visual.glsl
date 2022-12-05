vec3 get_base_color(uint material_id)
{
    return texelFetch(material_visual_properties, ivec2(0,material_id), 0).rgb;
}

vec3 get_emission(uint material_id)
{
    return texelFetch(material_visual_properties, ivec2(1,material_id), 0).rgb;
}

float get_roughness(uint material_id)
{
    return texelFetch(material_visual_properties, ivec2(2,material_id), 0).r;
}

float get_metalicity(uint material_id)
{
    return texelFetch(material_visual_properties, ivec2(2,material_id), 0).g;
}

float opacity(uint material_id)
{
    return texelFetch(material_visual_properties, ivec2(2,material_id), 0).b;
}

float refractive_index(uint material_id)
{
    return texelFetch(material_visual_properties, ivec2(3,material_id), 0).r;
}

float D(uint material_id, float nh)
{
    //Trowbridge-Reitz
    float roughness = get_roughness(material_id);
    float alphasq = pow(roughness, 4); //alpha = roughness^2
    return alphasq/(pi*sq(sq(nh)*(alphasq-1)+1));
}

vec3 F(uint material_id, float lh)
{
    //Schlick approximation
    vec3 F0 = mix(vec3(1), get_base_color(material_id), get_metalicity(material_id));
    // return F0-(vec3(1)-F0)*pow(1-lh, 5.0f);
    float b = 1-lh;
    return F0-(vec3(1)-F0)*sq(sq(b))*b;
}

//view factor, specular G/(4*dot(n, l)*dot(n,v))
float V(uint material_id, float nh, float nl, float nv)
{
    float roughness = get_roughness(material_id);
    float k = sq(roughness+1)/8;
    return 1.0f/(4*(nv*(1-k)+k)*(nl*(1-k)+k));
    // return 1.0f/4.0f;
}

//TODO: should also have transmittance
vec3 fr(uint material_id, vec3 l, vec3 v, vec3 n)
{
    vec3 h = normalize(l+v);
    float lh = dot(l,h);
    float nh = dot(n,h);
    float nl = dot(n,l);
    float nv = dot(n,v);
    // return (D(material_id, nh)*V(material_id, lh, nl, nv))*F(material_id, lh);
    vec3 diffuse = ((1.0f/pi)*nl)*get_base_color(material_id);
    vec3 specular = (nl*D(material_id, nh)*V(material_id, lh, nl, nv))*F(material_id, lh);
    float metalicity = get_metalicity(material_id);
    return diffuse*(1-metalicity) + specular*mix(0.02, 1.0, metalicity);
}
