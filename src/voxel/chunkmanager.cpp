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
                for (int y = 0; y < d * 240; y++)
                    if (y >= posvec.y && y < posvec.y + 64)
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

            UV.x = x;
            UV.y = y;
            UV.z = z;

            color = ColorPalette.v[mat_idx];

            gl_Position = GlobalParams.vp * (vec4(x, y, z, 1) + ChunkOffsets.v[gl_DrawID]);
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

void main(){
            out_color = vec4(fract(UV), 1);
            out_color.a = 1;
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
    PVG::GraphicsDevice::MulitDrawElementsIndirect(PVG::Topology::Triangles, PVG::IndexType::UInt, 16, 0, draw_count);

    glDisable(GL_DEPTH_TEST);
    glBlitNamedFramebuffer(fbuf->GetID(), 0, 0, 0, 1024, 1024, 0, 0, 1024, 1024, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
