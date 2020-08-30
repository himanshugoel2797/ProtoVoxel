//ComputeShader
#version 460

layout (local_size_x = 1024, local_size_y = 1) in;

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

layout(std430, binding = 3) writeonly restrict buffer OutOffsets_t {
    ivec4 v[];
} OutOffsets;

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

layout(std430, binding = 2) restrict coherent buffer OutDrawCalls_t{
	uint cnt;
	uint pd0;
	uint pd1;
	uint pd2;
	draw_cmd_t cmds[];
} out_draws;

uniform layout(binding = 0) sampler2D hiz_map;

vec4 offsets[8] = vec4[] (
    vec4(15, 31, 15, 0),
    vec4(15, 31, -15, 0),
    vec4(15, -31, 15, 0),
    vec4(15, -31, -15, 0),
    vec4(-15, 31, 15, 0),
    vec4(-15, 31, -15, 0),
    vec4(-15, -31, 15, 0),
    vec4(-15, -31, -15, 0)
);

void main(){
	uint DrawID = gl_GlobalInvocationID.x;
    if (DrawID >= in_draws.cnt)
        return;
    
    vec3 UV = vec3(ChunkOffsets.v[DrawID].x + 15, ChunkOffsets.v[DrawID].y + 31, ChunkOffsets.v[DrawID].z + 15);
	vec4 bbox;
    bbox = GlobalParams.vp * (vec4(UV, 1) + offsets[0]);
    bbox /= bbox.w;
    
    vec3 max_comps = bbox.xyz;
    vec3 min_comps = bbox.xyz;

    for (int i = 1; i < 8; i++){
            bbox = GlobalParams.vp * (vec4(UV, 1) + offsets[i]);
            bbox /= bbox.w;
        
            max_comps = max(bbox.xyz, max_comps);
            min_comps = min(bbox.xyz, min_comps);
    }

            max_comps.xy = max_comps.xy * 0.5f + 0.5f;
            min_comps.xy = min_comps.xy * 0.5f + 0.5f;

            //Chunk is outside the frustum, don't draw
            max_comps.xy = clamp(max_comps.xy, vec2(0), vec2(1));
            min_comps.xy = clamp(min_comps.xy, vec2(0), vec2(1));
            
            vec2 dvec0 = (max_comps.xy - min_comps.xy) * 1024.0f;  //Convert to pixels
            float max_rad = max(dvec0.x, dvec0.y);

            //Choose a mipmap level to test against
            float lod = ceil( log2 ( max_rad ));
            vec4 samples;
            samples.x = textureLod(hiz_map, vec2(min_comps.x, min_comps.y), lod).x;
            samples.y = textureLod(hiz_map, vec2(min_comps.x, max_comps.y), lod).x;
            samples.z = textureLod(hiz_map, vec2(max_comps.x, max_comps.y), lod).x;
            samples.w = textureLod(hiz_map, vec2(max_comps.x, min_comps.y), lod).x;
            float sampledDepth = min(min(samples.x, samples.y), min(samples.z, samples.w));
            //min=farthest point, max=nearest point

            if (max_rad == 0)  //Out of view
                return;
    
            if (sampledDepth <= max_comps.z)    //If the chunk is fully behind the sampled depth, don't render it
            {
                //Splat voxels via raster
                uint idx = atomicAdd(out_draws.cnt, 1);
                out_draws.cmds[idx].count = in_draws.cmds[DrawID].count;
                out_draws.cmds[idx].instanceCount = in_draws.cmds[DrawID].instanceCount;
                out_draws.cmds[idx].baseVertex = in_draws.cmds[DrawID].baseVertex;
                out_draws.cmds[idx].baseInstance = in_draws.cmds[DrawID].baseInstance;
                OutOffsets.v[idx] = ChunkOffsets.v[DrawID];
             }
}
