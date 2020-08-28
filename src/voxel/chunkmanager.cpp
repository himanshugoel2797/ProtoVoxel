#include "chunkmanager.h"
#include <chrono>
#include <iostream>
#include "graphics/graphicsdevice.h"
#include "core/freecamera.h"
#include "voxel/PerlinNoise.h"

//for now just a fixed size grid
//chunkjobmanager class manages per thread resources for chunks like temporary memory to which chunks are decompressed
//only chunks in a small radius around an active object are kept uncompressed at any time
//

namespace PVV = ProtoVoxel::Voxel;
namespace PVG = ProtoVoxel::Graphics;

PVV::ChunkManager::ChunkManager()
{
}

PVV::ChunkManager::~ChunkManager() {}

void PVV::ChunkManager::Initialize(ChunkPalette &palette)
{
    this->palette = palette;
    pos_buf = std::make_shared<PVG::GpuBuffer>();
    pos_buf->SetStorage(4 * sizeof(uint32_t) * GridLen, GL_DYNAMIC_STORAGE_BIT);
    glm::ivec4 positions[GridLen];
    mesh_mem.Initialize();
    uint32_t idx_offset = 0;
    uint32_t pos_offset = 0;

    siv::PerlinNoise noise(0);

    auto startTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    draw_cmds.BeginFrame();
    for (int i = 0; i < GridLen; i++)
    {
        auto posvec = glm::ivec3((i % 64) * 30, (i / (64 * 64)) * 62, ((i / 64) % 64) * 30);
        positions[pos_offset] = glm::ivec4(posvec, 0);
        chnks[i].Initialize();
        chnks[i].SetPosition(posvec);

        chnk_updater.UnpackChunk(&chnks[i]);
        for (int x = -1; x < 31; x++)
            for (int z = -1; z < 31; z++)
            //                for (int y = 0; y < 64; y++)
            {
                auto d = noise.noise3D_0_1((posvec.x + x) * 0.005, 0 * 0.005, (posvec.z + z) * 0.005);
                //if (d > 0.5)
                for (int y = posvec.y; y < d * 240 && y < posvec.y + 64; y++)
                    chnk_updater.SetBlock(x + 1, y - posvec.y, z + 1, 1);
            }

        uint32_t loopback_cntr = 0;
        auto count = chnk_updater.GetCompiledLength();
        auto mem_blk = mesh_mem.Alloc(count, &loopback_cntr);
        auto real_len = chnk_updater.Compile(mem_blk);
        mesh_mem.Flush(idx_offset, real_len, mem_blk);
        mesh_mem.FreeRear(count - real_len);

        if (count > 0)
        {
            draw_cmds.RecordDraw(count, idx_offset, 0, 0, 1);
            idx_offset += count;
            pos_offset++;
        }
    }
    draw_count = draw_cmds.EndFrame();
    auto stopTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::cout << "Generation time: " << (stopTime - startTime) / 1000000.0 << "ms" << std::endl;
    std::cout << "Splatted Voxels: " << idx_offset << std::endl;
    pos_buf->Update(0, 16 * GridLen, positions);

    PVG::ShaderSource vert(GL_VERTEX_SHADER), frag(GL_FRAGMENT_SHADER);
    vert.SetSource(
        R"(#version 460

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
} GlobalParams;

layout(std430, binding = 1) buffer readonly restrict ChunkOffsets_t{
        ivec4 v[];
} ChunkOffsets;

layout(std140, binding = 2) uniform ColorPalette_t{
        vec4 v[256];
} ColorPalette;

// Output data ; will be interpolated for each fragment.
out vec3 UV;
out vec4 color;

void main(){
            float x = float((gl_VertexID >> 19) & 0x1f);
            float y = float((gl_VertexID >> 8) & 0x3f);
            float z = float((gl_VertexID >> 14) & 0x1f);

            int mat_idx = int((gl_VertexID >> 0) & 0x7f);

            UV.x = x + ChunkOffsets.v[gl_DrawID].x;
            UV.y = y + ChunkOffsets.v[gl_DrawID].y;
            UV.z = z + ChunkOffsets.v[gl_DrawID].z;

            color = ColorPalette.v[mat_idx];

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
            gl_PointSize = 1024.0f * max_radius * 1.1f;
            
            gl_Position = GlobalParams.vp * vec4(UV, 1);
})");
    vert.Compile();

    frag.SetSource(
        R"(#version 460

// Interpolated values from the vertex shaders
in vec3 UV;
in vec4 color;

// Ouput data
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
} GlobalParams;

