define_program(
    sprite,
    ( //shaders
        {GL_VERTEX_SHADER, "<2d sprite vertex shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             ///////////////////////////////////////////////////////////////////
             layout(location = 0) in vec3 x; //coords from -1 to 1
             layout(location = 1) in vec3 X; //center position
             layout(location = 2) in float r;
             layout(location = 3) in vec2 u0; //lower cordinate of the texture to sample from
             layout(location = 4) in vec2 u1; //upper " " " "

             layout(location = 0) uniform mat4 t;

             smooth out vec2 uv;

             void main()
             {
                 gl_Position.xyz = (x*r+X);
                 gl_Position.w = 1.0;
                 gl_Position = t*gl_Position;
                 uv = u0+u1*0.5*(x.xy+vec2(1,1));
             }
             /*/////////////////////////////////////////////////////////////*/)},
        {GL_FRAGMENT_SHADER, "<2d sprite fragment shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             ///////////////////////////////////////////////////////////////////
             layout(location = 0) out vec4 frag_color;

             layout(location = 1) uniform sampler2D spritesheet;

             smooth in vec2 uv;

             void main()
             {
                 frag_color = texture(spritesheet, uv);
             }
             /*/////////////////////////////////////////////////////////////*/)}
        ));

define_program(
    circle,
    ( //shaders
    {GL_VERTEX_SHADER, "<2d circle vertex shader>",
            DEFAULT_HEADER SHADER_SOURCE(
                /////////////////<2d circle vertex shader>/////////////////
                layout(location = 0) in vec3 x;
                layout(location = 1) in vec3 X;
                layout(location = 2) in float r;
                layout(location = 3) in vec4 c;

                smooth out vec4 color;
                smooth out vec2 uv;
                smooth out float radius;

                layout(location = 0) uniform mat4 t;

                void main()
                {
                    gl_Position.xyz = (x*r+X);
                    gl_Position.w = 1.0;
                    gl_Position = t*gl_Position;
                    uv = x.xy*r;
                    radius = r;
                    color = c;
                }
                /*/////////////////////////////////////////////////////////////*/)},
    {GL_FRAGMENT_SHADER, "<2d circle fragment shader>",
            DEFAULT_HEADER SHADER_SOURCE(
                ////////////////<2d circle fragment shader>////////////////
                layout(location = 0) out vec4 frag_color;

                layout(location = 1) uniform float smoothness;

                smooth in vec4 color;
                smooth in vec2 uv;
                smooth in float radius;

                // uniform sampler2D tex;

                void main()
                {
                    frag_color = color;
                    float alpha = smoothstep(-radius, -radius+smoothness, -length(uv));
                    frag_color.a *= alpha;
                    gl_FragDepth = gl_FragCoord.z*alpha;
                }
                /*/////////////////////////////////////////////////////////////*/)}
        ));

define_program(
    fullscreen_texture,
    ( //shaders
    {GL_VERTEX_SHADER, "<2d circle vertex shader>",
            DEFAULT_HEADER SHADER_SOURCE(
                /////////////////<2d circle vertex shader>/////////////////
                layout(location = 0) in vec3 x;

                layout(location = 0) uniform mat4 t;

                smooth out vec2 uv;

                void main()
                {
                    gl_Position.xyz = x;
                    gl_Position.w = 1.0;
                    uv = 0.5f*x.xy+0.5f;
                }
                /*/////////////////////////////////////////////////////////////*/)},
    {GL_FRAGMENT_SHADER, "<2d circle fragment shader>",
            DEFAULT_HEADER SHADER_SOURCE(
                ////////////////<2d circle fragment shader>////////////////
                layout(location = 0) out vec4 frag_color;

                layout(location = 1) uniform sampler2D color;

                smooth in vec2 uv;

                void main()
                {
                    frag_color = texture(color, uv);
                }
                /*/////////////////////////////////////////////////////////////*/)}
        ));


define_program(
    rectangle,
    ( //shaders
    {GL_VERTEX_SHADER, "<2d rectangle vertex shader>",
            DEFAULT_HEADER SHADER_SOURCE(
                /////////////////<2d circle vertex shader>/////////////////

                layout(location = 0) in vec3 x;
                layout(location = 1) in vec3 X;
                layout(location = 2) in vec2 r;
                layout(location = 3) in vec4 c;

                layout(location = 0) uniform mat4 t;
                layout(location = 1) uniform float smoothness;

                smooth out vec4 color;
                smooth out vec2 uv;
                smooth out vec2 radius;

                void main()
                {
                    uv = x.xy*(r+2.0*smoothness);
                    gl_Position.xy = uv+X.xy;
                    gl_Position.z = x.z+X.z;
                    gl_Position.w = 1.0;
                    gl_Position = t*gl_Position;
                    radius = r;
                    color = c;
                }
                /*/////////////////////////////////////////////////////////////*/)},
    {GL_FRAGMENT_SHADER, "<2d rectangle fragment shader>",
            DEFAULT_HEADER SHADER_SOURCE(
                ////////////////<2d circle fragment shader>////////////////
                layout(location = 1) uniform float smoothness;

                smooth in vec4 color;
                smooth in vec2 uv;
                smooth in vec2 radius;

                void main()
                {
                    gl_FragColor = color;
                    gl_FragColor.a *= smoothstep(-radius.x, -radius.x+smoothness, -abs(uv.x));
                    gl_FragColor.a *= smoothstep(-radius.y, -radius.y+smoothness, -abs(uv.y));
                }
                /*/////////////////////////////////////////////////////////////*/)}
        ));

