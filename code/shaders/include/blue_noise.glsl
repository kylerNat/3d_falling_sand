vec4 blue_noise(vec2 co)
{
    return fract(texture(blue_noise_texture, co)+(frame_number%100)*vec4(1.0/PHI4,1.0/(PHI4*PHI4),1.0/(PHI4*PHI4*PHI4),1.0/(PHI4*PHI4*PHI4*PHI4)));
}

vec3 blue_noise_3(vec2 co)
{
    return fract(texture(blue_noise_texture, co).xyz+(frame_number%100)*vec3(1.0/PHI3,1.0/(PHI3*PHI3),1.0/(PHI3*PHI3*PHI3)));
}

vec2 blue_noise_2(vec2 co)
{
    return fract(texture(blue_noise_texture, co).xy+(frame_number%200)*vec2(1.0/PHI2,1.0/(PHI2*PHI2)));
}

vec4 static_blue_noise(vec2 co)
{
    return texture(blue_noise_texture, co);
}

// vec3 quasinoise_3(int i)
// {
//     float g = 1.0/PHI3;
//     return fract(0.5+i*vec3(g, sq(g), g*sq(g)));
//     // return vec4(fract(0.5 + vec2(i*12664745, i*9560333)/exp2(24.)),0,0);
// }

vec2 quasinoise_2(int i)
{
    float g = 1.0/PHI2;
    return fract(0.5+(i%1000)*vec2(g, sq(g)));
    // return vec4(fract(0.5 + vec2(i*12664745, i*9560333)/exp2(24.)),0,0);
}

vec2 halton(int i)
{
    vec2 s = vec2(i);
    vec2 base = vec2(2,3);
    vec4 a = vec4(1,1,0,0);
    while(all(greaterThan(s, vec2(0))))
    {
        a.xy = a.xy/base;
        a.zw += a.xy*mod(vec2(s), base);
        s = floor(s/base);
    }
    return a.zw;
}
