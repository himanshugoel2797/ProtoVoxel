//FragmentShader
#version 460

in vec2 UV;

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

uniform layout(binding = 0, r32ui) restrict uimage2D pointBuffer;

void main(){
	ivec2 px = ivec2(imageSize(pointBuffer) * UV);

	uint v = imageLoad(pointBuffer, px).x;

	gl_FragDepth = (v >> 8u) /  16777216.0f;

	vec4 n_ = GlobalParams.ivp * vec4(gl_FragCoord.x / 512.0f - 1.0f, gl_FragCoord.y / 512.0f - 1.0f, gl_FragDepth, 1);
    n_ /= n_.w;

    vec3 n = -(n_.xyz - GlobalParams.eyePos.xyz);
	
    vec3 abs_n = abs(n);
    float max_n = max(abs_n.x, max(abs_n.y, abs_n.z));
    n = step(max_n, abs_n) * sign(n);

	if ((v & 0xffu) != 0)
		out_color = vec4(n * 0.5f + 0.5f, 1);
	else
		out_color = vec4(0);
}