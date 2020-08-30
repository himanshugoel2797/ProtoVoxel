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

void main(){
    ivec2 px = ivec2(gl_GlobalInvocationID.xy);

	vec4 a0 = imageLoad(inBuffer, px * 2 + ivec2(0, 0));
	vec4 a1 = imageLoad(inBuffer, px * 2 + ivec2(0, 1));
	vec4 a2 = imageLoad(inBuffer, px * 2 + ivec2(1, 0));
	vec4 a3 = imageLoad(inBuffer, px * 2 + ivec2(1, 1));

    vec4 bounds;
    bounds.x = min(min(a0.x, a1.x), min(a2.x, a3.x));
    bounds.y = max(max(a0.y, a1.y), max(a2.y, a3.y));

    imageStore(outBuffer, px, bounds);
}
