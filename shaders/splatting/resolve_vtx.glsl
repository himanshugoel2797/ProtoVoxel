//VertexShader
#version 460

out vec2 UV;

void main(){
	float x = -1.0f + float((gl_VertexID & 1) << 2);
	float y = -1.0f + float((gl_VertexID & 2) << 1);

	UV.x = (x + 1.0f) * 0.5f;
	UV.y = (y + 1.0f) * 0.5f;

	gl_Position = vec4(x, y, 1.0f, 1);
}