#version 460

layout (local_size_x = 256, local_size_y = 1) in;

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

layout(std430, binding = 0) readonly restrict buffer ChunkOffsets_t {
    ivec4 v[];
} ChunkOffsets;

layout(std430, binding = 2) readonly restrict buffer Voxels_t {
    uint v[];
} vxls;

struct draw_cmd_t {
	uint count;
	uint instanceCount;
	uint baseVertex;
	uint baseInstance;
};

layout(std430, binding = 1) readonly restrict buffer InDrawCalls_t{
	uint cnt;
	uint pd0;
	uint pd1;
	uint pd2;
	draw_cmd_t cmds[];
} in_draws;

//splat smaller voxels as 2x2 pixels with atomicMin
uniform layout(binding = 0, r32ui) restrict uimage2D pointBuffer;

void main(){
	uint DrawID = gl_GlobalInvocationID.x;
    if (DrawID >= in_draws.cnt)
        return;

    ivec2 imgSize = imageSize(pointBuffer);
    uint VertexID = in_draws.cmds[DrawID].baseVertex;
    for (int i = 0; i < 256; i++){
            if (in_draws.cmds[DrawID].count <= i)
                return;

            uint vID = vxls.v[VertexID];
            float x = float((vID >> 19u) & 0x1fu);
            float y = float((vID >> 8u) & 0x3fu);
            float z = float((vID >> 14u) & 0x1fu);

            uint mat_idx = uint((vID) & 0x7fu);

            vec3 UV;
            UV.x = x + ChunkOffsets.v[DrawID].x;
            UV.y = y + ChunkOffsets.v[DrawID].y;
            UV.z = z + ChunkOffsets.v[DrawID].z;
            
            vec4 ppos = GlobalParams.vp * vec4(UV, 1);
            ppos /= ppos.w;

            ppos.xy = ppos.xy * 0.5f + 0.5f;
            if (ppos.x >= 0.0f && ppos.y >= 0.0f && ppos.x < 1.0f && ppos.y < 1.0f){
                ivec2 ppos_pxl = ivec2(ppos.xy * imgSize);

                uint test_val = uint(ppos.z * 16777216) << 8u;
                test_val |= mat_idx;

                imageAtomicMax(pointBuffer, ppos_pxl, test_val);
            }

            VertexID ++;
    }
}
