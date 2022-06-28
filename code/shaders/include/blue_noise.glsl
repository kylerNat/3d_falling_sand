vec4 blue_noise(vec2 co)
{
    return fract(texture(blue_noise_texture, co)+PHI*frame_number);
}
