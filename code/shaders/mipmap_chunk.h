#program simulate_chunk_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"

layout(location = 0) in vec2 r;
layout(location = 1) in vec2 X;

layout(location = 0) uniform int layer;
layout(location = 3) uniform usampler3D active_regions_in;
layout(location = 5) uniform writeonly uimage3D occupied_regions_out;

out vec3 p;

void main()
{
    int z = layer/16;
    int y = int(X.y);
    int x = int(X.x);

    p=vec3(16*x,16*y,layer);

    gl_Position = vec4(-2,-2,0,1);

    float scale = 2.0f/32.0f;

    uint region_active = texelFetch(active_regions_in, ivec3(x, y, z), 0).r;
    if(region_active != 0)
    {
        if(gl_VertexID == 0 && layer%16 == 0) imageStore(occupied_regions_out, ivec3(x,y,z), uvec4(0,0,0,0));
        gl_Position.xy = -1.0f+scale*(r+X);
    }
}

/////////////////////////////////////////////////////////////////

#shader GL_FRAGMENT_SHADER
#include "include/header.glsl"

layout(location = 0) out ivec4 frag_color;

layout(location = 0) uniform int layer;
layout(location = 1) uniform int frame_number;
layout(location = 2) uniform isampler3D materials;
layout(location = 4) uniform writeonly uimage3D active_regions_out;
layout(location = 5) uniform writeonly uimage3D occupied_regions_out;
layout(location = 6) uniform int update_cells;

in vec3 p;

uint rand(uint seed)
{
    seed ^= seed<<13;
    seed ^= seed>>17;
    seed ^= seed<<5;
    return seed;
}

float float_noise(uint seed)
{
    return fract(float(int(seed))/1.0e10);
}

const int chunk_size = 256;