define_program(
    round_line,
    ( //shaders
    {GL_VERTEX_SHADER, "<2d rounded line vertex shader>",
            DEFAULT_HEADER SHADER_SOURCE(
                /////////////////////////////////////////////////////////////////
                layout(location = 0) in vec3 X;

                layout(location = 0) uniform mat4 t;

                smooth out vec3 x;

                void main()
                {
                    gl_Position.xyz = X;
                    gl_Position.w = 1.0;
                    gl_Position = t*gl_Position;
                    x = X;
                }
                /*/////////////////////////////////////////////////////////////*/)},
    {GL_FRAGMENT_SHADER, "<2d rounded line fragment shader>",
            DEFAULT_HEADER SHADER_SOURCE(
                /////////////////////////////////////////////////////////////////
                layout(location = 0) out vec4 frag_color;

                smooth in vec3 x;

                layout(location = 1) uniform float smoothness;
                layout(location = 2) uniform sampler2D points;
                layout(location = 3) uniform int n_points;

                const int N_MAX_POINTS = 4096;

                vec3 dist_to_segment(int segment_id)
                {
                    vec4 a = texelFetch(points, ivec2(0, segment_id), 0); //xyz radius
                    vec4 b = texelFetch(points, ivec2(0, segment_id+1), 0); //xyz radius
                    vec3 v = b.xyz-a.xyz;

                    //calculate nearest point to the central line
                    float v_sq = dot(v, v);
                    float t = dot(x-a.xyz, v)/v_sq;
                    float r = mix(a.w, b.w, t);
                    vec3 x_rel = x - mix(a.xyz, b.xyz, t);

                    //adjust point to be nearest to the line accounting for the thickness
                    float Deltar = b.w-a.w;
                    if(v_sq < Deltar*Deltar) {
                        t = (Deltar < 0.0) ? 0.0 : 1.0;
                    } else {
                        t += dot(x_rel, x_rel)*Deltar/(v_sq*r);
                        t = clamp(t, 0.0, 1.0);
                    }

                    //recalculate for new point
                    r = mix(a.w, b.w, t);
                    x_rel = x - mix(a.xyz, b.xyz, t);
                    float distance = length(x_rel);
                    return vec3(r, t, distance);
                }

                void main()
                {
                    //find global min distance
                    vec3 dist = dist_to_segment(0);
                    int best_id = 0;
                    vec3 test_dist;

                    for(int test_id = 1; test_id <= N_MAX_POINTS-1; test_id++) {
                        if(test_id >= n_points-1) break;
                        test_dist = dist_to_segment(test_id);
                        if(dist.r*test_dist.z <= test_dist.r*dist.z) {
                            dist = test_dist;
                            best_id = test_id;
                        }
                    }

                    vec4 ca = texelFetch(points, ivec2(1, best_id), 0);
                    vec4 cb = texelFetch(points, ivec2(1, best_id+1), 0);

                    vec4 c = mix(ca, cb, dist.t); //NOTE: could probably get this automatically by sampling the texture at the right point with interpolation enabled

                    float alpha = smoothstep(-dist.r, -dist.r+smoothness, -dist.z);
                    frag_color = c;
                    frag_color.a *= alpha;
                    gl_FragDepth = gl_FragCoord.z*alpha;
                }
                /*/////////////////////////////////////////////////////////////*/)}
        ));

