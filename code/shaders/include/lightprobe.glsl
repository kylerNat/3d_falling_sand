#include "include/lightprobe_header.glsl"

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

vec3 sample_lightprobe_color(vec3 pos, vec3 normal, vec2 sample_oct, out vec2 depth)
{
    pos = pos+8*normal; //bias away from surfaces
    vec4 total_color = vec4(0);
    vec4 total_color_no_cheb = vec4(0);
    depth = vec2(0);
    for(int pz = 0; pz <= 1; pz++)
        for(int py = 0; py <= 1; py++)
            for(int px = 0; px <= 1; px++)
            {
                vec3 p = vec3(px, py, pz);
                ivec3 ip = ivec3(px, py, pz);
                ivec3 probe_pos = ivec3((pos-lightprobe_spacing/2)/lightprobe_spacing)+ip;
                if(any(lessThan(probe_pos, vec3(0))) || any(greaterThanEqual(probe_pos, vec3(lightprobes_per_axis)))) continue;
                int probe_index = int(dot(probe_pos, ivec3(1,lightprobes_per_axis,lightprobes_per_axis*lightprobes_per_axis)));
                ivec2 probe_coord = ivec2(probe_index%lightprobes_w, probe_index/lightprobes_w);

                // ivec2 sample_coord = lightprobe_resolution*probe_coord+ivec2(lightprobe_resolution*clamp(0.5f*sample_oct+0.5f,0,1));
                vec2 sample_coord = vec2(lightprobe_padded_resolution*probe_coord+1)+lightprobe_resolution*clamp(0.5f*sample_oct+0.5f,0,1);

                sample_coord += 0.5;
                vec2 t = trunc(sample_coord);
                vec2 f = fract(sample_coord);
                f = f*f*f*(f*(f*6.0-15.0)+10.0);
                // f = f*f*(-2*f+3);
                sample_coord = t+f-0.5;

                sample_coord *= vec2(1.0f/lightprobe_resolution_x, 1.0f/lightprobe_resolution_y);

                vec3 probe_x = texelFetch(lightprobe_x, probe_coord, 0).xyz;

                vec4 probe_color = texture(lightprobe_color, sample_coord);

                vec3 dist = probe_x-pos;
                float r = max(length(dist), 0.0001f);
                vec3 dir = dist*(1.0f/r);

                vec2 depth_sample_coord = vec2(lightprobe_padded_resolution*probe_coord+1)+lightprobe_resolution*clamp(0.5f*vec_to_oct(-dir)+0.5f,0,1);
                depth_sample_coord *= vec2(1.0f/lightprobe_resolution_x, 1.0f/lightprobe_resolution_y);
                vec2 probe_depth = texture(lightprobe_depth, depth_sample_coord).rg;

                #ifdef DEBUG_DOTS
                if(dot(dir, normal) > 0.999) return vec3(1,0,0);
                #endif

                float weight = sq(0.5*dot(normal, dir)+0.5)+0.2;
                // float weight = 1.0;

                vec3 base_dist = lightprobe_spacing*(vec3(probe_pos)+0.5)-pos; //distance from base probe position
                vec3 trilinear_weights = clamp(1.0f+(1-2*p)*(1.0f/lightprobe_spacing)*base_dist, 0, 1);
                weight *= trilinear_weights.x*trilinear_weights.y*trilinear_weights.z;

                total_color_no_cheb += weight*sqrt(probe_color);

                // if(r > probe_depth.r)
                // {
                //     float variance = abs(probe_depth.g-(probe_depth.r*probe_depth.r));
                //     weight *= variance/(variance+sq(r-probe_depth.r));
                // }

                // I think this makes more sense
                float variance = abs(probe_depth.g-sq(probe_depth.r));
                float x = (probe_depth.r-r)*inversesqrt(variance);
                weight *= cdf(x);

                weight = clamp(weight, 0, 1);

                //this smoothly kills low values
                const float threshold = 0.02;
                if(weight < threshold)
                    weight *= sq(weight)/sq(threshold);

                total_color += weight*sqrt(probe_color);
                depth += weight*(probe_depth);
            }
    // total_color.rgb = mix(sq(total_color_no_cheb.rgb*(1.0f/total_color_no_cheb.a)),
    //                       sq(total_color.rgb*(1.0f/total_color.a)), min(total_color.a, 1));
    total_color.rgb = sq(total_color.rgb*(1.0f/total_color.a));

    depth *= (1.0f/total_color.a);
    return total_color.rgb;
}
