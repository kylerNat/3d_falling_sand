vec3 sign_not_zero(vec3 p) {
    return 2*step(0, p)-1;
}

vec2 vec_to_oct(vec3 p)
{
    vec3 sign_p = sign_not_zero(p);
    vec2 oct = p.xy * (1.0f/dot(p, sign_p));
    return (p.z > 0) ? oct : (1.0f-abs(oct.yx))*sign_p.xy;
}

vec3 oct_to_vec(vec2 oct)
{
    vec2 sign_oct = sign(oct);
    vec3 p = vec3(oct.xy, 1.0-dot(oct, sign_oct));
    if(p.z < 0) p.xy = (1.0f-abs(p.yx))*sign_oct.xy;
    return normalize(p);
}

float cdf(float x)
{
    return 0.5f*tanh(0.797884560803f*(x+0.044715f*x*x*x))+0.5f;
}

vec3 sample_lightprobe_color(vec3 pos, vec3 normal, vec2 sample_oct)
{
    vec4 total_color = vec4(0);
    for(int pz = 0; pz <= 1; pz++)
        for(int py = 0; py <= 1; py++)
            for(int px = 0; px <= 1; px++)
            {
                vec3 p = vec3(px, py, pz);
                ivec3 ip = ivec3(px, py, pz);
                ivec3 probe_pos = clamp(ivec3(pos/16)+ip, 0, 32);
                int probe_index = int(dot(probe_pos, ivec3(1,32,32*32)));
                ivec2 probe_coord = ivec2(probe_index%256, probe_index/256);
                ivec2 color_sample_coord = 16*probe_coord+ivec2(16*sample_oct);
                ivec2 depth_sample_coord = 16*probe_coord+ivec2(16*sample_oct);

                vec3 probe_x = texelFetch(lightprobe_x, probe_coord, 0).xyz;

                vec4 probe_color = texelFetch(lightprobe_color, color_sample_coord, 0);
                vec2 probe_depth = texelFetch(lightprobe_depth, depth_sample_coord, 0).rg;

                vec3 dist = probe_x-pos;
                float r = length(dist);
                vec3 dir = dist*(1.0f/r);

                float weight = (dot(normal, dir)+1.0f)*0.5f;

                vec3 trilinear_weights = (1-2*p)*(1-p-dist/16.0f);
                weight *= trilinear_weights.x*trilinear_weights.y*trilinear_weights.z;

                if(r > probe_depth.r)
                {
                    float variance = probe_depth.g-sq(probe_depth.r);
                    weight *= variance/(variance+sq(r-probe_depth.r));
                }

                // // I think this makes more sense
                // real x = sqrt((probe_depth.r-r)/variance);
                // weight *= cdf(-x);

                total_color += weight*sqrt(probe_color);
            }
    // return total_color/total_weight;
    return sq(total_color.rgb*(1.0f/total_color.a));
}
