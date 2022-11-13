int n_texture_reads = 0;

bool cast_ray(isampler3D materials, vec3 ray_dir, vec3 ray_origin, ivec3 size, ivec3 origin, uint medium, out vec3 pos, out float hit_dist, out ivec3 hit_cell, out vec3 hit_dir, out vec3 normal, out uvec4 hit_voxel, int max_iterations)
{

    pos = ray_origin;
    vec3 ray_sign = sign(ray_dir);

    vec3 invabs_ray_dir = ray_sign/ray_dir;

    hit_dir = vec3(0);

    float epsilon = 0.02;
    int i = 0;
    hit_dist = 0;

    ivec3 ipos = ivec3(floor(pos));

    for(;;)
    {
        if(ipos.x < 0 || ipos.y < 0 || ipos.z < 0
           || ipos.x >= size.x || ipos.y >= size.y || ipos.z >= size.z)
        {
            return false;
        }

        #ifdef occupied_regions
        if(texelFetch(occupied_regions, ipos>>4, 0).r == 0)
        {
            vec3 dist = ((0.5f*ray_sign+0.5f)*16.0f+ray_sign*(16.0f*floor(pos*(1.0f/16.0f))-pos))*invabs_ray_dir;
            vec3 min_dir = step(dist.xyz, dist.zxy)*step(dist.xyz, dist.yzx);
            float min_dist = dot(dist, min_dir);
            hit_dir = min_dir;

            float step_dist = min_dist+epsilon;
            pos += step_dist*ray_dir;
            hit_dist += step_dist;
            // hit_dir = min_dir;

            ipos = ivec3(pos);
        }
        else
        #endif
        {
            n_texture_reads++;
            uvec4 voxel = texelFetch(materials, ipos+origin, 0);
            if(voxel.r != medium)
            {
                ivec3 ioutside = ipos - ivec3(hit_dir*ray_sign)+origin;
                // vec3 gradient = vec3(
                //     texelFetch(materials, ioutside+ivec3(1,0,0),0).g-texelFetch(materials, ioutside+ivec3(-1,0,0),0).g,
                //     texelFetch(materials, ioutside+ivec3(0,1,0),0).g-texelFetch(materials, ioutside+ivec3(0,-1,0),0).g,
                //     texelFetch(materials, ioutside+ivec3(0,0,1),0).g-texelFetch(materials, ioutside+ivec3(0,0,-1),0).g+0.001f
                //     );
                // normal = normalize(gradient);
                if(hit_dir == vec3(0,0,0))
                {
                    hit_dir = abs(pos-ipos+0.5);
                    hit_dir = step(hit_dir.zxy, hit_dir.xyz)*step(hit_dir.yzx, hit_dir.xyz);
                }
                normal = normalize(-hit_dir*ray_sign);

                hit_cell = ipos;
                hit_voxel = voxel;
                return true;
            }

            #ifndef RAY_CAST_IGNORE_DEPTH
            int depth = depth(voxel);
            if(depth >= 3
               // #ifdef ACTIVE_REGIONS
               // && texelFetch(active_regions, ipos>>4, 0).r == 0
               // #endif //ACTIVE_REGIONS
                )
            {
                float skip_dist = (depth-2)/dot(ray_dir,ray_sign);
                pos += ray_dir*skip_dist;
                hit_dist += skip_dist;
                ipos = ivec3(pos);
            }
            #endif

            vec3 dist = (0.5f*ray_sign+0.5f+ray_sign*(vec3(ipos)-pos))*invabs_ray_dir;

            vec3 min_dir = step(dist.xyz, dist.zxy)*step(dist.xyz, dist.yzx);
            float min_dist = dot(dist, min_dir);
            pos += min_dist*ray_dir;
            ipos += ivec3(min_dir*ray_sign);
            hit_dist += min_dist;
            hit_dir = min_dir;
        }

        if(++i >= max_iterations)
        {
            return false;
        }
    }
}
