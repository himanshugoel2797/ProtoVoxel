//VertexShader
#version 460

layout(location = 0) uniform uint DrawID;

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

layout(std430, binding = 1) readonly restrict buffer Voxels_t {
    uint v[];
} Voxels;

struct draw_cmd_t {
	uint count;
	uint instanceCount;
	uint baseVertex;
	uint baseInstance;
    ivec4 pos;
    ivec4 min_bnd;
    ivec4 max_bnd;
};

layout(std430, binding = 0) readonly restrict buffer InDrawCalls_t{
	uint cnt;
	uint pd0;
	uint pd1;
	uint pd2;
	draw_cmd_t cmds[];
} in_draws;

layout(std140, binding = 1) uniform ColorPalette_t{
        vec4 v[256];
} ColorPalette;

// Output data ; will be interpolated for each fragment.
out vec3 UV;
out vec3 eyePos_rel;
out vec4 color_vs;


void main(){
            uint vID = Voxels.v[gl_VertexID];
            float x = float((vID >> 19u) & 0x1fu);
            float y = float((vID >> 8u) & 0x3fu);
            float z = float((vID >> 14u) & 0x1fu);

            int mat_idx = int((vID) & 0x7fu);

            UV.x = x + in_draws.cmds[gl_DrawID].pos.x;
            UV.y = y + in_draws.cmds[gl_DrawID].pos.y;
            UV.z = z + in_draws.cmds[gl_DrawID].pos.z;
            eyePos_rel = GlobalParams.eyePos.xyz - UV;

            color_vs = ColorPalette.v[mat_idx];

            vec4 bbox[8];
            bbox[0] = GlobalParams.vp * vec4(UV + vec3(0.5f, 0.5f, 0.5f), 1);
            bbox[1] = GlobalParams.vp * vec4(UV + vec3(0.5f, 0.5f, -0.5f), 1);
            bbox[2] = GlobalParams.vp * vec4(UV + vec3(0.5f, -0.5f, 0.5f), 1);
            bbox[3] = GlobalParams.vp * vec4(UV + vec3(0.5f, -0.5f, -0.5f), 1);
            bbox[4] = GlobalParams.vp * vec4(UV + vec3(-0.5f, 0.5f, 0.5f), 1);
            bbox[5] = GlobalParams.vp * vec4(UV + vec3(-0.5f, 0.5f, -0.5f), 1);
            bbox[6] = GlobalParams.vp * vec4(UV + vec3(-0.5f, -0.5f, 0.5f), 1);
            bbox[7] = GlobalParams.vp * vec4(UV + vec3(-0.5f, -0.5f, -0.5f), 1);

            bbox[0] /= bbox[0].w;
            bbox[1] /= bbox[1].w;
            bbox[2] /= bbox[2].w;
            bbox[3] /= bbox[3].w;
            bbox[4] /= bbox[4].w;
            bbox[5] /= bbox[5].w;
            bbox[6] /= bbox[6].w;
            bbox[7] /= bbox[7].w;
            
            vec3 max_comps = max( max( max( bbox[0].xyz, bbox[1].xyz), 
                                       max( bbox[2].xyz, bbox[3].xyz)),  
                                  max( max( bbox[4].xyz, bbox[5].xyz), 
                                       max( bbox[6].xyz, bbox[7].xyz)));

            vec3 min_comps = min( min( min( bbox[0].xyz, bbox[1].xyz), 
                                       min( bbox[2].xyz, bbox[3].xyz)), 
                                  min( min( bbox[4].xyz, bbox[5].xyz), 
                                       min( bbox[6].xyz, bbox[7].xyz)));

            max_comps.xy = clamp(max_comps.xy, vec2(-1), vec2(1));
            min_comps.xy = clamp(min_comps.xy, vec2(-1), vec2(1));
            
            vec2 dvec0 = (max_comps.xy - min_comps.xy);
            
            float area = dvec0.x * dvec0.y;
            float max_radius = max(dvec0.x, dvec0.y) * 0.5f;

            gl_PointSize = 1024.0f * max_radius * 1.1f;
            gl_Position = vec4( (min_comps.xy + max_comps.xy) * 0.5f, 0.5f, 1.0f );
            
            //TODO: Large performance regression when point is too large
            if (area <= 0 || max_radius <= 0 || max_radius >= 1024.0f){
                gl_Position = vec4(2, 2, 2, 1);
                gl_PointSize = 1;
            }
}