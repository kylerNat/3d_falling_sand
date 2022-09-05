vec4 blue_noise(vec2 co)
{
    return fract(texture(blue_noise_texture, co)+(frame_number%100)*vec4(1.0/PHI4,1.0/(PHI4*PHI4),1.0/(PHI4*PHI4*PHI4),1.0/(PHI4*PHI4*PHI4*PHI4)));
}

vec4 static_blue_noise(vec2 co)
{
    return texture(blue_noise_texture, co);
}

vec4 quasi_noise(int i)
{
    float g = 1.0/PHI3;
    return fract(0.5+i*vec4(g, sq(g), g*sq(g), sq(sq(g))));
    // return vec4(fract(0.5 + vec2(i*12664745, i*9560333)/exp2(24.)),0,0);
}
