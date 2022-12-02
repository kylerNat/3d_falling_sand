#program simulate_chunk_atomic_program
/////////////////////////////////////////////////////////////////

#shader GL_COMPUTE_SHADER
#include "include/header.glsl"

#define localgroup_size 8
#define subgroup_size 2
layout(local_size_x = localgroup_size, local_size_y = localgroup_size, local_size_z = localgroup_size) in;

layout(location = 0) uniform int frame_number;
layout(location = 1) uniform sampler2D material_physical_properties;
layout(location = 2) uniform usampler3D materials;
layout(location = 3, rgba8ui) uniform uimage3D materials_out;
layout(location = 4) uniform usampler3D active_regions_in;
layout(location = 5) uniform writeonly uimage3D active_regions_out;
layout(location = 6) uniform writeonly uimage3D occupied_regions_out;
layout(location = 7) uniform int update_cells;
layout(location = 8) uniform int n_explosions;
layout(location = 9) uniform int n_beams;

#define MATERIAL_PHYSICAL_PROPERTIES
#include "include/materials_physical.glsl"

#include "include/particle_data.glsl"
#define EXPLOSION_DATA_BINDING 1
#include "include/explosion_data.glsl"
#define BEAM_DATA_BINDING 2
#include "include/beam_data.glsl"

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

uint conduction_dir(ivec3 pos)
{
    return rand(rand(frame_number+rand(rand(rand(pos.z)+pos.y)+pos.x)))%6;
}

bool c_active = false;
bool u_active = false;
bool d_active = false;
bool r_active = false;
bool l_active = false;
bool f_active = false;
bool b_active = false;

