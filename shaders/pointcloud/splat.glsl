//ComputeShader
#version 460
#extension GL_KHR_shader_subgroup_vote : enable
#extension GL_KHR_shader_subgroup_arithmetic : enable
//#extension GL_KHR_shader_subgroup : enable

layout (local_size_x = 128, local_size_y = 1) in;

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

layout(std430, binding = 0) readonly restrict buffer Voxels_t {
    ivec4 v[];
} vxls;

//splat smaller voxels as 2x2 pixels with atomicMin
uniform layout(binding = 0, r32ui) restrict uimage2D pointBuffer;
uniform layout(binding = 1, rgba8) restrict image2D colorBuffer;

void main(){
	uint DrawID = gl_GlobalInvocationID.x;
    //if (DrawID >= in_draws.cnt)
    //    return;
    //if (in_draws.cmds[DrawID].count < gl_GlobalInvocationID.y)
    //    return;

    ivec2 imgSize = imageSize(pointBuffer);
    uint VertexID = DrawID;//in_draws.cmds[DrawID].baseVertex + gl_GlobalInvocationID.y;
    
            ivec4 vID = vxls.v[VertexID];
            float x = float(vID.x) / 10000.0;
            float y = float(vID.y) / 10000.0;
            float z = float(vID.z) / 10000.0;
            vec4 color = unpackUnorm4x8(vID.w);

            vec3 UV;
            UV.x = x;// + in_draws.cmds[DrawID].pos.x;
            UV.y = y;// + in_draws.cmds[DrawID].pos.y;
            UV.z = z;// + in_draws.cmds[DrawID].pos.z;
            
            vec4 ppos = GlobalParams.vp * vec4(UV, 1);
            ppos /= ppos.w;

            ppos.xy = ppos.xy * 0.5f + 0.5f;
            if (ppos.x >= 0.0f && ppos.y >= 0.0f && ppos.x < 1.0f && ppos.y < 1.0f){
                ivec2 ppos_pxl = ivec2(ppos.xy * imgSize);
                uint test_val = uint(ppos.z * 16777216);

                //get all threads in workgroup which are writing to the same pixel
                //and find the one with the largest depth
                if (subgroupAllEqual(ppos_pxl))
                {
                    if( subgroupMax(test_val) == test_val)
                    {
                        if(test_val > imageAtomicMax(pointBuffer, ppos_pxl, test_val))
                        {
                            imageStore(colorBuffer, ppos_pxl, vec4(0.0f, 1.0f, 0.0f, 1.0f));
                        }
                    }
                }
                else if(test_val > imageAtomicMax(pointBuffer, ppos_pxl, test_val))
                {
                    imageStore(colorBuffer, ppos_pxl, color);
                }
            }

            VertexID ++;
}