define_program(
    render_chunk,
    ( //shaders
        {GL_VERTEX_SHADER, "<render chunk vertex shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             /////////////////////////////////////////////////////////////////
             layout(location = 0) in vec3 x;

             smooth out vec2 screen_pos;

             void main()
             {
                 gl_Position.xyz = x;
                 gl_Position.w = 1.0;
                 screen_pos = x.xy;
             }
             /*/////////////////////////////////////////////////////////////*/)},
        {GL_FRAGMENT_SHADER, "<render chunk fragment shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             /////////////////////////////////////////////////////////////////
             layout(location = 0) out vec4 frag_color;

             layout(location = 0) uniform mat3 camera_axes;
             layout(location = 1) uniform vec3 camera_pos;
             layout(location = 2) uniform isampler3D materials;
             layout(location = 3) uniform usampler3D occupied_regions;
             layout(location = 4) uniform ivec3 size;
             layout(location = 5) uniform ivec3 origin;

             smooth in vec2 screen_pos;

             uint rand(uint seed)
             {
                 seed ^= seed<<13;
                 seed ^= seed>>17;
                 seed ^= seed<<5;
                 return seed;
             }

             // float float_noise(uint seed)
             // {
             //     return fract(float(int(seed))/1.0e9);
             // }

             float float_noise(vec2 co){
                 return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
             }

             ivec4 voxelFetch(isampler3D tex, ivec3 coord)
             {
                 if(coord.x < 0
                    || coord.y < 0
                    || coord.z < 0
                    || coord.x >= size.x
                    || coord.y >= size.y
                    || coord.z >= size.z) return ivec4(0,1,1000,0);
                 return texelFetch(materials, coord+origin, 0);
                 // uint data = texelFetch(materials, chunk_origin+offset, 0).r;
                 // int material = data >> 6;
                 // int distance = (data>>2)&0xF - 7;
                 // int edgetype = (data)&0x3;
                 // return vec3(material, distance, edgetype);
             }

             void main()
             {
                 float fov = pi*120.0/180.0;
                 float screen_dist = 1.0/tan(fov/2);
                 vec3 ray_dir = (16.0/9.0*screen_pos.x*camera_axes[0]
                                 +        screen_pos.y*camera_axes[1]
                                 -        screen_dist *camera_axes[2]);
                 // vec3 ray_dir = (16.0/9.0*sin(screen_pos.x)*camera_axes[0]
                 //                 +        sin(screen_pos.y)*camera_axes[1]
                 //                 -        cos(screen_pos.x)*cos(screen_pos.y)*screen_dist *camera_axes[2]);
                 ray_dir = normalize(ray_dir);

                 int bounces_remaining = 1;
                 frag_color = vec4(0,0,0,1);
                 vec3 color_multiplier = vec3(1,1,1);
                 bool first_hit = true;
                 float coarse_step = 1.0;

                 vec3 pos = camera_pos;
                 vec3 ray_sign = sign(ray_dir);

                 vec3 invabs_ray_dir = ray_sign/ray_dir;

                 vec3 hit_dir = vec3(0);

                 float epsilon = 0.02;
                 int max_iterations = 150;
                 int i = 0;
                 float total_dist = 0;

                 if(pos.x < 0 && ray_dir.x > 0)      total_dist = max(total_dist, -epsilon+(-pos.x)/(ray_dir.x));
                 if(pos.x > size.x && ray_dir.x < 0) total_dist = max(total_dist, -epsilon+(size.x-pos.x)/(ray_dir.x));
                 if(pos.y < 0 && ray_dir.y > 0)      total_dist = max(total_dist, -epsilon+(-pos.y)/(ray_dir.y));
                 if(pos.y > size.y && ray_dir.y < 0) total_dist = max(total_dist, -epsilon+(size.y-pos.y)/(ray_dir.y));
                 if(pos.z < 0 && ray_dir.z > 0)      total_dist = max(total_dist, -epsilon+(-pos.z)/(ray_dir.z));
                 if(pos.z > size.z && ray_dir.z < 0) total_dist = max(total_dist, -epsilon+(size.z-pos.z)/(ray_dir.z));

                 pos += total_dist*ray_dir;

                 ivec3 ipos = ivec3(floor(pos));

                 // {
                 //     vec3 dist = ((0.5*ray_sign+0.5)*size-ray_sign*pos)*invabs_ray_dir;
                 //     float min_dist = dist.x;
                 //     int min_dir = 0;
                 //     if(dist.y < min_dist) {
                 //         min_dist = dist.y;
                 //         min_dir = 1;
                 //     }
                 //     if(dist.z < min_dist) {
                 //         min_dist = dist.z;
                 //         min_dir = 2;
                 //     }
                 //     ivec3 max_displacement = ivec3(ceil(abs(min_dist*ray_dir)));
                 //     max_iterations = max_displacement.x+max_displacement.y+max_displacement.z;
                 // }
                 // max_iterations = min(max_iterations, 100);
                 // max_iterations = max(max_iterations, 100);

                 for(;;)
                 {
                     if(ipos.x < -1 || ipos.y < -1 || ipos.z < -1
                        || ipos.x > size.x || ipos.y > size.y || ipos.z > size.z)
                     {
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
                         total_dist += step_dist;
                         // hit_dir = min_dir;

                         ipos = ivec3(floor(pos));

                         if(ipos.x < -1 || ipos.y < -1 || ipos.z < -1
                            || ipos.x > size.x || ipos.y > size.y || ipos.z > size.z)
                         {
                             return;
                         }

                         if(++i >= max_iterations)
                         {
                             return;
                         }
                     }

                     ivec4 voxel = texelFetch(materials, ipos+origin, 0);
                     if(voxel.r != 0)
                     {
                         float roughness = 0.0f;
                         vec3 emission = vec3(0.5f);
                         if(voxel.r == 3)
                         {
                             emission = vec3(0.05,0.05,0.1);
                         }
                         if(voxel.r == 4)
                         {
                             emission = vec3(1.0,1.0,1.0);
                         }

                         // emission *= 1.0f-total_dist/(512.0f*sqrt(3));

                         if(voxel.r > 1)
                         {
                             // color_multiplier *= 0.25;
                             color_multiplier *= vec3(0.1,0.1,0.2);
                             roughness = 0.1;
                         }
                         else
                         {
                             // color_multiplier *= 0.1;
                             color_multiplier *= vec3(0.54,0.44,0.21);
                             roughness = 0.9;
                         }

                         ivec3 ioutside = ipos - ivec3(hit_dir*ray_sign);
                         vec3 gradient = vec3(
                             voxelFetch(materials, ioutside+ivec3(1,0,0)).g-voxelFetch(materials, ioutside+ivec3(-1,0,0)).g,
                             voxelFetch(materials, ioutside+ivec3(0,1,0)).g-voxelFetch(materials, ioutside+ivec3(0,-1,0)).g,
                             voxelFetch(materials, ioutside+ivec3(0,0,1)).g-voxelFetch(materials, ioutside+ivec3(0,0,-1)).g+0.001f
                             );
                         vec3 normal = gradient;
                         normal += roughness*vec3(float_noise(gl_FragCoord.xy+i*pos.yx+vec2(108,93))-0.5f,
                                                    float_noise(gl_FragCoord.xy+i*pos.yx+vec2(48,139))-0.5f,
                                                    float_noise(gl_FragCoord.xy+i*pos.yx+vec2(389,12))-0.5f);
                         normal = normalize(normal);
                         gradient = normalize(gradient);

                         frag_color.rgb += color_multiplier*emission*(0.9f+0.1f*normal)*dot(normal, gradient);

                         if(first_hit)
                         {
                             gl_FragDepth = 1.0f-total_dist/(512.0f*sqrt(3));
                             first_hit = false;
                         }
                         if(bounces_remaining-- <= 0)
                         {
                             break;
                         }
                         else
                         {
                             ray_dir -= 2*dot(ray_dir, normal)*normal;

                             ray_sign = sign(ray_dir);

                             invabs_ray_dir = ray_sign/ray_dir;
                         }
                     }

                     if(first_hit)
                     {
                         if(voxel.g >= 3)
                         {
                             float skip_dist = (voxel.g-2)/dot(ray_dir,ray_sign);
                             // skip_dist -= mod(0.1*(gl_FragCoord.x+gl_FragCoord.y), 0.5);
                             pos += ray_dir*skip_dist;
                             total_dist += skip_dist;
                             ipos = ivec3(floor(pos));
                         }

                         vec3 dist = (0.5f*ray_sign+0.5f+ray_sign*(vec3(ipos)-pos))*invabs_ray_dir;

                         vec3 min_dir = step(dist.xyz, dist.zxy)*step(dist.xyz, dist.yzx);
                         float min_dist = dot(dist, min_dir);
                         pos += min_dist*ray_dir;
                         ipos += ivec3(min_dir*ray_sign);
                         total_dist += min_dist;
                         hit_dir = min_dir;
                     }
                     else
                     {
                         pos += ray_dir*coarse_step;
                         total_dist += coarse_step;
                         ipos = ivec3(floor(pos));
                         coarse_step += 1.0f*coarse_step*(float_noise(pos.xy+pos.zz));
                     }

                     // if(voxel.g >= 3)
                     // {
                     //     ivec3 original_ipos = ipos;
                     //     while(dot(abs(ipos-original_ipos), vec3(1,1,1)) < voxel.g-2)
                     //     {
                     //         pos += 0.3*ray_dir;
                     //         total_dist += 0.5;
                     //         ipos = ivec3(pos);
                     //     }
                     // }

                     if(++i >= max_iterations)
                     {
                         return;
                     }
                 }

                 // frag_color.rgb = mix(vec3(1,0,0), vec3(0,0,1), i*1.0/200);
                 // frag_color.rgb *= clamp(1.0-1.0*total_dist/chunk_size, 0, 1);
             }
             /*/////////////////////////////////////////////////////////////*/)}
        ));

