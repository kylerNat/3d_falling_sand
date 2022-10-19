#program simulate_chunk_program

/////////////////////////////////////////////////////////////////

#shader GL_VERTEX_SHADER
#include "include/header.glsl"

layout(location = 0) in vec2 r;
layout(location = 1) in vec2 X;

layout(location = 0) uniform int layer;
layout(location = 4) uniform usampler3D active_regions_in;
layout(location = 6) uniform writeonly uimage3D occupied_regions_out;

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

layout(location = 0) out uvec4 out_voxel;

layout(location = 0) uniform int layer;
layout(location = 1) uniform int frame_number;
layout(location = 2) uniform sampler2D material_physical_properties;
layout(location = 3) uniform usampler3D materials;
layout(location = 5) uniform writeonly uimage3D active_regions_out;
layout(location = 6) uniform writeonly uimage3D occupied_regions_out;
layout(location = 7) uniform int update_cells;

#define MATERIAL_PHYSICAL_PROPERTIES
#include "include/materials_physical.glsl"

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

    uvec4 c  = texelFetch(materials, ivec3(pos.x, pos.y, pos.z),0);

    //+,0,-,0
    //0,+,0,-
    // int rot = (frame_number+layer)%4;
    // uint rot = rand(rand(rand(frame_number)))%4;
    uint flow = flow(c);
    int i = 0;
    ivec2 dir = ivec2(((flow&1)*(2-flow)), (1-(flow&1))*(1-flow)); //(0,1), (1,0), (0,-1), (-1,0)

    uvec4 u  = texelFetch(materials, ivec3(pos.x, pos.y, pos.z+1),0);
    uvec4 d  = texelFetch(materials, ivec3(pos.x, pos.y, pos.z-1),0);
    uvec4 r  = texelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z),0);
    uvec4 l  = texelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z),0);
    uvec4 dr = texelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z-1),0);
    uvec4 ur = texelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z+1),0);

    uvec4 f  = texelFetch(materials, ivec3(pos.x-dir.y, pos.y+dir.x, pos.z),0);
    uvec4 b  = texelFetch(materials, ivec3(pos.x+dir.y, pos.y-dir.x, pos.z),0);

    uvec4 ul = texelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z+1),0);
    uvec4 ll = texelFetch(materials, ivec3(pos.x-2*dir.x, pos.y-2*dir.y, pos.z),0);
    uvec4 dl = texelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z-1),0);

    if(update_cells == 1)
    {
        if(mat(c) == 0)
        { //central cell is empty

            //check if upper cell fell           , liquids and sands
            //then check if upper left cell fell , liquids and sands
            //then check if left cell flowed     , liquids
            if(mat(u) != 0 && phase(u) != phase_solid && transient(u)==0)
                c = u;
            else if(mat(l) != 0 && mat(ul) != 0 && mat(u) == 0 && phase(ul) != phase_solid && flow(ul) == flow && transient(ul)==0)
                c = ul;
            else if(mat(dl) != 0 && mat(d) != 0 && (mat(ul) == 0 || phase(ul) < phase_sand || flow(ul) != flow) && phase(l) == phase_liquid && flow(l) == flow && transient(l)==0)
                c = l;
            else
            {
                // flow = rand(rand(rand(frame_number+pos.x+pos.y+pos.z)))%4;
                flow = (frame_number)%4;

                for(int f = 0; f < 4; f++)
                {
                    uint fl = (f+flow)%4;
                    ivec2 test_dir = ivec2(((fl&1)*(2-fl)), (1-(fl&1))*(1-fl));
                    uvec4 l = texelFetch(materials, ivec3(pos.x-test_dir.x, pos.y-test_dir.y, pos.z),0);
                    if(mat(l) != 0 && flow(l) == fl && phase(l) >= phase_sand)
                    {
                        flow = fl;
                        break;
                    }
                }

                for(int f = 0; f < 4; f++)
                {
                    uint fl = (f+flow)%4;
                    ivec2 test_dir = ivec2(((fl&1)*(2-fl)), (1-(fl&1))*(1-fl));
                    uvec4 ul = texelFetch(materials, ivec3(pos.x-test_dir.x, pos.y-test_dir.y, pos.z-1),0);
                    if(mat(ul) != 0 && flow(ul) == fl && phase(ul) >= phase_liquid)
                    {
                        flow = fl;
                        break;
                    }
                }
            }
            //might be better to multiply things by 0 instead of branching
        }
        else if(phase(c) != phase_solid && transient(c)==0)
        { //if the cell is not empty and not solid check if it fell or flew
            bool fall_allowed = (pos.z > 0 && (mat(d) == 0 || (mat(dr) == 0 && mat(r) == 0 && flow(dr) == flow)));
            bool flow_allowed = (pos.z > 0 && mat(r) == 0
                                 && mat(dr) != 0 && mat(d) != 0
                                 && (mat(ur) == 0 || phase(ur) < phase_sand || flow(ur) != flow)
                                 && (mat(u) == 0 || phase(u) < phase_sand)
                                 && phase(c) > phase_sand && flow(r) == flow);
            if(fall_allowed || flow_allowed) c = uvec4(0);
            else
            {
                flow = rand(rand(rand(frame_number)))%4;
                for(int f = 0; f < 4; f++)
                {
                    uint fl = (f+flow)%4;
                    ivec2 test_dir = ivec2(((fl&1)*(2-fl)), (1-(fl&1))*(1-fl));
                    uvec4 r = texelFetch(materials, ivec3(pos.x+test_dir.x, pos.y+test_dir.y, pos.z),0);
                    if(mat(r) == 0)
                    {
                        flow = fl;
                        break;
                    }
                }

                for(int f = 0; f < 4; f++)
                {
                    uint fl = (f+flow)%4;
                    ivec2 test_dir = ivec2(((fl&1)*(2-fl)), (1-(fl&1))*(1-fl));
                    uvec4 dr = texelFetch(materials, ivec3(pos.x+test_dir.x, pos.y+test_dir.y, pos.z-1),0);
                    if(mat(dr) == 0)
                    {
                        flow = fl;
                        break;
                    }
                }
            }
        }
    }

    int depth = SURF_DEPTH;
    if(mat(c) != 0 && transient(c)==0)
    {
        if(mat(r) == 0 || transient(r) != 0 ||
           mat(l) == 0 || transient(l) != 0 ||
           mat(u) == 0 || transient(u) != 0 ||
           mat(d) == 0 || transient(d) != 0 ||
           mat(f) == 0 || transient(f) != 0 ||
           mat(b) == 0 || transient(b) != 0) depth = SURF_DEPTH;
        else
        {
            depth = MAX_DEPTH-1;
            depth = min(depth, depth(r)+1);
            depth = min(depth, depth(l)+1);
            depth = min(depth, depth(u)+1);
            depth = min(depth, depth(d)+1);
            depth = min(depth, depth(f)+1);
            depth = min(depth, depth(b)+1);
        }
    }
    else
    {
        depth = 0;
        depth = max(depth, depth(r)-1);
        depth = max(depth, depth(l)-1);
        depth = max(depth, depth(u)-1);
        depth = max(depth, depth(d)-1);
        depth = max(depth, depth(f)-1);
        depth = max(depth, depth(b)-1);
    }

    uint volt = volt(c);

    // // float capacitance = 0;
    // vec2 avg_volt = vec2(0);
    // float r_c = 1.0/conductivity(mat(c));
    // if(mat(r)!=0 && volt(r)>0 && conductivity(mat(r))>0){float sigma=1.0/(r_c+1.0/conductivity(mat(r)));avg_volt+=vec2(sigma*float(volt(r)),sigma);}
    // if(mat(l)!=0 && volt(l)>0 && conductivity(mat(l))>0){float sigma=1.0/(r_c+1.0/conductivity(mat(l)));avg_volt+=vec2(sigma*float(volt(l)),sigma);}
    // if(mat(u)!=0 && volt(u)>0 && conductivity(mat(u))>0){float sigma=1.0/(r_c+1.0/conductivity(mat(u)));avg_volt+=vec2(sigma*float(volt(u)),sigma);}
    // if(mat(d)!=0 && volt(d)>0 && conductivity(mat(d))>0){float sigma=1.0/(r_c+1.0/conductivity(mat(d)));avg_volt+=vec2(sigma*float(volt(d)),sigma);}
    // if(mat(f)!=0 && volt(f)>0 && conductivity(mat(f))>0){float sigma=1.0/(r_c+1.0/conductivity(mat(f)));avg_volt+=vec2(sigma*float(volt(f)),sigma);}
    // if(mat(b)!=0 && volt(b)>0 && conductivity(mat(b))>0){float sigma=1.0/(r_c+1.0/conductivity(mat(b)));avg_volt+=vec2(sigma*float(volt(b)),sigma);}
    // if(avg_volt.y > 0)
    // {
    //     avg_volt.x = avg_volt.x/avg_volt.y;

    //     float dvolt = avg_volt.x-float(volt)-1;
    //     float dvolt_i = round(dvolt);
    //     float dvolt_f = dvolt-dvolt_i;
    //     if((rand(frame_number+rand(rand(pos.z))))%32 < int(32.0*abs(dvolt_f))) dvolt_i += sign(dvolt_f);
    //     volt = uint(clamp(float(volt)+dvolt_i, 0.0, 255.0));
    // }
    // // if(volt > 1) volt = volt-1;
    // if(conductivity(mat(c)) == 0)
    // {
    //     volt = 0;
    // }
    if(conductivity(mat(c)) > 0)
    {
        int dvolt = 0;
        int volti = int(volt);
        int nvolti = int(volt);
        int neighbors = 0;
        uint current_dir = rand(frame_number+rand(rand(rand(pos.z)+pos.y)+pos.x)+10)%6;
        if(mat(r)!=0 && conductivity(mat(r))>0){if(volt(r)>0)neighbors++; if(current_dir==0)nvolti = max(nvolti, int(volt(r)));}
        if(mat(l)!=0 && conductivity(mat(l))>0){if(volt(l)>0)neighbors++; if(current_dir==1)nvolti = max(nvolti, int(volt(l)));}
        if(mat(u)!=0 && conductivity(mat(u))>0){if(volt(u)>0)neighbors++; if(current_dir==2)nvolti = max(nvolti, int(volt(u)));}
        if(mat(d)!=0 && conductivity(mat(d))>0){if(volt(d)>0)neighbors++; if(current_dir==3)nvolti = max(nvolti, int(volt(d)));}
        if(mat(f)!=0 && conductivity(mat(f))>0){if(volt(f)>0)neighbors++; if(current_dir==4)nvolti = max(nvolti, int(volt(f)));}
        if(mat(b)!=0 && conductivity(mat(b))>0){if(volt(b)>0)neighbors++; if(current_dir==5)nvolti = max(nvolti, int(volt(b)));}
        dvolt = nvolti-volti;
        // if((rand(frame_number+rand(rand(rand(pos.z)+pos.y)+pos.x))%4)!=0) dvolt--;
        if(dvolt > 0 && neighbors > 1) dvolt = 0;
        volti = volti+dvolt;
        volt = clamp(volti, 0, 15);

        // int dvolt = 0;
        // int volti = int(volt);
        // if(mat(r)!=0 && conductivity(mat(r))>0){dvolt += 2*sign(int(volt(r))-volti);}
        // if(mat(l)!=0 && conductivity(mat(l))>0){dvolt += 2*sign(int(volt(l))-volti);}
        // if(mat(u)!=0 && conductivity(mat(u))>0){dvolt += 2*sign(int(volt(u))-volti);}
        // if(mat(d)!=0 && conductivity(mat(d))>0){dvolt += 2*sign(int(volt(d))-volti);}
        // if(mat(f)!=0 && conductivity(mat(f))>0){dvolt += 2*sign(int(volt(f))-volti);}
        // if(mat(b)!=0 && conductivity(mat(b))>0){dvolt += 2*sign(int(volt(b))-volti);}
        volt = uint(clamp(volti, 0, 15));
    }
    else
    {
        volt = 0;
    }
    if(frame_number == 0) volt = 0;

    // if(mat(c) == 4)
    // {
    //     volt = 15;
    // }

    uint temp = temp(c);
    // if(mat(c) == 4) temp = 255; else
    if(mat(c) != 0)
    {
        float C = heat_capacity(mat(c));
        float Q = 0;
        //NOTE: this would save some divisions by storing thermal resistivity,
        //      but it's nicer to define materials in terms of conductivity so 0 has well defined behavior
        float r_c = 1.0/thermal_conductivity(mat(c));
        {float k = 1.0/(r_c+1.0/thermal_conductivity(mat(r))); Q += k*(float(temp(r))-float(temp));}
        {float k = 1.0/(r_c+1.0/thermal_conductivity(mat(l))); Q += k*(float(temp(l))-float(temp));}
        {float k = 1.0/(r_c+1.0/thermal_conductivity(mat(u))); Q += k*(float(temp(u))-float(temp));}
        {float k = 1.0/(r_c+1.0/thermal_conductivity(mat(d))); Q += k*(float(temp(d))-float(temp));}
        {float k = 1.0/(r_c+1.0/thermal_conductivity(mat(f))); Q += k*(float(temp(f))-float(temp));}
        {float k = 1.0/(r_c+1.0/thermal_conductivity(mat(b))); Q += k*(float(temp(b))-float(temp));}

        //NOTE: there might be oscillations due to neighboring temperatures changing past each other,
        //      but fixing would require each voxel knowing how much heat it's neighbors are getting
        //      hopefully won't be a problem since everything will eventually converge to room temp

        float dtemp = Q/C;
        float dtemp_i = round(dtemp);
        float dtemp_f = 32.0*(dtemp-dtemp_i);

        if(dtemp_f < 0 && dtemp_f > -1.0) dtemp_f = -1.0;

        if((frame_number+rand(rand(rand(rand(pos.z)+pos.y)+pos.x)))%32 < int(abs(dtemp_f))) dtemp_i += sign(dtemp_f);
        temp = uint(clamp(float(temp)+dtemp_i, 0.0, 255.0));

        // if(avg_temp.x > float(temp)) temp++;
        // else if(avg_temp.x < float(temp)) temp--;
    }
    else temp = 100;

    // temp = clamp(temp+volt, 0u, 255u);

    uint phase = phase_solid;
    if(hardness(mat(c)) == 0.0
       || (mat(r) == 0 && mat(l) == 0 && mat(u) == 0 && mat(d) == 0 && mat(f) == 0 && mat(b) == 0)) phase = phase_sand;
    if(float(temp) > melting_point(mat(c))) phase = phase_liquid;
    if(float(temp) > boiling_point(mat(c))) phase = phase_gas;

    out_voxel = c;

    uint transient = transient(out_voxel);
    if(transient == 1)
    {
        out_voxel.r = 0;
        transient = 0;
    }

    out_voxel.g = uint(depth) | (phase << 5) | (transient << 7);

    out_voxel.b = temp;

    out_voxel.a = volt | (flow<<4);

    // bool changed = c.r != out_voxel.r || c.g != out_voxel.g || c.b != out_voxel.b;
    // bool changed = c.r != out_voxel.r || c.g != out_voxel.g;
    bool changed = c.r != out_voxel.r || c.g != out_voxel.g || (int(c.b) != int(out_voxel.b) && out_voxel.b > 128) || volt != volt(c);
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
