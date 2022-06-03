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
             layout(location = 3) uniform isampler3D chunk_lookup;
             layout(location = 4) uniform ivec3 size;

             smooth in vec2 screen_pos;

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
                 ivec3 chunk_pos = coord/chunk_size;
                 ivec3 offset = coord%chunk_size;
                 ivec3 chunk_origin;
                 chunk_origin.xy = chunk_size*texelFetch(chunk_lookup, chunk_pos, 0).xy;
                 chunk_origin.z = 0;
                 return texelFetch(materials, chunk_origin+offset, 0);
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

                 vec3 pos = camera_pos;
                 vec3 sign = vec3(ray_dir.x > 0 ? 1: -1,
                                  ray_dir.y > 0 ? 1: -1,
                                  ray_dir.z > 0 ? 1: -1);

                 int hit_dir = 0;

                 float epsilon = 0.02;
                 int max_iterations = 200;
                 int i = 0;
                 float total_dist = 0;
                 vec3 invabs_ray_dir = sign/ray_dir;
                 // if(abs(ray_dir.x) <= 0.002) {frag_color.rgba = vec4(1,0,0,1); return;};
                 // if(abs(ray_dir.y) <= 0.002) {frag_color.rgba = vec4(1,0,0,1); return;};
                 // if(abs(ray_dir.z) <= 0.002) {frag_color.rgba = vec4(1,0,0,1); return;};
                 float xy_scale = (1.0/chunk_size);
                 vec3 scale = vec3(xy_scale, xy_scale, xy_scale);

                 if(pos.x < 0 && ray_dir.x > 0)      total_dist = max(total_dist, epsilon+(-pos.x)/(ray_dir.x));
                 if(pos.x > size.x && ray_dir.x < 0) total_dist = max(total_dist, epsilon+(size.x-pos.x)/(ray_dir.x));
                 if(pos.y < 0 && ray_dir.y > 0)      total_dist = max(total_dist, epsilon+(-pos.y)/(ray_dir.y));
                 if(pos.y > size.y && ray_dir.y < 0) total_dist = max(total_dist, epsilon+(size.y-pos.y)/(ray_dir.y));
                 if(pos.z < 0 && ray_dir.z > 0)      total_dist = max(total_dist, epsilon+(-pos.z)/(ray_dir.z));
                 if(pos.z > size.z && ray_dir.z < 0) total_dist = max(total_dist, epsilon+(size.z-pos.z)/(ray_dir.z));

                 pos += total_dist*ray_dir;

                 {
                     vec3 dist = mod(-pos*sign, size)*invabs_ray_dir;
                     float min_dist = dist.x;
                     int min_dir = 0;
                     if(dist.y < min_dist) {
                         min_dist = dist.y;
                         min_dir = 1;
                     }
                     if(dist.z < min_dist) {
                         min_dist = dist.z;
                         min_dir = 2;
                     }
                     ivec3 max_displacement = ivec3(abs(min_dist*ray_dir));
                     max_iterations = int(max_displacement.x+max_displacement.y+max_displacement.z)+4;
                 }
                 // max_iterations = min(max_iterations, 100);

                 for(;;)
                 {
                     if(pos.x < 0 || pos.y < 0 || pos.z < 0
                        || pos.x > size.x || pos.y > size.y || pos.z > size.z)
                     {
                         discard;
                         return;
                     }
                     ivec4 voxel = voxelFetch(materials, ivec3(pos));
                     if(voxel.r != 0) break;
                     if(voxel.g >= 5)
                     {
                         //TODO: tune this, optimal distance isn't constant, it should depend on direction
                         //add some noise so the visual errors don't look terrible
                         float skip_dist = (voxel.g*0.5)-1;
                         pos += skip_dist*ray_dir;
                         total_dist += skip_dist;
                     }

                     vec3 dist = mod(-pos*sign, 1.0)*invabs_ray_dir;
                     float min_dist = dist.x;
                     int min_dir = 0;
                     if(dist.y < min_dist) {
                         min_dist = dist.y;
                         min_dir = 1;
                     }
                     if(dist.z < min_dist) {
                         min_dist = dist.z;
                         min_dir = 2;
                     }

                     float min_dist_with_epsilon = min_dist+epsilon;
                     pos += min_dist_with_epsilon*ray_dir;
                     total_dist += min_dist_with_epsilon;
                     hit_dir = min_dir;

                     if(++i >= max_iterations)
                     {
                         // frag_color = vec4(0.4, 0.2, 0.2, 1.0)*(min_dir+1.0);
                         // frag_color.a = 1.0;
                         discard;
                         return;
                     }
                 }

                 frag_color = vec4(0.0, 0.0, 0.0, 1.0);
                 // float brightness = clamp(1.0-2.0*total_dist/chunk_size, 0, 1);
                 float brightness = 1.0;
                 vec3 dist_from_center = 2.0*abs(mod(pos, 1.0)-0.5);
                 // brightness *= 0.1
                 //     +pow((1.0-dist_from_center.x)*(1.0-dist_from_center.y), 0.5)
                 //     +pow((1.0-dist_from_center.y)*(1.0-dist_from_center.z), 0.5)
                 //     +pow((1.0-dist_from_center.z)*(1.0-dist_from_center.x), 0.5);
                 // brightness *= 1.5;
                 // if(hit_dir == 0) frag_color.r = 1;
                 // if(hit_dir == 1) frag_color.g = 1;
                 // if(hit_dir == 2) frag_color.b = 1;

                 vec3 gradient = vec3(
                     voxelFetch(materials, ivec3(pos+vec3(1,0,0))).g-voxelFetch(materials, ivec3(pos+vec3(-1,0,0))).g,
                     voxelFetch(materials, ivec3(pos+vec3(0,1,0))).g-voxelFetch(materials, ivec3(pos+vec3(0,-1,0))).g,
                     voxelFetch(materials, ivec3(pos+vec3(0,0,1))).g-voxelFetch(materials, ivec3(pos+vec3(0,0,-1))).g+0.001
                     );
                 gradient = normalize(gradient);
                 frag_color.rgb = 0.5*gradient+vec3(0.5,0.5,0.5);

                 if(voxelFetch(materials, ivec3(pos)).r > 1)
                 {
                     frag_color.rgb *= 0.25;
                     frag_color.rgb += vec3(0.1,0.1,0.2);
                 }
                 else
                 {
                     frag_color.rgb *= 0.1;
                     frag_color.rgb += vec3(0.54,0.44,0.21);
                 }
                 frag_color.rgb *= brightness;

                 // float x = texture(materials, (pos - 2*epsilon*ray_dir)*scale).g;
                 // float y = texture(materials, (pos - 2*epsilon*ray_dir)*scale).a/sqrt(71.0);
                 // float light_brightness = (x*x);
                 // light_brightness *= 0.00000001;

                 float light_brightness = voxelFetch(materials, ivec3(pos - 2*epsilon*ray_dir)).b;
                 light_brightness *= 0.0025;

                 frag_color.rgb *= light_brightness;
                 // frag_color.rgb *= clamp(1.0-1.0*total_dist/chunk_size, 0, 1);
                 frag_color.a = 1.0;
                 gl_FragDepth = 1.0-total_dist/(2*chunk_size*sqrt(3));
             }
             /*/////////////////////////////////////////////////////////////*/)}
        ));