define_program(
    simulate_chunk,
    ( //shaders
        {GL_VERTEX_SHADER, "<simulate chunk vertex shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             /////////////////////////////////////////////////////////////////
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
             /*/////////////////////////////////////////////////////////////*/)},
        {GL_FRAGMENT_SHADER, "<simulate chunk fragment shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             /////////////////////////////////////////////////////////////////
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
             /*/////////////////////////////////////////////////////////////*/)}
        ));

define_program(
    simulate_particles,
    ( //shaders
        {GL_VERTEX_SHADER, "<simulate particles vertex shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             /////////////////////////////////////////////////////////////////
             layout(location = 0) in vec3 x;

             smooth out vec2 uv;

             void main()
             {
                 gl_Position.xyz = x;
                 gl_Position.w = 1.0;
                 uv = 0.5*x.xy+0.5;
             }
             /*/////////////////////////////////////////////////////////////*/)},
        {GL_FRAGMENT_SHADER, "<simulate particles fragment shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             /////////////////////////////////////////////////////////////////
             layout(location = 0) out vec3 new_x;
             layout(location = 1) out vec3 new_x_dot;

             layout(location = 0) uniform int frame_number;
             layout(location = 1) uniform isampler3D materials;
             layout(location = 2) uniform sampler2D old_x;
             layout(location = 3) uniform sampler2D old_x_dot;

             smooth in vec2 uv;

             uint rand(uint seed)
             {
                 seed ^= seed<<13;
                 seed ^= seed>>17;
                 seed ^= seed<<5;
                 return seed;
             }

             float float_noise(uint seed)
             {
                 return float(int(seed))/1.0e10;
             }

             const int chunk_size = 256;

             ivec4 voxelFetch(isampler3D tex, ivec3 coord)
             {
                 return texelFetch(materials, coord, 0);
             }

             void main()
             {
                 float scale = 1.0/chunk_size;
                 vec3 x = texture(old_x, vec2(0, 0)).rgb;
                 vec3 x_dot = texture(old_x_dot, vec2(0, 0)).rgb;
                 float epsilon = 0.001;
                 // float epsilon = 0.02;

                 vec3 x_ddot = vec3(0,0,-0.08);
                 x_dot += x_ddot;

                 float remaining_dist = length(x_dot);
                 if(remaining_dist > epsilon)
                 {
                     x += x_dot;
                     if(x.z < epsilon && x_dot.z < 0)
                     {
                         x.z = epsilon;
                         x_dot.z = 0;
                         // x_dot *= 0.99;
                     }
                     int max_iterations = 50;
                     int i = 0;
                     // while(voxelFetch(materials, ivec3(x)).r == 1 || voxelFetch(materials, ivec3(x)).r == 3)
                     while(voxelFetch(materials, ivec3(x)).r != 0)
                     {
                         vec3 gradient = vec3(
                             voxelFetch(materials, ivec3(x+vec3(1,0,0))).g-voxelFetch(materials, ivec3(x+vec3(-1,0,0))).g,
                             voxelFetch(materials, ivec3(x+vec3(0,1,0))).g-voxelFetch(materials, ivec3(x+vec3(0,-1,0))).g,
                             voxelFetch(materials, ivec3(x+vec3(0,0,1))).g-voxelFetch(materials, ivec3(x+vec3(0,0,-1))).g+0.001f
                             );
                         gradient = normalize(gradient);
                         x += 0.05*gradient;
                         float rej = dot(x_dot, gradient);
                         x_dot -= gradient*rej;
                         // x_dot *= 0.99;
                         // x_dot += 1.0*gradient;
                         if(++i > max_iterations) break;

                         // x.z += 0.01;
                         // x.z = ceil(x.z);
                     }
                     if(voxelFetch(materials, ivec3(x)).r == 2)
                     {
                         x_dot *= 0.95;
                     }

                     // if(hit_dir == 0) x.x -= epsilon*sign.x;
                     // if(hit_dir == 1) x.y -= epsilon*sign.y;
                     // if(hit_dir == 2) x.z -= epsilon*sign.z;
                 }

                 new_x = x;
                 new_x_dot = x_dot;
             }
             /*/////////////////////////////////////////////////////////////*/)}
        ));

