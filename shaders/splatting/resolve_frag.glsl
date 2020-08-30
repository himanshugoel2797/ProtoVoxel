//FragmentShader
#version 460

in vec2 UV;

layout(location = 0) out vec4 out_color;

uniform layout(binding = 0, r32ui) restrict uimage2D pointBuffer;

void main(){
	ivec2 px = ivec2(imageSize(pointBuffer) * UV);

	uint v = imageLoad(pointBuffer, px).x;

	gl_FragDepth = (v >> 8u) /  16777216.0f;
	if ((v & 0xffu) != 0)
		out_color = vec4(1);
	else
		out_color = vec4(0);
}