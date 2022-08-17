vec3 get_base_color(int material_id)
{
    return texelFetch(material_properties, ivec2(0,material_id), 0).rgb;
}

vec3 get_emission(int material_id)
{
    return texelFetch(material_properties, ivec2(1,material_id), 0).rgb;
}

float get_roughness(int material_id)
{
    return texelFetch(material_properties, ivec2(2,material_id), 0).r;
}

float get_metalicity(int material_id)
{
    return texelFetch(material_properties, ivec2(2,material_id), 0).g;
}

float D(int material_id, float nh)
{
    //Trowbridge-Reitz
    float roughness = get_roughness(material_id);
    float alphasq = pow(roughness, 4); //alpha = roughness^2
    return alphasq/(pi*sq(sq(nh)*(alphasq-1)+1));
}

vec3 F(int material_id, float lh)
{
    //Schlick approximation
    vec3 F0 = mix(vec3(1), get_base_color(material_id), get_metalicity(material_id));
    return F0-(vec3(1)-F0)*pow(1-lh, 5.0f);
}

//view factor, specular G/(4*dot(n, l)*dot(n,v))
float V(int material_id, float nh, float nl, float nv)
{
    float roughness = get_roughness(material_id);
    float k = sq(roughness+1)/8;
    return 1.0f/(4*(nv*(1-k)+k)*(nl*(1-k)+k));
}

//TODO: should also have transmittance
vec3 fr(int material_id, vec3 l, vec3 v, vec3 n)
{
    vec3 h = normalize(l+v);
    float lh = dot(l,h);
    float nh = dot(n,h);
    float nl = dot(n,l);
    float nv = dot(n,v);
    // return (D(material_id, nh)*V(material_id, lh, nl, nv))*F(material_id, lh);
    return mix(nl*get_base_color(material_id), (D(material_id, nh)*V(material_id, lh, nl, nv))*F(material_id, lh), get_metalicity(material_id));
}