void main()
{
    float scale = 1.0/chunk_size;
    // ivec2 pos = ivec2(chunk_size*uv);
    // ivec2 pos = ivec2(gl_FragCoord.xy);
    ivec3 pos = ivec3(p);
    ivec3 cell_p;
    cell_p.xy = ivec2(gl_FragCoord.xy)%16;
    cell_p.z = pos.z%16;
    pos.xy += cell_p.xy;

    //+,0,-,0
    //0,+,0,-
    // int rot = (frame_number+layer)%4;
    uint rot = rand(rand(rand(frame_number)))%4;
    int i = 0;
    ivec2 dir = ivec2(((rot&1)*(2-rot)), (1-(rot&1))*(1-rot));

    ivec4 c  = texelFetch(materials, ivec3(pos.x, pos.y, pos.z),0);
    ivec4 u  = texelFetch(materials, ivec3(pos.x, pos.y, pos.z+1),0);
    ivec4 d  = texelFetch(materials, ivec3(pos.x, pos.y, pos.z-1),0);
    ivec4 r  = texelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z),0);
    ivec4 l  = texelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z),0);
    ivec4 dr = texelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z-1),0);
    ivec4 ur = texelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z+1),0);

    ivec4 f  = texelFetch(materials, ivec3(pos.x-dir.y, pos.y+dir.x, pos.z),0);
    ivec4 b  = texelFetch(materials, ivec3(pos.x+dir.y, pos.y-dir.x, pos.z),0);

    ivec4 ul = texelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z+1),0);
    ivec4 ll = texelFetch(materials, ivec3(pos.x-2*dir.x, pos.y-2*dir.y, pos.z),0);
    ivec4 dl = texelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z-1),0);

    frag_color = c;
    if(update_cells == 1)
    {
        if(c.r == 0)
        {
            if(u.r > 0 && u.r != 3) frag_color = u;
            else if(l.r > 0 && ul.r > 0 && u.r != 3 && ul.r != 3) frag_color = ul;
            else if(dl.r > 0 && d.r > 0 && l.r > 1 && ll.r > 0 && u.r != 3 && ul.r != 3 && l.r != 3) frag_color = l;

            // if(u.r == 0 && ul.r == 0 && dl.r > 0 && d.r > 0 && l.r > 0) frag_color = l;
            // else if(u.r == 0 && l.r > 0) frag_color = ul;
            // else if(u.r > 0) frag_color = u;
        }
        else if(c.r != 3)
        {
            bool fall_allowed = (pos.z > 0 && (d.r == 0 || (dr.r == 0 && r.r == 0)));
            bool flow_allowed = (pos.z > 0 && r.r == 0 && ur.r == 0 && u.r == 0 && c.r > 1 && l.r > 0);
            if(fall_allowed || flow_allowed) frag_color.r = 0;
        }
    }

    const int max_depth = 16;
    if(c.r > 0)
    {
        if(l.r == 0 ||
           r.r == 0 ||
           u.r == 0 ||
           d.r == 0 ||
           f.r == 0 ||
           b.r == 0) frag_color.g = 0;
        else
        {
            frag_color.g = -max_depth;
            frag_color.g = max(frag_color.g, l.g-1);
            frag_color.g = max(frag_color.g, r.g-1);
            frag_color.g = max(frag_color.g, u.g-1);
            frag_color.g = max(frag_color.g, d.g-1);
            frag_color.g = max(frag_color.g, f.g-1);
            frag_color.g = max(frag_color.g, b.g-1);
        }

        frag_color.b = c.b/2;
        if(frag_color.r == 3) frag_color.b = 1000;
        // if(frag_color.r == 1) frag_color.b = 100;
        // frag_color.b = 0;
    }
    else
    {
        frag_color.g = max_depth;
        frag_color.g = min(frag_color.g, l.g+1);
        frag_color.g = min(frag_color.g, r.g+1);
        frag_color.g = min(frag_color.g, u.g+1);
        frag_color.g = min(frag_color.g, d.g+1);
        frag_color.g = min(frag_color.g, f.g+1);
        frag_color.g = min(frag_color.g, b.g+1);

        // // frag_color.b = 1+texelFetch(materials, ivec3(pos.x, pos.y, layer), 0).b;
        // float brightness = 100.0;
        // brightness += brightness_curve(l.b);
        // brightness += brightness_curve(r.b);
        // brightness += brightness_curve(u.b);
        // brightness += brightness_curve(d.b);
        // brightness += brightness_curve(f.b);
        // brightness += brightness_curve(b.b);
        // brightness *= 1.0/6.0;
        // frag_color.b = int(inverse_brightness_curve(brightness));
    }
    frag_color.g = clamp(frag_color.g,-max_depth,max_depth);

    // bool changed = c.r != frag_color.r || c.g != frag_color.g || c.b != frag_color.b;
    bool changed = c.r != frag_color.r || c.g != frag_color.g;
    if(changed)
    {
        imageStore(active_regions_out, ivec3(pos.x/16, pos.y/16, pos.z/16), uvec4(1,0,0,0));
        if(cell_p.x==15) imageStore(active_regions_out, ivec3(pos.x/16+1, pos.y/16, pos.z/16), uvec4(1,0,0,0));
        if(cell_p.x== 0) imageStore(active_regions_out, ivec3(pos.x/16-1, pos.y/16, pos.z/16), uvec4(1,0,0,0));
        if(cell_p.y==15) imageStore(active_regions_out, ivec3(pos.x/16, pos.y/16+1, pos.z/16), uvec4(1,0,0,0));
        if(cell_p.y== 0) imageStore(active_regions_out, ivec3(pos.x/16, pos.y/16-1, pos.z/16), uvec4(1,0,0,0));
        if(cell_p.z==15) imageStore(active_regions_out, ivec3(pos.x/16, pos.y/16, pos.z/16+1), uvec4(1,0,0,0));
        if(cell_p.z== 0) imageStore(active_regions_out, ivec3(pos.x/16, pos.y/16, pos.z/16-1), uvec4(1,0,0,0));
    }

    if(c.r > 0) imageStore(occupied_regions_out, ivec3(pos.x/16, pos.y/16, pos.z/16), uvec4(1,0,0,0));
}
