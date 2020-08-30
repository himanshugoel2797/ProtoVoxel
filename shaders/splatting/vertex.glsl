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

layout(std430, binding = 2) readonly restrict buffer Voxels_t {
    uint v[];
} Voxels;

layout(std430, binding = 1) readonly restrict buffer ChunkOffsets_t {
    ivec4 v[];
} ChunkOffsets;

layout(std140, binding = 2) uniform ColorPalette_t{
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

            UV.x = x + ChunkOffsets.v[gl_DrawID].x;
            UV.y = y + ChunkOffsets.v[gl_DrawID].y;
            UV.z = z + ChunkOffsets.v[gl_DrawID].z;
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
            
            vec2 max_comps = max( max( max( bbox[0].xy, bbox[1].xy), 
                                       max( bbox[2].xy, bbox[3].xy)),  
                                  max( max( bbox[4].xy, bbox[5].xy), 
                                       max( bbox[6].xy, bbox[7].xy)));

            vec2 min_comps = min( min( min( bbox[0].xy, bbox[1].xy), 
                                       min( bbox[2].xy, bbox[3].xy)), 
                                  min( min( bbox[4].xy, bbox[5].xy), 
                                       min( bbox[6].xy, bbox[7].xy)));

            vec2 dvec0 = (max_comps - min_comps);
            float max_radius = max(dvec0.x, dvec0.y) * 0.5f;
            gl_PointSize = 1024.0f * max_radius;

            //gl_Position = vec4(x, y, z, 1);
            gl_Position = GlobalParams.vp * vec4(UV, 1);
            //gl_PointSize = (1024.0f * 1.5f) / gl_Position.w;
}