define_program(
    collide_body,
    ( //shaders
        {GL_VERTEX_SHADER, "<collide body vertex shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             /////////////////////////////////////////////////////////////////
             layout(location = 0) in vec3 x;

             smooth out vec2 uv;

             void main()
             {//TODO: only need to run fragments over a projected cuboid region
                 gl_Position.xyz = x;
                 gl_Position.w = 1.0;
                 uv = 0.5*x.xy+0.5;
             }
             /*/////////////////////////////////////////////////////////////*/)},
        {GL_FRAGMENT_SHADER, "<collide body fragment shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             /////////////////////////////////////////////////////////////////
             layout(location = 0) out vec4 collision_point;
             layout(location = 1) out vec3 collision_normal;

             layout(location = 0) uniform int frame_number;
             layout(location = 1) uniform isampler3D materials;
             layout(location = 2) uniform isampler3D body_materials;
             layout(location = 3) uniform vec3 body_x;
             layout(location = 4) uniform mat3 inv_body_axes;
             layout(location = 5) uniform vec3 body_x_cm;
             layout(location = 6) uniform vec3 bbl;
             layout(location = 7) uniform vec3 bbu;

             smooth in vec2 uv;

             const int chunk_size = 256;

             ivec4 voxelFetch(isampler3D tex, ivec3 coord)
             {
                 return texelFetch(materials, coord, 0);
             }

             void main()
             {
                 vec2 xy = mix(bbl.xy, bbu.xy, uv);

                 float min_z = bbl.z; //TODO: use fragment z coord, then scan until the body coord is outside
                 float max_z = bbu.z;

                 collision_point.xyz = vec3(0,0,0);
                 collision_point.a = 1.0;
                 collision_normal = vec3(0,0,0);

                 float scale = (1.0/chunk_size);

                 for(float z = min_z; z <= max_z; z += 1)
                 {
                     vec3 world_coord = vec3(xy.x, xy.y, z);
                     vec3 body_coord = inv_body_axes*(world_coord-body_x)+body_x_cm;
                     vec4 world_voxel = voxelFetch(materials, ivec3(world_coord));
                     float world_material = world_voxel.r;
                     float body_material = texelFetch(body_materials, ivec3(body_coord), 0).r;
                     if(world_material > 0 && body_material > 0)
                     {

                         vec3 gradient = vec3(
                             voxelFetch(materials, ivec3(world_coord+vec3(1,0,0))).g-voxelFetch(materials, ivec3(world_coord+vec3(-1,0,0))).g,
                             voxelFetch(materials, ivec3(world_coord+vec3(0,1,0))).g-voxelFetch(materials, ivec3(world_coord+vec3(0,-1,0))).g,
                             voxelFetch(materials, ivec3(world_coord+vec3(0,0,1))).g-voxelFetch(materials, ivec3(world_coord+vec3(0,0,-1))).g+0.001
                             );
                         gradient = normalize(gradient);
                         collision_point.xyz = world_coord;
                         collision_point.a = world_voxel.g;
                         collision_normal = gradient;
                         return;
                     }
                 }
             }
             /*/////////////////////////////////////////////////////////////*/)}
        ));

