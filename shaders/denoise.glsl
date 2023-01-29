#program denoise_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"

layout(location = 0) in vec3 x;

smooth out vec2 screen_pos;

void main()
{
    gl_Position.xyz = x;
    gl_Position.w = 1.0;
    screen_pos = x.xy;
}

/////////////////////////////////////////////////////////////////

#shader GL_FRAGMENT_SHADER
#include "include/header.glsl"

layout(location = 0) out vec4 frag_color;

layout(location = 0) uniform sampler2D color;
layout(location = 1) uniform sampler2D depth;
layout(location = 2) uniform sampler2D normal;

smooth in vec2 screen_pos;

void main()
{

    ivec2 d = ivec2(1,0);
    vec2 p = 0.5f*screen_pos+0.5f;
    int lod = 0;
    ivec2 ip = ivec2(pow(0.5f, lod)*gl_FragCoord.xy);
    // float laplacian =
    //     texelFetch(depth, ip-d.yx, lod).r+texelFetch(depth, ip+d.yx, lod).r
    //     +texelFetch(depth, ip-d.xy, lod).r+texelFetch(depth, ip+d.xy, lod).r
    //     -4.0f*texelFetch(depth, ip, lod).r;
    // frag_color.rgb = vec3(100*laplacian.r);
    // frag_color.rgb = textureLod(color, p, clamp(4.0f-1000.0f*laplacian, 0, 8)).rgb;

    float center_depth = texelFetch(depth, ip, 0).r;
    vec3 center_normal = texelFetch(normal, ip, 0).rgb;
    float weight = 0.0f;
    frag_color.rgb = vec3(0);
    int radius = 3;
    for(int dx = -radius; dx <= radius; dx++)
        for(int dy = -radius; dy <= radius; dy++)
            // if(abs(1.0-texelFetch(depth, ip+ivec2(dx,dy), 0).r/center_depth) < 0.01)
            if(length(texelFetch(normal, ip+ivec2(dx,dy), 0).rgb-center_normal) < 0.01)
            {
                // float w = exp(-0.5*(sq(dx)+sq(dy)));
                float w = 1.0;
                frag_color.rgb += w*texelFetch(color, ip+ivec2(dx,dy), 0).rgb;
                weight += w;
            }

    frag_color.rgb *= 1.0f/weight;
    frag_color.a = 1.0f;
}
