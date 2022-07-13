bool coarse_cast_ray(vec3 ray_dir, vec3 ray_origin, out vec3 pos, out float hit_dist, out ivec3 hit_cell, out vec3 hit_dir, out vec3 normal)
{
    float step_size = 1.0;

    pos = ray_origin;
    vec3 ray_sign = sign(ray_dir);

    vec3 invabs_ray_dir = ray_sign/ray_dir;

    hit_dir = vec3(0);

    float epsilon = 0.02;
    const int max_iterations = 8;
    int i = 0;
    hit_dist = 0;

    ivec3 ipos = ivec3(floor(pos));

    for(;;)
    {
        if(ipos.x < -1 || ipos.y < -1 || ipos.z < -1
           || ipos.x > size.x || ipos.y > size.y || ipos.z > size.z)
        {
            return false;
        }
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

            ipos = ivec3(floor(pos));

            if(ipos.x < -1 || ipos.y < -1 || ipos.z < -1
               || ipos.x > size.x || ipos.y > size.y || ipos.z > size.z)
            {
                return false;
            }

            if(++i >= max_iterations)
            {
                return false;
            }
        }

        ivec4 voxel = texelFetch(materials, ipos+origin, 0);
        if(voxel.r != 0)
        {
            ivec3 ioutside = ipos - ivec3(hit_dir*ray_sign);
            vec3 gradient = vec3(
                texelFetch(materials, ioutside+ivec3(1,0,0),0).g-texelFetch(materials, ioutside+ivec3(-1,0,0),0).g,
                texelFetch(materials, ioutside+ivec3(0,1,0),0).g-texelFetch(materials, ioutside+ivec3(0,-1,0),0).g,
                texelFetch(materials, ioutside+ivec3(0,0,1),0).g-texelFetch(materials, ioutside+ivec3(0,0,-1),0).g+0.001f
                );
            normal = normalize(gradient);

            hit_cell = ipos;
            return true;
        }

        if(voxel.g >= 3)
        {
            float skip_dist = (voxel.g-2)/dot(ray_dir,ray_sign);
            pos += ray_dir*skip_dist;
            hit_dist += skip_dist;
            ipos = ivec3(floor(pos));
        }

        pos += ray_dir*step_size;
        hit_dist += step_size;
        step_size += 0.25f*step_size;
        ipos = ivec3(floor(pos));

        // vec3 dist = (0.5f*ray_sign+0.5f+ray_sign*(vec3(ipos)-pos))*invabs_ray_dir;

        // vec3 min_dir = step(dist.xyz, dist.zxy)*step(dist.xyz, dist.yzx);
        // float min_dist = dot(dist, min_dir);
        // pos += min_dist*ray_dir;
        // ipos += ivec3(min_dir*ray_sign);
        // hit_dist += min_dist;
        // hit_dir = min_dir;

        if(++i >= max_iterations)
        {
            return false;
        }
    }
    return false;
}

int n_texture_reads = 0;

bool cast_ray(isampler3D materials, vec3 ray_dir, vec3 ray_origin, ivec3 size, ivec3 origin, out vec3 pos, out float hit_dist, out ivec3 hit_cell, out vec3 hit_dir, out vec3 normal, out ivec4 hit_voxel)
{

    pos = ray_origin;
    vec3 ray_sign = sign(ray_dir);

    vec3 invabs_ray_dir = ray_sign/ray_dir;

    hit_dir = vec3(0);

    float epsilon = 0.02;
    int max_iterations = 200;
    int i = 0;
    hit_dist = 0;

    ivec3 ipos = ivec3(floor(pos));

    for(;;)
    {
        if(ipos.x < -1 || ipos.y < -1 || ipos.z < -1
           || ipos.x > size.x || ipos.y > size.y || ipos.z > size.z)
        {
            return false;
        }

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

            ipos = ivec3(floor(pos));
        }
        else
        {
            n_texture_reads++;
            ivec4 voxel = texelFetch(materials, ipos+origin, 0);
            if(voxel.r != 0)
            {
                ivec3 ioutside = ipos - ivec3(hit_dir*ray_sign)+origin;
                // vec3 gradient = vec3(
                //     texelFetch(materials, ioutside+ivec3(1,0,0),0).g-texelFetch(materials, ioutside+ivec3(-1,0,0),0).g,
                //     texelFetch(materials, ioutside+ivec3(0,1,0),0).g-texelFetch(materials, ioutside+ivec3(0,-1,0),0).g,
                //     texelFetch(materials, ioutside+ivec3(0,0,1),0).g-texelFetch(materials, ioutside+ivec3(0,0,-1),0).g+0.001f
                //     );
                // normal = normalize(gradient);
                normal = normalize(-hit_dir*ray_sign);

                hit_cell = ipos;
                hit_voxel = voxel;
                return true;
            }

            if(voxel.g >= 3)
            {
                float skip_dist = (voxel.g-2)/dot(ray_dir,ray_sign);
                pos += ray_dir*skip_dist;
                hit_dist += skip_dist;
                ipos = ivec3(floor(pos));
            }

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

        if(ipos.x < -1 || ipos.y < -1 || ipos.z < -1
           || ipos.x > size.x || ipos.y > size.y || ipos.z > size.z)
        {
            return false;
        }

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

            ipos = ivec3(floor(pos));
        }
        else
        {
            n_texture_reads++;
            ivec4 voxel = texelFetch(materials, ipos+origin, 0);
            if(voxel.r != 0)
            {
                ivec3 ioutside = ipos - ivec3(hit_dir*ray_sign)+origin;
                // vec3 gradient = vec3(
                //     texelFetch(materials, ioutside+ivec3(1,0,0),0).g-texelFetch(materials, ioutside+ivec3(-1,0,0),0).g,
                //     texelFetch(materials, ioutside+ivec3(0,1,0),0).g-texelFetch(materials, ioutside+ivec3(0,-1,0),0).g,
                //     texelFetch(materials, ioutside+ivec3(0,0,1),0).g-texelFetch(materials, ioutside+ivec3(0,0,-1),0).g+0.001f
                //     );
                // normal = normalize(gradient);
                normal = normalize(-hit_dir*ray_sign);

                hit_cell = ipos;
                hit_voxel = voxel;
                return true;
            }

            if(voxel.g >= 3)
            {
                float skip_dist = (voxel.g-2)/dot(ray_dir,ray_sign);
                pos += ray_dir*skip_dist;
                hit_dist += skip_dist;
                ipos = ivec3(floor(pos));
            }

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
