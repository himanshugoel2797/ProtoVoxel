//ComputeShader
#version 460

layout (local_size_x = 64, local_size_y = 1) in;

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

layout(std430, binding = 1) restrict coherent buffer OutDrawCalls_t{
	uint cnt;
	uint pd0;
	uint pd1;
	uint pd2;
	draw_cmd_t cmds[];
} out_draws;

layout(std430, binding = 2) restrict coherent buffer Splats_t{
	uint cnt;
	uint pd0;
	uint pd1;
	uint pd2;
	draw_cmd_t cmds[];
} splats;

layout(std430, binding = 3) restrict coherent buffer Rejects_t{
	uint cnt;
	uint pd0;
	uint pd1;
	uint pd2;
	draw_cmd_t cmds[];
} rejects;

uniform layout(binding = 0) sampler2D hiz_map;


void main(){
	uint DrawID = gl_GlobalInvocationID.x;
    if (DrawID >= in_draws.cnt)
        return;

    rejects.pd1 = rejects.pd2 = 1;
    out_draws.pd0 = out_draws.pd1 = out_draws.pd2 = 1;
    splats.pd1 = 1;
    
    vec3 min_bnd = in_draws.cmds[DrawID].min_bnd.xyz;
    vec3 max_bnd = in_draws.cmds[DrawID].max_bnd.xyz;
    vec3 bnd_diff = max_bnd - min_bnd;
    float bnd_sz = min(min(bnd_diff.x, bnd_diff.y), bnd_diff.z);

vec4 offsets[8] = vec4[] (
    vec4(max_bnd.x, max_bnd.y, max_bnd.z, 0),
    vec4(max_bnd.x, max_bnd.y, min_bnd.z, 0),
    vec4(max_bnd.x, min_bnd.y, max_bnd.z, 0),
    vec4(max_bnd.x, min_bnd.y, min_bnd.z, 0),
    vec4(min_bnd.x, max_bnd.y, max_bnd.z, 0),
    vec4(min_bnd.x, max_bnd.y, min_bnd.z, 0),
    vec4(min_bnd.x, min_bnd.y, max_bnd.z, 0),
    vec4(min_bnd.x, min_bnd.y, min_bnd.z, 0)
);

    vec3 UV = vec3(in_draws.cmds[DrawID].pos.x, in_draws.cmds[DrawID].pos.y, in_draws.cmds[DrawID].pos.z);
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

            if (sampledDepth >= max_comps.z)    //If the chunk is fully behind the sampled depth, don't render it
            {
                uint idx = atomicAdd(rejects.cnt, 1);
                atomicMax(rejects.pd0, int(idx + 63) / 64);
                rejects.cmds[idx].count = in_draws.cmds[DrawID].count;
                rejects.cmds[idx].instanceCount = 1;
                rejects.cmds[idx].baseVertex = in_draws.cmds[DrawID].baseVertex;
                rejects.cmds[idx].baseInstance = 0;
                rejects.cmds[idx].pos = in_draws.cmds[DrawID].pos;
                rejects.cmds[idx].min_bnd = in_draws.cmds[DrawID].min_bnd;
                rejects.cmds[idx].max_bnd = in_draws.cmds[DrawID].max_bnd;
                rejects.cmds[idx].pos = in_draws.cmds[DrawID].pos;
            } else if (max_rad <= 2 * bnd_sz)
            {
                //Splat voxels via compute
                uint idx = atomicAdd(splats.cnt, 1);
                atomicMax(splats.pd0, int(in_draws.cmds[DrawID].count + 255) / 256 );
                splats.cmds[idx].count = in_draws.cmds[DrawID].count;
                splats.cmds[idx].instanceCount = 1;
                splats.cmds[idx].baseVertex = in_draws.cmds[DrawID].baseVertex;
                splats.cmds[idx].baseInstance = 0;
                splats.cmds[idx].pos = in_draws.cmds[DrawID].pos;
            }else
            {
                //Splat voxels via raster
                uint idx = atomicAdd(out_draws.cnt, 1);
                out_draws.cmds[idx].count = in_draws.cmds[DrawID].count;
                out_draws.cmds[idx].instanceCount = 1;
                out_draws.cmds[idx].baseVertex = in_draws.cmds[DrawID].baseVertex;
                out_draws.cmds[idx].baseInstance = 0;
                out_draws.cmds[idx].pos = in_draws.cmds[DrawID].pos;
            }
}
