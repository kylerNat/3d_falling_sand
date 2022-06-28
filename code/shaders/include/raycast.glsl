void cast_ray(vec3 ray_dir, vec3 ray_origin, out vec3 pos, out float hit_dist, out ivec3 hit_cell, out vec3 hit_dir, out vec3 normal)
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
            hit_cell.x = -1;
            return;
        }
        while(texelFetch(occupied_regions, ipos/16, 0).r == 0)
        {
            vec3 dist = ((0.5f*ray_sign+0.5f)*16.0f+ray_sign*(16.0f*floor(pos/16.0f)-pos))*invabs_ray_dir;
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
                hit_cell.x = -1;
                return;
            }

            if(++i >= max_iterations)
            {
                hit_cell.x = -1;
                return;
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
            return;
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

        if(++i >= max_iterations)
        {
            hit_cell.x = -1;
            return;
        }
    }
}
