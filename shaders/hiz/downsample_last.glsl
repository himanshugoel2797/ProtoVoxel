//ComputeShader
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

uniform layout(binding = 0, rg32f) restrict readonly image2D inBuffer;
uniform layout(binding = 1, rg32f) restrict writeonly image2D outBuffer;
uniform layout(binding = 2, rg32f) restrict writeonly image2D outBuffer_4;
uniform layout(binding = 3, rg32f) restrict writeonly image2D outBuffer_2;
uniform layout(binding = 4, rg32f) restrict writeonly image2D outBuffer_1;

shared vec2 ds[8 * 8];

void main(){
    ivec2 px = ivec2(gl_GlobalInvocationID.xy);

	vec2 a0 = imageLoad(inBuffer, px * 2 + ivec2(0, 0)).xy;
	vec2 a1 = imageLoad(inBuffer, px * 2 + ivec2(0, 1)).xy;
	vec2 a2 = imageLoad(inBuffer, px * 2 + ivec2(1, 0)).xy;
	vec2 a3 = imageLoad(inBuffer, px * 2 + ivec2(1, 1)).xy;

    vec4 bounds;
    bounds.x = min(min(a0.x, a1.x), min(a2.x, a3.x));
    bounds.y = max(max(a0.y, a1.y), max(a2.y, a3.y));

    ds[gl_LocalInvocationIndex] = bounds.xy;

    imageStore(outBuffer, px, bounds);
    memoryBarrierShared();

    if(px.x >= 4)
        return;
    if(px.y >= 4)
        return;

    a0 = ds[gl_LocalInvocationID.y * 8 * 2 + gl_LocalInvocationID.x * 2];
    a1 = ds[gl_LocalInvocationID.y * 8 * 2 + gl_LocalInvocationID.x * 2 + 1];
    a2 = ds[(gl_LocalInvocationID.y + 1) * 8 * 2 + gl_LocalInvocationID.x * 2];
    a3 = ds[(gl_LocalInvocationID.y + 1) * 8 * 2 + gl_LocalInvocationID.x * 2 + 1];

    bounds.x = min(min(a0.x, a1.x), min(a2.x, a3.x));
    bounds.y = max(max(a0.y, a1.y), max(a2.y, a3.y));

    ds[gl_LocalInvocationIndex] = bounds.xy;
    imageStore(outBuffer_4, ivec2(gl_WorkGroupID.xy * 0.5f + gl_LocalInvocationID.xy), bounds);

    memoryBarrierShared();
    
    if(px.x >= 2)
        return;
    if(px.y >= 2)
        return;

    a0 = ds[gl_LocalInvocationID.y * 8 * 2 + gl_LocalInvocationID.x * 2];
    a1 = ds[gl_LocalInvocationID.y * 8 * 2 + gl_LocalInvocationID.x * 2 + 1];
    a2 = ds[(gl_LocalInvocationID.y + 1) * 8 * 2 + gl_LocalInvocationID.x * 2];
    a3 = ds[(gl_LocalInvocationID.y + 1) * 8 * 2 + gl_LocalInvocationID.x * 2 + 1];

    bounds.x = min(min(a0.x, a1.x), min(a2.x, a3.x));
    bounds.y = max(max(a0.y, a1.y), max(a2.y, a3.y));

    ds[gl_LocalInvocationIndex] = bounds.xy;
    imageStore(outBuffer_2, ivec2(gl_WorkGroupID.xy * 0.25f + gl_LocalInvocationID.xy), bounds);
    memoryBarrierShared();
    
    if(px.x >= 1)
        return;
    if(px.y >= 1)
        return;

    a0 = ds[0];
    a1 = ds[1];
    a2 = ds[8 * 2];
    a3 = ds[8 * 2 + 1];

    bounds.x = min(min(a0.x, a1.x), min(a2.x, a3.x));
    bounds.y = max(max(a0.y, a1.y), max(a2.y, a3.y));

    ds[gl_LocalInvocationIndex] = bounds.xy;
    imageStore(outBuffer_1, ivec2(0), bounds);
    memoryBarrierShared();
}