define_program(
    simulate_chunk,
    ( //shaders
        {GL_VERTEX_SHADER, "<simulate chunk vertex shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             /////////////////////////////////////////////////////////////////
             layout(location = 0) in vec3 x;

             void main()
             {
                 gl_Position.xyz = x;
                 gl_Position.w = 1.0;
             }
             /*/////////////////////////////////////////////////////////////*/)},
        {GL_GEOMETRY_SHADER, "<simulate chunk geometry shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             /////////////////////////////////////////////////////////////////
             layout(points) in;

             layout(location = 0) uniform int layer;
             layout(location = 3) uniform isampler3D chunk_lookup;
             layout(location = 4) uniform usampler3D active_regions_in;

             layout(triangle_strip, max_vertices = 128) out;

             out vec3 p;

             const int chunk_size = 256;

             void main()
             {
                 float scale = 1.0/(2.0*16.0);
                 int z = int(gl_in[0].gl_Position.z)+layer/16;
                 int y = int(gl_in[0].gl_Position.y);
                 int x = int(gl_in[0].gl_Position.x);

                 uint region_bits = texelFetch(active_regions_in, ivec3(x, y, z), 0).r;
                 ivec3 chunk_pos = ivec3(x,y,z)/16;
                 ivec3 CO; //chunk origin in memory space
                 CO.xy = 16*texelFetch(chunk_lookup, chunk_pos, 0).xy;
                 CO.z = 0;
                 if(region_bits != 0)
                 {
                     p=16*vec3(x,y,z);
                     gl_Position=vec4(scale*(CO.x+(x%16)+0)-1,scale*(CO.y+(y%16)+0)-1,0,1);EmitVertex();
                     gl_Position=vec4(scale*(CO.x+(x%16)+0)-1,scale*(CO.y+(y%16)+1)-1,0,1);EmitVertex();
                     gl_Position=vec4(scale*(CO.x+(x%16)+1)-1,scale*(CO.y+(y%16)+0)-1,0,1);EmitVertex();
                     gl_Position=vec4(scale*(CO.x+(x%16)+1)-1,scale*(CO.y+(y%16)+1)-1,0,1);EmitVertex();
                     EndPrimitive();
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
             layout(location = 3) uniform isampler3D chunk_lookup;
             layout(location = 5) uniform writeonly uimage3D active_regions_out;

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
                 return float(int(seed))/1.0e10;
             }

             float brightness_curve(float x)
             {
                 return pow(x, 2.0);
             }

             float inverse_brightness_curve(float x)
             {
                 return pow(x, 0.5);
             }

             const int chunk_size = 256;

             ivec4 voxelFetch(isampler3D tex, ivec3 coord)
             {
                 ivec3 chunk_pos = coord/chunk_size;
                 ivec3 offset = coord%chunk_size;
                 ivec3 chunk_origin;
                 chunk_origin.xy = chunk_size*texelFetch(chunk_lookup, chunk_pos, 0).xy;
                 chunk_origin.z = 0;
                 return texelFetch(materials, chunk_origin+offset, 0);
             }

             void main()
             {
                 float scale = 1.0/chunk_size;
                 // ivec2 pos = ivec2(chunk_size*uv);
                 // ivec2 pos = ivec2(gl_FragCoord.xy);
                 ivec3 pos = ivec3(p);
                 pos.xy += ivec2(gl_FragCoord.xy)%16;
                 pos.z += layer%16;

                 //+,0,-,0
                 //0,+,0,-
                 // int rot = (frame_number+layer)%4;
                 uint rot = rand(rand(rand(frame_number)))%4;
                 int i = 0;
                 ivec2 dir = ivec2(((rot&1)*(2-rot)), (1-(rot&1))*(1-rot));

                 ivec4 m  = voxelFetch(materials, ivec3(pos.x, pos.y, pos.z));
                 ivec4 u  = voxelFetch(materials, ivec3(pos.x, pos.y, pos.z+1));
                 ivec4 d  = voxelFetch(materials, ivec3(pos.x, pos.y, pos.z-1));
                 ivec4 r  = voxelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z));
                 ivec4 l  = voxelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z));
                 ivec4 dr = voxelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z-1));
                 ivec4 ur = voxelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z+1));

                 ivec4 f  = voxelFetch(materials, ivec3(pos.x-dir.y, pos.y+dir.x, pos.z));
                 ivec4 b  = voxelFetch(materials, ivec3(pos.x+dir.y, pos.y-dir.x, pos.z));

                 ivec4 ul = voxelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z+1));
                 ivec4 ll = voxelFetch(materials, ivec3(pos.x-2*dir.x, pos.y-2*dir.y, pos.z));
                 ivec4 dl = voxelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z-1));

                 // ivec4 m  = texelFetch(materials, ivec3(pos.x, pos.y, pos.z),0);
                 // ivec4 u  = texelFetch(materials, ivec3(pos.x, pos.y, pos.z+1),0);
                 // ivec4 d  = texelFetch(materials, ivec3(pos.x, pos.y, pos.z-1),0);
                 // ivec4 r  = texelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z),0);
                 // ivec4 l  = texelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z),0);
                 // ivec4 dr = texelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z-1),0);
                 // ivec4 ur = texelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, pos.z+1),0);

                 // ivec4 f  = texelFetch(materials, ivec3(pos.x-dir.y, pos.y+dir.x, pos.z),0);
                 // ivec4 b  = texelFetch(materials, ivec3(pos.x+dir.y, pos.y-dir.x, pos.z),0);

                 // ivec4 ul = texelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z+1),0);
                 // ivec4 ll = texelFetch(materials, ivec3(pos.x-2*dir.x, pos.y-2*dir.y, pos.z),0);
                 // ivec4 dl = texelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, pos.z-1),0);

                 frag_color = m;
                 if(m.r == 0)
                 {
                     if(u.r > 0 && u.r != 3) frag_color = u;
                     else if(l.r > 0 && ul.r > 0 && u.r != 3 && ul.r != 3) frag_color = ul;
                     else if(dl.r > 0 && d.r > 0 && l.r > 1 && ll.r > 0 && u.r != 3 && ul.r != 3 && l.r != 3) frag_color = l;

                     // if(u.r == 0 && ul.r == 0 && dl.r > 0 && d.r > 0 && l.r > 0) frag_color = l;
                     // else if(u.r == 0 && l.r > 0) frag_color = ul;
                     // else if(u.r > 0) frag_color = u;
                 }
                 else if(m.r != 3)
                 {
                     bool fall_allowed = (pos.z > 0 && (d.r == 0 || (dr.r == 0 && r.r == 0)));
                     bool flow_allowed = (pos.z > 0 && r.r == 0 && ur.r == 0 && u.r == 0 && m.r > 1 && l.r > 0);
                     if(fall_allowed || flow_allowed) frag_color.r = 0;
                 }

                 const int max_depth = 16;
                 if(m.r > 0)
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

                     frag_color.b = m.b/2;
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
                     frag_color.g = min(frag_color.g, b.g+1);

                     // frag_color.b = 1+texelFetch(materials, ivec3(pos.x, pos.y, layer), 0).b;
                     float brightness = 100.0;
                     brightness += brightness_curve(l.b);
                     brightness += brightness_curve(r.b);
                     brightness += brightness_curve(u.b);
                     brightness += brightness_curve(d.b);
                     brightness += brightness_curve(f.b);
                     brightness += brightness_curve(b.b);
                     brightness *= 1.0/6.0;
                     frag_color.b = int(inverse_brightness_curve(brightness));
                 }
                 frag_color.g = clamp(frag_color.g,-max_depth,max_depth);

                 // bool changed = m.r != frag_color.r || m.g != frag_color.g || m.b != frag_color.b;
                 bool changed = m.r != frag_color.r || m.g != frag_color.g;
                 if(changed)
                 {
                     imageStore(active_regions_out, ivec3(ivec3(pos.x/16, pos.y/16, pos.z/16  )), uvec4(1,0,0,0));
                     imageStore(active_regions_out, ivec3(ivec3(pos.x/16+1, pos.y/16, pos.z/16)), uvec4(1,0,0,0));
                     imageStore(active_regions_out, ivec3(ivec3(pos.x/16-1, pos.y/16, pos.z/16)), uvec4(1,0,0,0));
                     imageStore(active_regions_out, ivec3(ivec3(pos.x/16, pos.y/16+1, pos.z/16)), uvec4(1,0,0,0));
                     imageStore(active_regions_out, ivec3(ivec3(pos.x/16, pos.y/16-1, pos.z/16)), uvec4(1,0,0,0));
                     imageStore(active_regions_out, ivec3(ivec3(pos.x/16, pos.y/16, pos.z/16+1)), uvec4(1,0,0,0));
                     imageStore(active_regions_out, ivec3(ivec3(pos.x/16, pos.y/16, pos.z/16-1)), uvec4(1,0,0,0));
                 }
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
             layout(location = 2) uniform isampler3D chunk_lookup;
             layout(location = 3) uniform sampler2D old_x;
             layout(location = 4) uniform sampler2D old_x_dot;

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
                 ivec3 chunk_pos = coord/chunk_size;
                 ivec3 offset = ivec3(mod(coord, chunk_size));
                 ivec3 chunk_origin;
                 chunk_origin.xy = chunk_size*texelFetch(chunk_lookup, chunk_pos, 0).xy;
                 chunk_origin.z = 0;
                 return texelFetch(materials, chunk_origin+offset, 0);
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
                     int max_iterations = 100;
                     int i = 0;
                     while(voxelFetch(materials, ivec3(x)).r == 1 || voxelFetch(materials, ivec3(x)).r == 3)
                     {
                         vec3 gradient = vec3(
                             voxelFetch(materials, ivec3(x+vec3(1,0,0))).g-voxelFetch(materials, ivec3(x+vec3(-1,0,0))).g,
                             voxelFetch(materials, ivec3(x+vec3(0,1,0))).g-voxelFetch(materials, ivec3(x+vec3(0,-1,0))).g,
                             voxelFetch(materials, ivec3(x+vec3(0,0,1))).g-voxelFetch(materials, ivec3(x+vec3(0,0,-1))).g+0.001
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
             layout(location = 2) uniform isampler3D chunk_lookup;
             layout(location = 3) uniform isampler3D body_materials;
             layout(location = 4) uniform vec3 body_x;
             layout(location = 5) uniform mat3 inv_body_axes;
             layout(location = 6) uniform vec3 body_x_cm;
             layout(location = 7) uniform vec3 bbl;
             layout(location = 8) uniform vec3 bbu;

             smooth in vec2 uv;

             const int chunk_size = 256;

             ivec4 voxelFetch(isampler3D tex, ivec3 coord)
             {
                 ivec3 chunk_pos = coord/chunk_size;
                 ivec3 offset = coord%chunk_size;
                 ivec3 chunk_origin;
                 chunk_origin.xy = chunk_size*texelFetch(chunk_lookup, chunk_pos, 0).xy;
                 chunk_origin.z = 0;
                 return texelFetch(materials, chunk_origin+offset, 0);
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
    simulate_body_physics,
    ( //shaders
        {GL_VERTEX_SHADER, "<simulate body physics vertex shader>",
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
        {GL_FRAGMENT_SHADER, "<simulate body physics fragment shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             /////////////////////////////////////////////////////////////////
             layout(location = 0) out vec3 x_dot;
             layout(location = 1) out vec3 omega;

             layout(location = 0) uniform int frame_number;
             layout(location = 1) uniform isampler3D materials;
             layout(location = 2) uniform isampler3D chunk_lookup;
             layout(location = 3) uniform isampler3D body_materials;

             struct body
             {
                 ivec3 materials_origin;
                 ivec3 size;
                 vec3 x_cm
                 vec3 x;
                 vec3 x_dot;
                 vec4 orientation;
                 vec3 omega;
             };

             layout(binding = 0) buffer body_data
             {
                 body bodies[];
             };

             // layout(location = 4) uniform isampler2D body_materials_origin;
             // layout(location = 5) uniform isampler2D body_size;
             // layout(location = 6) uniform isampler2D body_x_cm;
             // layout(location = 7) uniform isampler2D old_x;
             // layout(location = 8) uniform isampler2D old_x_dot;
             // layout(location = 9) uniform isampler2D old_orientation;
             // layout(location =10) uniform isampler2D old_omega;

             smooth in vec2 uv;

             const int chunk_size = 256;

             ivec4 voxelFetch(isampler3D tex, ivec3 coord)
             {
                 ivec3 chunk_pos = coord/chunk_size;
                 ivec3 offset = coord%chunk_size;
                 ivec3 chunk_origin;
                 chunk_origin.xy = chunk_size*texelFetch(chunk_lookup, chunk_pos, 0).xy;
                 chunk_origin.z = 0;
                 return texelFetch(materials, chunk_origin+offset, 0);
             }

             ivec4 bodyVoxelFetch(int body_index, ivec3 coord)
             {
                 return texelFetch(texelFetch(body_materials_lbound, ivec2(body_index, 0), 0)+coord, 0);
             }

             vec4 axis_to_quaternion(vec3 axis)
             {
                 float half_angle = len(axis)/2;
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
                 quaternion p_quat = vec4(0, p.x, p.y, p.z);
                 quaternion out = qmult(qmult(q, p_quat), vec4(q.x, -q.y, -q.z, -q.w));
                 return vec3(out.y, out.z, out.w);
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
                 int b = glFragCoord.x;
                 x_dot = bodies[b].x_dot;
                 vec3 x = bodies[b].x+x_dot;
                 omega = bodies[b].omega;
                 vec4 orientation = qmult(axis_to_quaternion(omega), bodies[b].orientation);

                 mat3 body_axes;
                 body_axes[0] = apply_rotation(orientation, vec3(1,0,0));
                 body_axes[1] = apply_rotation(orientation, vec3(0,1,0));
                 body_axes[2] = apply_rotation(orientation, vec3(0,0,1));
                 mat3 inv_body_axes = transpose(body_axes);

                 const int n_test_points = 5;
                 const int max_steps = 50;

                 const float I = 100;
                 const float m = 1;
                 const float COR = 0.1;

                 for(int i = 0; i < n_test_points; i++)
                 {
                     ivec3 body_coord = ivec3(rand((n_test_points*frame_number+i)*3+0)
                                              rand((n_test_points*frame_number+i)*3+1)
                                              rand((n_test_points*frame_number+i)*3+2))*body_size;
                     ivec2 step_scale = body_size/2;
                     for(int j = 0; j < max_steps; j++)
                     {
                         voxelFetch(body_materials, body_materials_origin+body_coord);
                         vec3 world_coord = body_axes*(body_coord-bodies[b].x_cm)+x;
                         vec4 world_voxel = voxelFetch(materials, ivec3(world_coord));
                         float world_material = world_voxel.r;
                         float body_voxel = bodyVoxelFetch(gl_FragCoord.x, body_coord).r;

                         vec3 world_gradient = vec3(
                             voxelFetch(materials, ivec3(world_coord+vec3(1,0,0))).g-voxelFetch(materials, ivec3(world_coord+vec3(-1,0,0))).g,
                             voxelFetch(materials, ivec3(world_coord+vec3(0,1,0))).g-voxelFetch(materials, ivec3(world_coord+vec3(0,-1,0))).g,
                             voxelFetch(materials, ivec3(world_coord+vec3(0,0,1))).g-voxelFetch(materials, ivec3(world_coord+vec3(0,0,-1))).g+0.001
                             );
                         world_gradient = inv_body_axes*world_gradient;

                         if(body_voxel.r > 0 && world_voxel.r > 0)
                         {
                             //resolve collision
                             normal = normalize(world_gradient);
                             vec3 r = body_coord-bodies[b].x_cm;
                             vec3 u = dot(x_dot+cross(omega, r), normal);
                             real K = 1.0+m*(dot(r,r)-sq(dot(r, normal)))/I; //TODO: full intertia tensor
                             x_dot += (-(1.0+COR)*u/K)*normal;
                             omega += (m/I)*cross(r, x_dot);
                             //TODO: add pseudo velocities for position correction
                             return;
                         }

                         vec3 body_gradient = vec3(
                             voxelFetch(gl_FragCoord.x, ivec3(body_coord+vec3(1,0,0))).g-voxelFetch(gl_FragCoord.x, ivec3(body_coord+vec3(-1,0,0))).g,
                             voxelFetch(gl_FragCoord.x, ivec3(body_coord+vec3(0,1,0))).g-voxelFetch(gl_FragCoord.x, ivec3(body_coord+vec3(0,-1,0))).g,
                             voxelFetch(gl_FragCoord.x, ivec3(body_coord+vec3(0,0,1))).g-voxelFetch(gl_FragCoord.x, ivec3(body_coord+vec3(0,0,-1))).g
                             );
                         //want to move along the world gradient constrained to the surface of the body
                         total_gradient = body_gradient+world_gradient;
                         float proj = dot(world_gradient, body_gradient);
                         if(proj != 0) total_gradient -= proj*normalize(body_gradient);
                         body_coord -= total_gradient*jump_scale;
                         step_scale = max(step_scale/2, 1);
                     }
                 }
             }
             /*/////////////////////////////////////////////////////////////*/)}
        ));

// define_program(
//     update_active_regions,
//     ( //shaders
//         {GL_COMPUTE_SHADER, "<update active regions compute shader>",
//          DEFAULT_HEADER SHADER_SOURCE(
//              /////////////////////////////////////////////////////////////////

//              void main()
//              {

//              }
//              /*/////////////////////////////////////////////////////////////*/)}
//         ));