define_program(
    simulate_body,
    ( //shaders
        {GL_VERTEX_SHADER, "<simulate body vertex shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             /////////////////////////////////////////////////////////////////
             layout(location = 0) in vec3 x;

             layout(location = 0) uniform int layer;
             layout(location = 4) uniform int n_bodies;

             struct body
             {
                 int materials_origin_x; int materials_origin_y; int materials_origin_z;
                 int size_x; int size_y; int size_z;
                 float x_cm_x; float x_cm_y; float x_cm_z;
                 float x_x; float x_y; float x_z;
                 float x_dot_x; float x_dot_y; float x_dot_z;
                 float orientation_r; float orientation_x; float orientation_y; float orientation_z;
                 float omega_x; float omega_y; float omega_z;

                 float m;

                 float I_xx; float I_yx; float I_zx;
                 float I_xy; float I_yy; float I_zy;
                 float I_xz; float I_yz; float I_zz;

                 float invI_xx; float invI_yx; float invI_zx;
                 float invI_xy; float invI_yy; float invI_zy;
                 float invI_xz; float invI_yz; float invI_zz;
             };

             ivec3 body_materials_origin;
             ivec3 body_size;
             vec3  body_x_cm;
             vec3  body_x;
             vec3  body_x_dot;
             vec4  body_orientation;
             vec3  body_omega;

             layout(std430, binding = 0) buffer body_data
             {
                 body bodies[];
             };

             flat out int b;

             void main()
             {
                 b = gl_InstanceID;
                 body_materials_origin = ivec3(bodies[b].materials_origin_x,
                                               bodies[b].materials_origin_y,
                                               bodies[b].materials_origin_z);
                 body_size = ivec3(bodies[b].size_x,
                                   bodies[b].size_y,
                                   bodies[b].size_z);

                 float scale = 2.0/128.0;

                 gl_Position = vec4(-2,-2,0,1);

                 if(body_materials_origin.z <= layer && layer < body_materials_origin.z+body_size.z)
                 {
                     gl_Position.xy = scale*(body_materials_origin.xy+body_size.xy*x.xy)-1.0;
                 }
             }
             /*/////////////////////////////////////////////////////////////*/)},
        {GL_FRAGMENT_SHADER, "<simulate body fragment shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             /////////////////////////////////////////////////////////////////
             layout(location = 0) out ivec4 frag_color;

             layout(location = 0) uniform int layer;
             layout(location = 1) uniform int frame_number;
             layout(location = 2) uniform isampler3D materials;
             layout(location = 3) uniform isampler3D body_materials;

             struct body
             {
                 int materials_origin_x; int materials_origin_y; int materials_origin_z;
                 int size_x; int size_y; int size_z;
                 float x_cm_x; float x_cm_y; float x_cm_z;
                 float x_x; float x_y; float x_z;
                 float x_dot_x; float x_dot_y; float x_dot_z;
                 float orientation_r; float orientation_x; float orientation_y; float orientation_z;
                 float omega_x; float omega_y; float omega_z;

                 float m;

                 float I_xx; float I_yx; float I_zx;
                 float I_xy; float I_yy; float I_zy;
                 float I_xz; float I_yz; float I_zz;

                 float invI_xx; float invI_yx; float invI_zx;
                 float invI_xy; float invI_yy; float invI_zy;
                 float invI_xz; float invI_yz; float invI_zz;

             };

             ivec3 body_materials_origin;
             ivec3 body_size;
             vec3  body_x_cm;
             vec3  body_x;
             vec3  body_x_dot;
             vec4  body_orientation;
             vec3  body_omega;

             layout(std430, binding = 0) buffer body_data
             {
                 body bodies[];
             };

             flat in int b;

             vec4 qmult(vec4 a, vec4 b)
             {
                 return vec4(a.x*b.x-a.y*b.y-a.z*b.z-a.w*b.w,
                             a.x*b.y+a.y*b.x+a.z*b.w-a.w*b.z,
                             a.x*b.z+a.z*b.x+a.w*b.y-a.y*b.w,
                             a.x*b.w+a.w*b.x+a.y*b.z-a.z*b.y);
             }

             vec3 apply_rotation(vec4 q, vec3 p)
             {
                 vec4 p_quat = vec4(0, p.x, p.y, p.z);
                 vec4 q_out = qmult(qmult(q, p_quat), vec4(q.x, -q.y, -q.z, -q.w));
                 return q_out.yzw;
             }

             uint rand(uint seed)
             {
                 seed ^= seed<<13;
                 seed ^= seed>>17;
                 seed ^= seed<<5;
                 return seed;
             }

             float float_noise(uint seed)
             {
                 return float(int(seed))/1.0e10;
             }

             void main()
             {
                 ivec3 pos;
                 pos.xy = ivec2(gl_FragCoord.xy);
                 pos.z = layer;

                 //+,0,-,0
                 //0,+,0,-
                 // int rot = (frame_number+layer)%4;
                 uint rot = rand(rand(rand(frame_number)))%4;
                 int i = 0;
                 ivec2 dir = ivec2(((rot&1)*(2-rot)), (1-(rot&1))*(1-rot));

                 ivec4 c  = texelFetch(body_materials, ivec3(pos.x, pos.y, pos.z), 0);
                 ivec4 u  = texelFetch(body_materials, ivec3(pos.x, pos.y, pos.z+1), 0);
                 ivec4 d  = texelFetch(body_materials, ivec3(pos.x, pos.y, pos.z-1), 0);
                 ivec4 r  = texelFetch(body_materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z), 0);
                 ivec4 l  = texelFetch(body_materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z), 0);
                 ivec4 f  = texelFetch(body_materials, ivec3(pos.x-dir.y, pos.y+dir.x, pos.z), 0);
                 ivec4 ba  = texelFetch(body_materials, ivec3(pos.x+dir.y, pos.y-dir.x, pos.z), 0);

                 // c.r = 1;
                 frag_color = c;

                 const int max_depth = 16;
                 if(c.r > 0)
                 {
                     if(l.r == 0 ||
                        r.r == 0 ||
                        u.r == 0 ||
                        d.r == 0 ||
                        f.r == 0 ||
                        ba.r == 0) frag_color.g = 0;
                     else
                     {
                         frag_color.g = -max_depth;
                         frag_color.g = max(frag_color.g, l.g-1);
                         frag_color.g = max(frag_color.g, r.g-1);
                         frag_color.g = max(frag_color.g, u.g-1);
                         frag_color.g = max(frag_color.g, d.g-1);
                         frag_color.g = max(frag_color.g, f.g-1);
                         frag_color.g = max(frag_color.g, ba.g-1);
                     }

                     frag_color.b = c.b/2;
                     if(frag_color.r == 3) frag_color.b = 1000;
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
                     frag_color.g = min(frag_color.g, ba.g+1);
                 }
                 frag_color.g = clamp(frag_color.g,-max_depth,max_depth);

                 frag_color.b = 1000;
             }
             /*/////////////////////////////////////////////////////////////*/)}
        ));

