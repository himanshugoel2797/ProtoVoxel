
struct {
    uint count;
    uint instanceCount;
    uint baseIndex;
    uint zr0;
    uint baseInstance;
} indirect_draw_t;

struct {
    vec4 o;
} block_info_t;

layout (std430, binding = 1) readonly buffer block_infos_t{
    block_info_t[] v;
} BlockInfos;

layout (std430, binding = 2) readonly buffer draw_cmds_t{
    uint drawCount;
    uint pd0;
    uint pd1;
    uint pd2;
    indirect_draw_t v[];
} DrawCmds;

layout (std140, binding = 0) uniform global_params_t{
    mat4 proj;
	mat4 view;
	mat4 vp;
	mat4 ivp;
	mat4 prev_view;
	mat4 prev_vp;
	mat4 prev_ivp;
	vec4 prev_eyePos;
	vec4 prev_eyeUp;
	vec4 prev_eyeDir;
	vec4 eyePos;
	vec4 eyeUp;
	vec4 eyeDir;
} GlobalParams;

void main(){
    
}