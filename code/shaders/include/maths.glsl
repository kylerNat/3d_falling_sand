vec4 conjugate(vec4 q)
{
    return vec4(q.x, -q.y, -q.z, -q.w);
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

vec3 sign_not_zero(vec3 p) {
    return 2*step(0, p)-1;
}

vec2 vec_to_oct(vec3 p)
{
    vec3 sign_p = sign_not_zero(p);
    vec2 oct = p.xy * (1.0f/dot(p, sign_p));
    return (p.z > 0) ? oct : (1.0f-abs(oct.yx))*sign_p.xy;
}

vec3 oct_to_vec(vec2 oct)
{
    vec2 sign_oct = sign(oct);
    vec3 p = vec3(oct.xy, 1.0-dot(oct, sign_oct));
    if(p.z < 0) p.xy = (1.0f-abs(p.yx))*sign_oct.xy;
    return normalize(p);
}

float cdf(float x)
{
    return 0.5f*tanh(0.797884560803f*(x+0.044715f*x*x*x))+0.5f;
}