define_program(
    simulate_body_physics,
    ( //shaders
        {GL_VERTEX_SHADER, "<simulate body physics vertex shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             /////////////////////////////////////////////////////////////////
             layout(location = 0) in vec3 x;

             smooth out vec2 uv;

             void main()
             {
                 gl_Position.xyz = x;
                 gl_Position.w = 1.0;
                 uv = 0.5*x.xy+0.5;
             }
             /*/////////////////////////////////////////////////////////////*/)},
        {GL_FRAGMENT_SHADER, "<simulate body physics fragment shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             /////////////////////////////////////////////////////////////////
             layout(location = 0) out vec3 deltax_dot;
             layout(location = 1) out vec3 deltaomega;
             layout(location = 2) out vec3 pseudo_x_dot;
             layout(location = 3) out vec3 pseudo_omega;

             layout(location = 0) uniform int frame_number;
             layout(location = 1) uniform isampler3D materials;
             layout(location = 2) uniform isampler3D body_materials;
             layout(location = 3) uniform sampler3D body_forces;
             layout(location = 4) uniform sampler3D body_shifts;

             struct body
             {
                 int materials_origin_x; int materials_origin_y; int materials_origin_z;
                 int size_x; int size_y; int size_z;
                 float x_cm_x; float x_cm_y; float x_cm_z;
                 float x_x; float x_y; float x_z;
                 float x_dot_x; float x_dot_y; float x_dot_z;
                 float orientation_r; float orientation_x; float orientation_y; float orientation_z;
                 float omega_x; float omega_y; float omega_z;

                 float m;

                 float I_xx; float I_yx; float I_zx;
                 float I_xy; float I_yy; float I_zy;
                 float I_xz; float I_yz; float I_zz;

                 float invI_xx; float invI_yx; float invI_zx;
                 float invI_xy; float invI_yy; float invI_zy;
                 float invI_xz; float invI_yz; float invI_zz;

                 // ivec3 materials_origin;
                 // ivec3 size;
                 // vec3 x_cm;
                 // vec3 x;
                 // vec3 x_dot;
                 // vec4 orientation;
                 // vec3 omega;
             };

             ivec3 body_materials_origin;
             ivec3 body_size;
             vec3  body_x_cm;
             vec3  body_x;
             vec3  body_x_dot;
             vec4  body_orientation;
             vec3  body_omega;

             layout(std430, binding = 0) buffer body_data
             {
                 body bodies[];
             };

             smooth in vec2 uv;

             const int chunk_size = 256;

             ivec4 voxelFetch(isampler3D tex, ivec3 coord)
             {
                 return texelFetch(materials, coord, 0);
             }

             ivec4 bodyVoxelFetch(int body_index, ivec3 coord)
             {
                 return texelFetch(body_materials, body_materials_origin+coord, 0);
             }

             vec4 axis_to_quaternion(vec3 axis)
             {
                 float half_angle = length(axis)/2;
                 if(half_angle <= 0.0001) return vec4(1,0,0,0);
                 vec3 axis_hat = normalize(axis);
                 float s = sin(half_angle);
                 float c = cos(half_angle);
                 return vec4(c, s*axis_hat.x, s*axis_hat.y, s*axis_hat.z);
             }

             vec4 qmult(vec4 a, vec4 b)
             {
                 return vec4(a.x*b.x-a.y*b.y-a.z*b.z-a.w*b.w,
                             a.x*b.y+a.y*b.x+a.z*b.w-a.w*b.z,
                             a.x*b.z+a.z*b.x+a.w*b.y-a.y*b.w,
                             a.x*b.w+a.w*b.x+a.y*b.z-a.z*b.y);
             }

             vec3 apply_rotation(vec4 q, vec3 p)
             {
                 vec4 p_quat = vec4(0, p.x, p.y, p.z);
                 vec4 q_out = qmult(qmult(q, p_quat), vec4(q.x, -q.y, -q.z, -q.w));
                 return q_out.yzw;
             }

             uint rand(uint seed)
             {
                 seed ^= seed<<13;
                 seed ^= seed>>17;
                 seed ^= seed<<5;
                 return seed;
             }

             float float_noise(uint seed)
             {
                 return float(int(seed))/1.0e10;
             }

             void main()
             {
                 int b = int(gl_FragCoord.y);
                 // int b = 0;

                 body_materials_origin = ivec3(bodies[b].materials_origin_x,
                                               bodies[b].materials_origin_y,
                                               bodies[b].materials_origin_z);
                 body_size = ivec3(bodies[b].size_x,
                                   bodies[b].size_y,
                                   bodies[b].size_z);

                 body_x_cm = vec3(bodies[b].x_cm_x, bodies[b].x_cm_y, bodies[b].x_cm_z);
                 body_x = vec3(bodies[b].x_x, bodies[b].x_y, bodies[b].x_z);
                 body_x_dot = vec3(bodies[b].x_dot_x, bodies[b].x_dot_y, bodies[b].x_dot_z);
                 body_orientation = vec4(bodies[b].orientation_r, bodies[b].orientation_x, bodies[b].orientation_y, bodies[b].orientation_z);
                 body_omega = vec3(bodies[b].omega_x, bodies[b].omega_y, bodies[b].omega_z);

                 float m = bodies[b].m;
                 mat3 I = mat3(bodies[b].I_xx, bodies[b].I_yx, bodies[b].I_zx,
                               bodies[b].I_xy, bodies[b].I_yy, bodies[b].I_zy,
                               bodies[b].I_xz, bodies[b].I_yz, bodies[b].I_zz);
                 mat3 invI = mat3(bodies[b].invI_xx, bodies[b].invI_yx, bodies[b].invI_zx,
                                  bodies[b].invI_xy, bodies[b].invI_yy, bodies[b].invI_zy,
                                  bodies[b].invI_xz, bodies[b].invI_yz, bodies[b].invI_zz);

                 deltax_dot = vec3(0,0,0);
                 deltaomega = vec3(0,0,0);

                 pseudo_x_dot = vec3(0,0,0);
                 pseudo_omega = vec3(0,0,0);

                 for(int i = 0; i < 1; i++)
                 {
                     vec3 x_dot = body_x_dot+deltax_dot;
                     vec3 omega = body_omega+deltaomega;
                     vec3 x = body_x;
                     vec4 orientation = body_orientation;
                     // vec3 x = body_x+deltax_dot;
                     // vec4 orientation = qmult(axis_to_quaternion(deltaomega), body_orientation);
                     // orientation = normalize(orientation);

                     const int n_test_points = 5;
                     const int max_steps = 20;

                     int deepest_x;
                     int deepest_y;
                     int deepest_z;
                     int deepest_depth = 0;
                     bool point_found = false;

                     // float old_E = m*dot(x_dot, x_dot)+I*dot(omega, omega);

                     vec3 best_deltax_dot = vec3(0,0,0);
                     vec3 best_deltax = vec3(0,0,0);
                     vec3 best_r = vec3(0,0,0);
                     int n_collisions = 0;
                     float best_E = 2*(m*dot(x_dot, x_dot)+dot(I*omega, omega));
                     //TODO: build list of points to test that are non-empty and on the surface instead of checking all of them
                     //      maybe make it a linked list to minimize texture lookups
                     for(int test_z = 0; test_z < body_size.z; test_z+=1)
                         for(int test_y = 0; test_y < body_size.y; test_y+=1)
                             for(int test_x = 0; test_x < body_size.x; test_x+=1)
                             {

                                 ivec3 body_coord = ivec3(test_x, test_y, test_z);
                                 vec3 world_coord = apply_rotation(orientation, vec3(body_coord)+0.5-body_x_cm)+x;

                                 vec4 body_voxel = texelFetch(body_materials, body_materials_origin+body_coord, 0);

                                 if(body_voxel.g == 0 && body_voxel.r > 0)
                                 {
                                     for(int wz = 0; wz <= 1; wz++)
                                         for(int wy = 0; wy <= 1; wy++)
                                             for(int wx = 0; wx <= 1; wx++)
                                             {
                                                 ivec3 wvc = ivec3(world_coord-0.5)+ivec3(wx,wy,wz); //world_voxel_coord
                                                 ivec4 world_voxel = voxelFetch(materials, wvc);
                                                 vec3 rel_pos = vec3(wvc)+0.5-world_coord;
                                                 if(world_voxel.r > 0 && dot(rel_pos, rel_pos) <= 1.0)
                                                 {
                                                     const float COR = 0.1;

                                                     vec3 world_gradient = vec3(
                                                         voxelFetch(materials, wvc+ivec3(1,0,0)).g-voxelFetch(materials, wvc+ivec3(-1,0,0)).g,
                                                         voxelFetch(materials, wvc+ivec3(0,1,0)).g-voxelFetch(materials, wvc+ivec3(0,-1,0)).g,
                                                         voxelFetch(materials, wvc+ivec3(0,0,1)).g-voxelFetch(materials, wvc+ivec3(0,0,-1)).g+0.001
                                                         );

                                                     vec3 normal = normalize(world_gradient);
                                                     vec3 a = wvc-x;

                                                     float u = dot(x_dot+cross(omega, a), normal);
                                                     float K = 1.0+m*dot(cross(invI*cross(a, normal), a), normal);
                                                     vec3 test_deltax_dot = vec3(0,0,0);
                                                     if(u < 0)
                                                     {
                                                         //normal force
                                                         test_deltax_dot = (-(1.0+COR)*u/K)*normal;
                                                         //friction
                                                         test_deltax_dot += -0.1*(-(1.0+COR)*u/K)*(x_dot+cross(omega, a)-u*normal);
                                                     }
                                                     vec3 v = x_dot+cross(omega, a);

                                                     vec3 test_deltaomega = m*invI*cross(a, test_deltax_dot);
                                                     float E = (m*dot(x_dot+test_deltax_dot, x_dot+test_deltax_dot)
                                                                +dot(I*(omega+test_deltaomega), omega+test_deltaomega)); //TODO: need to adjust I as rotation changes
                                                     if(E < best_E)
                                                     {
                                                         best_deltax_dot = test_deltax_dot;
                                                         // best_deltax = deltax;
                                                         best_r = a;
                                                         best_E = E;
                                                     }

                                                     vec3 deltax = -0.005*world_voxel.g*normal;
                                                     pseudo_x_dot += deltax;
                                                     pseudo_omega += m*invI*cross(best_r, deltax);
                                                 }
                                             }
                                 }
                             }

                     deltax_dot += best_deltax_dot;
                     deltaomega += m*invI*cross(best_r, best_deltax_dot);
                     // pseudo_x_dot = best_deltax;
                     // pseudo_omega = (m/I)*cross(best_r, best_deltax);
                 }
             }
             /*/////////////////////////////////////////////////////////////*/)}
        ));
