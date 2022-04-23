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
             layout(location = 3) uniform ivec3 size;

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
                 float chunk_size = 256;
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
                 // max_iterations = min(max_iterations, 150);

                 for(;;)
                 {
                     // if(pos.x < 0 || pos.y < 0 || pos.z < 0
                     //    || pos.x > size.x || pos.y > size.y || pos.z > size.z)
                     // {
                     //     discard;
                     //     return;
                     // }
                     ivec4 voxel = texelFetch(materials, ivec3(pos), 0);
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
                     texture(materials, (pos+vec3(1,0,0))*scale).g-texture(materials, (pos+vec3(-1,0,0))*scale).g,
                     texture(materials, (pos+vec3(0,1,0))*scale).g-texture(materials, (pos+vec3(0,-1,0))*scale).g,
                     texture(materials, (pos+vec3(0,0,1))*scale).g-texture(materials, (pos+vec3(0,0,-1))*scale).g+0.001
                     );
                 gradient = normalize(gradient);
                 frag_color.rgb = 0.5*gradient+vec3(0.5,0.5,0.5);

                 if(texelFetch(materials, ivec3(pos), 0).r > 1)
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

                 float light_brightness = texture(materials, (pos - 2*epsilon*ray_dir)*scale).b;
                 light_brightness *= 0.0025;

                 frag_color.rgb *= light_brightness;
                 // frag_color.a = clamp(1.0-8.0*total_dist/chunk_size, 0, 1);
                 frag_color.a = 1.0;
                 gl_FragDepth = 1.0-total_dist/(chunk_size*sqrt(3));
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

             smooth out vec2 uv;

             void main()
             {
                 gl_Position.xyz = x;
                 gl_Position.w = 1.0;
                 uv = 0.5*x.xy+0.5;
             }
             /*/////////////////////////////////////////////////////////////*/)},
        {GL_FRAGMENT_SHADER, "<simulate chunk fragment shader>",
         DEFAULT_HEADER SHADER_SOURCE(
             /////////////////////////////////////////////////////////////////
             layout(location = 0) out ivec4 frag_color;

             layout(location = 0) uniform int layer;
             layout(location = 1) uniform int frame_number;
             layout(location = 2) uniform isampler3D materials;

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

             float brightness_curve(float x)
             {
                 return pow(x, 2.0);
             }

             float inverse_brightness_curve(float x)
             {
                 return pow(x, 0.5);
             }

             void main()
             {
                 int chunk_size = 256;
                 float scale = 1.0/chunk_size;
                 ivec2 pos = ivec2(chunk_size*uv);

                 //+,0,-,0
                 //0,+,0,-
                 // int rot = (frame_number+layer)%4;
                 uint rot = rand(rand(rand(frame_number)))%4;
                 int i = 0;
                 ivec2 dir = ivec2(((rot&1)*(2-rot)), (1-(rot&1))*(1-rot));

                 ivec4 m = texelFetch(materials, ivec3(pos.x, pos.y, layer), 0);
                 ivec4 u = texelFetch(materials, ivec3(pos.x, pos.y, layer+1), 0);
                 ivec4 d = texelFetch(materials, ivec3(pos.x, pos.y, layer-1), 0);
                 ivec4 r = texelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, layer), 0);
                 ivec4 l0 = texelFetch(materials, ivec3(pos.x-dir.x, pos.y-dir.y, layer), 0);
                 ivec4 dr = texelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, layer-1), 0);
                 ivec4 ur = texelFetch(materials, ivec3(pos.x+dir.x, pos.y+dir.y, layer+1), 0);

                 bool fall_allowed = (layer > 0 && (d.r == 0 || (dr.r == 0 && r.r == 0)));
                 bool flow_allowed = (layer > 0 && r.r == 0 && ur.r == 0 && u.r == 0 && m.r > 1 && l0.r > 0);

                 if(m.r == 3)
                 {
                     fall_allowed = false;
                     flow_allowed = false;
                 }

                 frag_color = m;
                 if(m.r == 0)
                 {
                     // int in_rot = (frame_number+layer+1)%4;
                     uint in_rot = rot;
                     ivec2 in_dir = ivec2(((in_rot&1)*(2-in_rot)), (1-(in_rot&1))*(1-in_rot));

                     ivec4 ul = texelFetch(materials, ivec3(pos.x-in_dir.x, pos.y-in_dir.y, layer+1), 0);
                     ivec4 l = texelFetch(materials, ivec3(pos.x-in_dir.x, pos.y-in_dir.y, layer), 0);
                     ivec4 ll = texelFetch(materials, ivec3(pos.x-2*in_dir.x, pos.y-2*in_dir.y, layer), 0);
                     ivec4 dl = texelFetch(materials, ivec3(pos.x-in_dir.x, pos.y-in_dir.y, layer-1), 0);
                     if(u.r > 0 && u.r != 3) frag_color = u;
                     else if(l.r > 0 && ul.r > 0 && u.r != 3 && ul.r != 3) frag_color = ul;
                     else if(dl.r > 0 && d.r > 0 && l.r > 1 && ll.r > 0 && u.r != 3 && ul.r != 3 && l.r != 3) frag_color = l;

                     // if(u.r == 0 && ul.r == 0 && dl.r > 0 && d.r > 0 && l.r > 0) frag_color = l;
                     // else if(u.r == 0 && l.r > 0) frag_color = ul;
                     // else if(u.r > 0) frag_color = u;
                 }
                 else if(fall_allowed)
                 {
                     frag_color.r = 0;
                 }
                 else if(flow_allowed)
                 {
                     frag_color.r = 0;
                 }

                 if(frag_color.r > 0)
                 {
                     frag_color.g = -10;
                     frag_color.g = max(frag_color.g, texelFetch(materials, ivec3(pos.x+1, pos.y, layer), 0).g-1);
                     frag_color.g = max(frag_color.g, texelFetch(materials, ivec3(pos.x-1, pos.y, layer), 0).g-1);
                     frag_color.g = max(frag_color.g, texelFetch(materials, ivec3(pos.x, pos.y+1, layer), 0).g-1);
                     frag_color.g = max(frag_color.g, texelFetch(materials, ivec3(pos.x, pos.y-1, layer), 0).g-1);
                     frag_color.g = max(frag_color.g, texelFetch(materials, ivec3(pos.x, pos.y, layer+1), 0).g-1);
                     frag_color.g = max(frag_color.g, texelFetch(materials, ivec3(pos.x, pos.y, layer-1), 0).g-1);

                     if(texelFetch(materials, ivec3(pos.x+1, pos.y, layer), 0).r == 0) frag_color.g = 0;
                     if(texelFetch(materials, ivec3(pos.x-1, pos.y, layer), 0).r == 0) frag_color.g = 0;
                     if(texelFetch(materials, ivec3(pos.x, pos.y+1, layer), 0).r == 0) frag_color.g = 0;
                     if(texelFetch(materials, ivec3(pos.x, pos.y-1, layer), 0).r == 0) frag_color.g = 0;
                     if(texelFetch(materials, ivec3(pos.x, pos.y, layer+1), 0).r == 0) frag_color.g = 0;
                     if(texelFetch(materials, ivec3(pos.x, pos.y, layer-1), 0).r == 0) frag_color.g = 0;

                     frag_color.b = texelFetch(materials, ivec3(pos.x, pos.y, layer), 0).b/2;
                     if(frag_color.r == 3) frag_color.b = 1000;
                     // frag_color.b = 0;
                 }
                 else
                 {
                     frag_color.g = 10;
                     frag_color.g = min(frag_color.g, texelFetch(materials, ivec3(pos.x+1, pos.y, layer), 0).g+1);
                     frag_color.g = min(frag_color.g, texelFetch(materials, ivec3(pos.x-1, pos.y, layer), 0).g+1);
                     frag_color.g = min(frag_color.g, texelFetch(materials, ivec3(pos.x, pos.y+1, layer), 0).g+1);
                     frag_color.g = min(frag_color.g, texelFetch(materials, ivec3(pos.x, pos.y-1, layer), 0).g+1);
                     frag_color.g = min(frag_color.g, texelFetch(materials, ivec3(pos.x, pos.y, layer+1), 0).g+1);
                     frag_color.g = min(frag_color.g, texelFetch(materials, ivec3(pos.x, pos.y, layer-1), 0).g+1);

                     // frag_color.b = 1+texelFetch(materials, ivec3(pos.x, pos.y, layer), 0).b;
                     float brightness = 100.0;
                     brightness += brightness_curve(texelFetch(materials, ivec3(pos.x+1, pos.y, layer), 0).b);
                     brightness += brightness_curve(texelFetch(materials, ivec3(pos.x-1, pos.y, layer), 0).b);
                     brightness += brightness_curve(texelFetch(materials, ivec3(pos.x, pos.y+1, layer), 0).b);
                     brightness += brightness_curve(texelFetch(materials, ivec3(pos.x, pos.y-1, layer), 0).b);
                     brightness += brightness_curve(texelFetch(materials, ivec3(pos.x, pos.y, layer+1), 0).b);
                     brightness += brightness_curve(texelFetch(materials, ivec3(pos.x, pos.y, layer-1), 0).b);
                     brightness *= 1.0/6.0;
                     frag_color.b = int(inverse_brightness_curve(brightness));

                     // frag_color.b = texelFetch(materials, ivec3(pos.x, pos.y, layer), 0).b;
                     // frag_color.b += texelFetch(materials, ivec3(pos.x, pos.y, layer), 0).a;

                     // frag_color.a = 10*texelFetch(materials, ivec3(pos.x, pos.y, layer), 0).a;
                     // frag_color.a += -7*texelFetch(materials, ivec3(pos.x, pos.y, layer), 0).b;
                     // frag_color.a += texelFetch(materials, ivec3(pos.x+1, pos.y, layer), 0).b;
                     // frag_color.a += texelFetch(materials, ivec3(pos.x-1, pos.y, layer), 0).b;
                     // frag_color.a += texelFetch(materials, ivec3(pos.x, pos.y+1, layer), 0).b;
                     // frag_color.a += texelFetch(materials, ivec3(pos.x, pos.y-1, layer), 0).b;
                     // frag_color.a += texelFetch(materials, ivec3(pos.x, pos.y, layer+1), 0).b;
                     // frag_color.a += texelFetch(materials, ivec3(pos.x, pos.y, layer-1), 0).b;
                     // frag_color.a /= 20;
                 }
                 // float osc = sin(frame_number*sqrt(71.0/2));
                 float osc = 1.0;
                 if(pos.x == 0) frag_color.b = int(10000*osc);
                 if(pos.x == 128 && pos.y == 128 && layer >= 30 && layer <= 40) frag_color.b = int(32768*osc);
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

             void main()
             {
                 int chunk_size = 256;
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
                     // vec3 ray_dir = normalize(x_dot);
                     // vec3 sign = vec3(ray_dir.x > 0 ? 1: -1,
                     //                  ray_dir.y > 0 ? 1: -1,
                     //                  ray_dir.z > 0 ? 1: -1);
                     // vec3 invabs_ray_dir = sign/ray_dir;
                     // if(ray_dir.x == 0) invabs_ray_dir.x = 1000;
                     // if(ray_dir.y == 0) invabs_ray_dir.y = 1000;
                     // if(ray_dir.z == 0) invabs_ray_dir.z = 1000;

                     // int hit_dir = 0;

                     // int max_iterations = 20;
                     // int i = 0;
                     // for(;;)
                     // {
                     //     if(texture(materials, x*scale).r > 0)
                     //     {
                     //         break;
                     //     }

                     //     vec3 dist = mod(-x*sign, 1.0)*invabs_ray_dir;
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
                     //     if(min_dist+epsilon >= remaining_dist)
                     //     {
                     //         x += remaining_dist*ray_dir;
                     //         break;
                     //     }

                     //     x += (min_dist+epsilon)*ray_dir;
                     //     remaining_dist -= min_dist+epsilon;
                     //     hit_dir = min_dir;

                     //     if(++i >= max_iterations)
                     //     {
                     //         return;
                     //     }
                     // }
                     x += x_dot;
                     if(x.z < epsilon && x_dot.z < 0)
                     {
                         x.z = epsilon;
                         x_dot.z = 0;
                         // x_dot *= 0.99;
                     }
                     int max_iterations = 100;
                     int i = 0;
                     while(texture(materials, x*scale).r > 0)
                     {
                         vec3 gradient = vec3(
                             texture(materials, (x+vec3(1,0,0))*scale).g-texture(materials, (x+vec3(-1,0,0))*scale).g,
                             texture(materials, (x+vec3(0,1,0))*scale).g-texture(materials, (x+vec3(0,-1,0))*scale).g,
                             texture(materials, (x+vec3(0,0,1))*scale).g-texture(materials, (x+vec3(0,0,-1))*scale).g+0.001
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

             void main()
             {
                 vec2 xy = mix(bbl.xy, bbu.xy, uv);

                 float min_z = bbl.z; //TODO: use fragment z coord, then scan until the body coord is outside
                 float max_z = bbu.z;

                 collision_point.xyz = vec3(0,0,0);
                 collision_point.a = 1.0;
                 collision_normal = vec3(0,0,0);

                 float chunk_size = 256;
                 float scale = (1.0/chunk_size);

                 for(float z = min_z; z <= max_z; z += 1)
                 {
                     vec3 world_coord = vec3(xy.x, xy.y, z);
                     vec3 body_coord = inv_body_axes*(world_coord-body_x)+body_x_cm;
                     vec4 world_voxel = texelFetch(materials, ivec3(world_coord), 0);
                     float world_material = world_voxel.r;
                     float body_material = texelFetch(body_materials, ivec3(body_coord), 0).r;
                     if(world_material > 0 && body_material > 0)
                     {
                         vec3 gradient = vec3(
                             texture(materials, (world_coord+vec3(1,0,0))*scale).g
                             -texture(materials, (world_coord+vec3(-1,0,0))*scale).g,
                             texture(materials, (world_coord+vec3(0,1,0))*scale).g
                             -texture(materials, (world_coord+vec3(0,-1,0))*scale).g,
                             texture(materials, (world_coord+vec3(0,0,1))*scale).g
                             -texture(materials, (world_coord+vec3(0,0,-1))*scale).g+0.001
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