bool boxIntersection( vec3 ro, vec3 rd, vec3 aabb_min, vec3 aabb_max, out float t0, out float t1 ) 
{
    vec3 invR = 1.0f / rd;
    vec3 tmin = invR * (aabb_min - ro);
    vec3 tmax = invR * (aabb_max - ro);
    vec3 t1_ = min(tmin, tmax);
    vec3 t2_ = max(tmin, tmax);

    t0 = max(max(t1_.x, t1_.y), t1_.z);
    t1 = min(min(t2_.x, t2_.y), t2_.z);
    return t0 <= t1;
}

void main(){
    vec3 eyePos = GlobalParams.eyePos.xyz;
    vec2 screenPos = 2.0f * gl_FragCoord.xy / 1024.0f - 1.0f;
    vec3 rayDir = vec3(screenPos.x, screenPos.y, 0.01f);
    vec4 pPos = (GlobalParams.ivp * vec4(rayDir, 1));
    pPos /= pPos.w;

    rayDir = (pPos.xyz - eyePos);

    float t0;
    float t1;

    //out_color = vec4(rayDir * 0.5f + 0.5f, 1);
    bool intersected = boxIntersection(eyePos, rayDir, UV - vec3(0.5f), UV + vec3(0.5f), t0, t1);
    if( intersected ){
        vec3 intersection = eyePos + rayDir * t0;
        vec3 n = normalize(intersection - UV);

        vec3 abs_n = abs(n);
        float max_n = max(abs_n.x, max(abs_n.y, abs_n.z));
        n = step(max_n, abs_n) * sign(n);

        vec4 trans_pos = GlobalParams.vp * vec4(intersection, 1);
        gl_FragDepth = trans_pos.z / trans_pos.w;
        out_color = vec4(n * 0.5f + 0.5f, 1);
    }else
        discard;

    //out_color = vec4(1, 0, 0, 1);
}
)");
    frag.Compile();

    render_prog = std::make_shared<PVG::ShaderProgram>();
    render_prog->Attach(vert);
    render_prog->Attach(frag);
    render_prog->Link();

    fbuf = std::make_shared<PVG::Framebuffer>(1024, 1024);

    colorTgt.SetStorage(GL_TEXTURE_2D, 1, GL_RGBA8, 1024, 1024);
    depthTgt.SetStorage(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, 1024, 1024);

    GLenum fbuf_drawbufs[] = {GL_COLOR_ATTACHMENT0};
    fbuf->Attach(GL_COLOR_ATTACHMENT0, colorTgt, 0);
    fbuf->Attach(GL_DEPTH_ATTACHMENT, depthTgt, 0);
    fbuf->DrawBuffers(1, fbuf_drawbufs);

    pipeline.SetDepth(0);
    pipeline.SetDepthTest(GL_GEQUAL);
    pipeline.SetShaderProgram(render_prog);
    pipeline.SetFramebuffer(fbuf);
    pipeline.SetSSBO(1, pos_buf, 0, 4 * sizeof(float) * GridLen);
    pipeline.SetUBO(2, palette.GetBuffer(), 0, 4 * sizeof(float) * 256);
    pipeline.SetIndirectBuffer(draw_cmds.GetBuffer(), 0, PVV::DrawCmdList::ListSize);
    pipeline.SetIndexBuffer(mesh_mem.GetBuffer(), 0, PVV::DrawCmdList::ListSize);
}

void PVV::ChunkManager::Update(std::weak_ptr<PVG::GpuBuffer> camera_buffer)
{
    pipeline.SetUBO(0, camera_buffer, 0, sizeof(ProtoVoxel::Core::GlobalParameters));
}

void PVV::ChunkManager::Render(double time)
{
    PVG::GraphicsDevice::BindGraphicsPipeline(pipeline);
    PVG::GraphicsDevice::ClearAll();
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_PROGRAM_POINT_SIZE);
    PVG::GraphicsDevice::MulitDrawElementsIndirect(PVG::Topology::Points, PVG::IndexType::UInt, 16, 0, draw_count);

    glDisable(GL_DEPTH_TEST);
    glBlitNamedFramebuffer(fbuf->GetID(), 0, 0, 0, 1024, 1024, 0, 0, 1024, 1024, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