void simulate_voxel(ivec3 pos, ivec3 rpos)
{
    ivec3 cell_p = pos%16;
    // ivec3 cell_p = gl_LocalInvocationID.xyz;
    // ivec3 rpos = gl_WorkGroupID.xyz;

    if(cell_p == ivec3(0,0,0)) imageStore(occupied_regions_out, rpos, uvec4(0,0,0,0));

    uvec4 c  = texelFetch(materials, ivec3(pos.x, pos.y, pos.z),0);
    uvec4 old_voxel  = c;

    //+,0,-,0
    //0,+,0,-
    // int rot = (frame_number+layer)%4;
    // uint rot = rand(rand(rand(frame_number)))%4;
    uint flow = flow(c);
    int i = 0;
    ivec2 dir = ivec2(((flow&1)*(2-flow)), (1-(flow&1))*(1-flow)); //(0,1), (1,0), (0,-1), (-1,0)

    ivec3 pu = ivec3(pos.x, pos.y, pos.z+1);
    ivec3 pd = ivec3(pos.x, pos.y, pos.z-1);
    ivec3 pr = ivec3(pos.x+dir.x, pos.y+dir.y, pos.z);
    ivec3 pl = ivec3(pos.x-dir.x, pos.y-dir.y, pos.z);
    ivec3 pf = ivec3(pos.x-dir.y, pos.y+dir.x, pos.z);
    ivec3 pb = ivec3(pos.x+dir.y, pos.y-dir.x, pos.z);

    uvec4 u  = texelFetch(materials, pu, 0);
    uvec4 d  = texelFetch(materials, pd, 0);
    uvec4 r  = texelFetch(materials, pr, 0);
    uvec4 l  = texelFetch(materials, pl, 0);
    uvec4 f  = texelFetch(materials, pf, 0);
    uvec4 b  = texelFetch(materials, pb, 0);

    uvec4 dr = texelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z-1),0);
    uvec4 ur = texelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z+1),0);

    uvec4 df = texelFetch(materials, ivec3(pos.x-dir.y, pos.y+dir.x, pos.z-1),0);
    uvec4 db = texelFetch(materials, ivec3(pos.x+dir.y, pos.y-dir.x, pos.z-1),0);

    uvec4 ldf = texelFetch(materials, ivec3(pl.x-dir.y, pl.y+dir.x, pl.z-1),0);
    uvec4 ldb = texelFetch(materials, ivec3(pl.x+dir.y, pl.y-dir.x, pl.z-1),0);

    uvec4 ul = texelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z+1),0);
    uvec4 ll = texelFetch(materials, ivec3(pos.x-2*dir.x, pos.y-2*dir.y, pos.z),0);
    uvec4 dl = texelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z-1),0);

    uvec4 uu = texelFetch(materials, ivec3(pos.x, pos.y, pos.z+2),0);

    if(update_cells == 1 && frame_number != 0)
    {
        if(mat(c) == 0)
        { //central cell is empty

            //check if upper cell fell           , liquids and sands
            //then check if upper left cell fell , liquids and sands
            //then check if left cell flowed     , liquids
            if(mat(u) != 0 && phase(u) >= phase_sand && transient(u)==0)
                c = u;
            else if(mat(ul) != 0 && mat(l) != 0 && mat(u) == 0 && phase(ul) >= phase_sand && flow(ul) == flow && transient(ul)==0)
                c = ul;
            else if(mat(l) != 0 && mat(dl) != 0 && mat(d) != 0 && mat(ldf) != 0 && mat(ldb) != 0 && (phase(d) >= phase_liquid || phase(dl) >= phase_liquid) && phase(l) == phase_liquid && flow(l) == flow && transient(l)==0)
                c = l;
            else if(mat(d) != 0 && phase(d) == phase_gas && transient(d)==0)
                c = d;
            else if(mat(dl) != 0 && mat(l) != 0 && mat(d) == 0 && phase(dl) == phase_gas && flow(dl) == flow && transient(dl)==0)
                c = dl;
            else if(mat(l) != 0 && mat(ul) != 0 && mat(u) != 0 && phase(l) == phase_gas && flow(l) == flow && transient(l)==0)
                c = l;
            else
            {
                // flow = rand(rand(rand(frame_number+pos.x+pos.y+pos.z)))%4;
                // flow = (frame_number)%4;
                flow = rand(871841735+frame_number)%4;

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
            bool fall_allowed = (pos.z > 0 && (mat(d) == 0 || (mat(dr) == 0 && mat(r) == 0 && flow(dr) == flow)) && phase(c) >= phase_sand);
            bool flow_allowed = (pos.z > 0 && mat(r) == 0
                                 && (mat(ur) == 0 || phase(ur) < phase_sand || flow(ur) != flow)
                                 && mat(df) != 0 && mat(db) != 0
                                 && (mat(u) == 0 || phase(u) < phase_sand)
                                 && (phase(dr) >= phase_liquid || phase(d) >= phase_liquid) && phase(c) == phase_liquid && flow(r) == flow);

            bool float_allowed = (pos.z < 511 && (mat(u) == 0 || (mat(ur) == 0 && mat(r) == 0 && flow(ur) == flow)) && phase(c) == phase_gas && (mat(uu) == 0 || (phase(uu) <= phase_solid)));

            bool gas_flow_allowed = (pos.z > 0 && mat(r) == 0
                                     && mat(ur) != 0 && mat(u) != 0
                                     && (mat(dr) == 0 || phase(dr) < phase_sand || flow(dr) != flow)
                                     && (mat(d) == 0 || phase(d) < phase_sand)
                                     && phase(c) == phase_gas && flow(r) == flow);

            if(fall_allowed || flow_allowed || float_allowed || gas_flow_allowed) c = uvec4(0);
            else
            {
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

    int depth = MAX_DEPTH-1;
    uint solidity = solidity(c);
    if((solidity(u) != solidity) ||
       (solidity(d) != solidity) ||
       (solidity(r) != solidity) ||
       (solidity(l) != solidity) ||
       (solidity(f) != solidity) ||
       (solidity(b) != solidity)) depth = 0;
    else
    {
        depth = min(depth, depth(u)+1);
        depth = min(depth, depth(d)+1);
        depth = min(depth, depth(r)+1);
        depth = min(depth, depth(l)+1);
        depth = min(depth, depth(f)+1);
        depth = min(depth, depth(b)+1);
    }

    int volt = int(volt(c));

    if(conductivity(mat(c)) > 0)
    {
        uint cd = conduction_dir(pos);
        int dvolt = 0;
        int nvolt = volt;
        int neighbors = 0;
        if(mat(r)!=0 && conductivity(mat(r))>0){if(volt(r)>0)neighbors++; nvolt = max(nvolt, int(volt(r)));}
        if(mat(l)!=0 && conductivity(mat(l))>0){if(volt(l)>0)neighbors++; nvolt = max(nvolt, int(volt(l)));}
        if(mat(u)!=0 && conductivity(mat(u))>0){if(volt(u)>0)neighbors++; nvolt = max(nvolt, int(volt(u)));}
        if(mat(d)!=0 && conductivity(mat(d))>0){if(volt(d)>0)neighbors++; nvolt = max(nvolt, int(volt(d)));}
        if(mat(f)!=0 && conductivity(mat(f))>0){if(volt(f)>0)neighbors++; nvolt = max(nvolt, int(volt(f)));}
        if(mat(b)!=0 && conductivity(mat(b))>0){if(volt(b)>0)neighbors++; nvolt = max(nvolt, int(volt(b)));}
        dvolt = nvolt-volt;
        if((rand(frame_number+rand(rand(rand(pos.z)+pos.y)+pos.x))%7)!=0) dvolt--;
        // if(volt <= 8 && neighbors == 0) dvolt--;
        if(dvolt > 0 && neighbors > 1) dvolt = 0;
        volt = volt+dvolt;
        volt = clamp(volt, 0, 15);
    }
    else
    {
        volt = 0;
    }
    if(frame_number == 0) volt = 0;

    uint temp = temp(c);
    // if(mat(c) == 4) temp = 255; else
    if(mat(c) != 0)
    {
        float C = heat_capacity(mat(c));
        float Q = 0;
        //NOTE: this would save some divisions by storing thermal resistivity,
        //      but it's nicer to define materials in terms of conductivity so 0 has well defined behavior
        float r_c = 1.0/thermal_conductivity(mat(c));
        {float k = 1.0/(r_c+1.0/thermal_conductivity(mat(u))); Q += k*(float(temp(u))-float(temp));}
        {float k = 1.0/(r_c+1.0/thermal_conductivity(mat(d))); Q += k*(float(temp(d))-float(temp));}
        {float k = 1.0/(r_c+1.0/thermal_conductivity(mat(r))); Q += k*(float(temp(r))-float(temp));}
        {float k = 1.0/(r_c+1.0/thermal_conductivity(mat(l))); Q += k*(float(temp(l))-float(temp));}
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
    else temp = room_temp;

    // temp = clamp(temp+abs((volt-int(volt(c)))), 0u, 255u);
    temp = clamp(temp+volt, 0u, 255u);

    if(mat(c) == 6 && temp < 128) c.r = 0;

    uint phase = phase_solid;
    if(hardness(mat(c)) == 0.0
       || (mat(u) == 0 && mat(d) == 0 && mat(r) == 0 && mat(l) == 0 && mat(f) == 0 && mat(b) == 0)) phase = phase_sand;
    if(float(temp) > melting_point(mat(c))) phase = phase_liquid;
    if(float(temp) > boiling_point(mat(c))) phase = phase_gas;

    uvec4 out_voxel = c;

    uint transient = transient(out_voxel);
    if(transient == 1)
    {
        out_voxel.r = 0;
        transient = 0;
    }

    out_voxel.g = uint(depth) | (phase << 5) | (transient << 7);

    out_voxel.b = temp;

    out_voxel.a = volt | (flow<<4);

    vec3 voxel_x = (vec3(pos)+0.5);
    for(int e = 0; e < n_explosions; e++)
    {
        vec3 r = voxel_x-explosion_x(e);
        if(dot(r, r) <= sq(explosions[e].r))
        {
            if(mat(out_voxel) != 0)
            {
                int dead_index = atomicAdd(n_dead_particles, -1)-1;
                //this assumes particle creation and destruction never happen simutaneously
                uint p = dead_particles[dead_index];
                particles[p].voxel_data = out_voxel.r|out_voxel.g<<8|out_voxel.b<<16|out_voxel.a<<24;
                particles[p].x = voxel_x.x;
                particles[p].y = voxel_x.y;
                particles[p].z = voxel_x.z;
                // vec3 x_dot = normalize(unnormalized_gradient(materials, pos));
                vec3 x_dot = 2*r/explosions[e].r;
                particles[p].x_dot = x_dot.x;
                particles[p].y_dot = x_dot.y;
                particles[p].z_dot = x_dot.z;
                particles[p].is_visual = true;
                particles[p].die_on_collision = true;
                particles[p].alive = true;
            }

            out_voxel = uvec4(0);
        }
        else if(dot(r, r) <= sq(explosions[e].r+1))
        {
            out_voxel.b = clamp(out_voxel.b+100, 0u, 255u);
        }
    }

    for(int be = 0; be < n_beams; be++)
    {
        vec3 delta = voxel_x-beam_x(be);
        vec3 dhat = normalize(beam_d(be));
        float d = clamp(dot(dhat, delta), 0.0, length(beam_d(be)));
        vec3 nearest_x = d*dhat+beam_x(be);
        vec3 r = voxel_x-nearest_x;
        if(dot(r, r) <= sq(beams[be].r))
        {
            out_voxel.b = clamp(out_voxel.b+100, 0u, 255u);
        }
    }

    bool changed = old_voxel.r != out_voxel.r || old_voxel.g != out_voxel.g || (int(old_voxel.b) != int(out_voxel.b) && out_voxel.b > 128) || out_voxel.b > 130 || volt != volt(old_voxel) || volt > 0;
    if(changed)
    {
        c_active = true;
        u_active = u_active || cell_p.z>=15;
        d_active = d_active || cell_p.z<= 0;
        r_active = r_active || cell_p.y>=15;
        l_active = l_active || cell_p.y<= 0;
        f_active = f_active || cell_p.x>=15;
        b_active = b_active || cell_p.x<= 0;
    }

    if(out_voxel.r != 0) imageStore(occupied_regions_out, ivec3(pos.x/16, pos.y/16, pos.z/16), uvec4(1,0,0,0));

    // memoryBarrier();
    imageStore(materials_out, pos, out_voxel);
}

void main()
{
    ivec3 pos = subgroup_size*ivec3(gl_GlobalInvocationID.xyz);
    ivec3 rpos = pos>>4;

    uint region_active = texelFetch(active_regions_in, rpos, 0).r;
    if(region_active == 0)
    {
        return;
    }

    for(int x = 0; x < subgroup_size; x++)
        for(int y = 0; y < subgroup_size; y++)
            for(int z = 0; z < subgroup_size; z++)
                simulate_voxel(pos+ivec3(x,y,z), rpos);

    if(c_active) imageStore(active_regions_out, ivec3(rpos.x, rpos.y, rpos.z),   uvec4(1,0,0,0));
    if(u_active) imageStore(active_regions_out, ivec3(rpos.x, rpos.y, rpos.z+1), uvec4(1,0,0,0));
    if(d_active) imageStore(active_regions_out, ivec3(rpos.x, rpos.y, rpos.z-1), uvec4(1,0,0,0));
    if(r_active) imageStore(active_regions_out, ivec3(rpos.x, rpos.y+1, rpos.z), uvec4(1,0,0,0));
    if(l_active) imageStore(active_regions_out, ivec3(rpos.x, rpos.y-1, rpos.z), uvec4(1,0,0,0));
    if(f_active) imageStore(active_regions_out, ivec3(rpos.x+1, rpos.y, rpos.z), uvec4(1,0,0,0));
    if(b_active) imageStore(active_regions_out, ivec3(rpos.x-1, rpos.y, rpos.z), uvec4(1,0,0,0));
}
