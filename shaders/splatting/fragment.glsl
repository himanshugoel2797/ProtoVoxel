//FragmentShader
#version 460

// Interpolated values from the vertex shaders
in vec3 UV;
in vec3 eyePos_rel;
in vec4 color;

// Ouput data
layout(location = 0) out vec4 out_color;

// Values that stay constant for the whole mesh.
layout(std140, binding = 0) uniform GlobalParams_t {
        mat4 proj;
        mat4 view;
        mat4 vp;
        mat4 ivp;
        mat4 prev_proj;
        mat4 prev_view;
        mat4 prev_vp;
        mat4 prev_ivp;
        vec4 prev_eyePos;
        vec4 prev_eyeUp;
        vec4 prev_eyeDir;
        vec4 eyePos;
        vec4 eyeUp;
        vec4 eyeDir;
        vec4 eyeRight;
} GlobalParams;

bool boxIntersection( vec3 ro, vec3 rd, out float t0, out float t1 ) 
{
    vec3 invR = 1.0f / rd;
    vec3 tmin = invR * (-0.5f - ro);
    vec3 tmax = invR * (0.5f - ro);
    vec3 t1_ = min(tmin, tmax);
    vec3 t2_ = max(tmin, tmax);

    t0 = max(max(t1_.x, t1_.y), t1_.z);
    t1 = min(min(t2_.x, t2_.y), t2_.z);
    return t0 <= t1;
}

void main(){
    vec2 screenPos = 2.0f / 1024.0f * gl_FragCoord.xy - 1.0f;
    vec3 rayDir = vec3(screenPos.x, screenPos.y, 0.01);
    vec4 pPos = (GlobalParams.ivp * vec4(rayDir, 1));
    pPos /= pPos.w;

    rayDir = (pPos.xyz - GlobalParams.eyePos.xyz);

    float t0;
    float t1;

    bool intersected = boxIntersection(eyePos_rel, rayDir, t0, t1);
    vec3 intersection = eyePos_rel + rayDir * t0;
        
    vec3 abs_n = abs(intersection);
    float max_n = max(abs_n.x, max(abs_n.y, abs_n.z));
    vec3 n = step(max_n, abs_n) * sign(intersection);

    if( intersected ){
        vec4 trans_pos = GlobalParams.vp * vec4(intersection + UV, 1);
        gl_FragDepth = trans_pos.z / trans_pos.w;
        out_color = vec4(n * 0.5f + 0.5f, 1);
    }else
        discard;
    //out_color = vec4(1, 0, 0, 1);
}