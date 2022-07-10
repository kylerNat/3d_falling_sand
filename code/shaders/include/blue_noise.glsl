vec4 blue_noise(vec2 co)
{
    return fract(texture(blue_noise_texture, co)+(frame_number%100)*vec4(1.0/PHI4,1.0/(PHI4*PHI4),1.0/(PHI4*PHI4*PHI4),1.0/(PHI4*PHI4*PHI4*PHI4)));
}
