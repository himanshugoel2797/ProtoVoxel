#version 460

layout (local_size_x = 8, local_size_y = 8) in;

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

uniform layout(binding = 0) sampler2D depthBuffer;
uniform layout(binding = 0, rg32f) restrict writeonly image2D mipBuffer;

void main(){
    ivec2 px = ivec2(gl_GlobalInvocationID.xy);
	vec4 a0 = texelFetch(depthBuffer, px, 0);

    ivec2 sz = imageSize(mipBuffer);

    vec4 opos = vec4(2.0f * (px / vec2(sz)) - 1.0f, a0.r, 1);
    vec4 reproj = opos;

    imageStore(mipBuffer, ivec2(sz * (reproj.xy * 0.5f + 0.5f)), vec4(reproj.z));
}
