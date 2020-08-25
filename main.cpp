#include "core/input.h"
#include "core/window.h"
#include "graphics/gpubuffer.h"
#include "graphics/graphicsdevice.h"
#include "scenegraph/scenebase.h"
#include "voxel/chunk.h"
#include "voxel/chunkupdater.h"
#include "voxel/draw_cmdlist.h"
#include "voxel/mesh_malloc.h"
#include "voxel/mortoncode.h"

#include "voxel/PerlinNoise.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <x86intrin.h>

namespace PVC = ProtoVoxel::Core;
namespace PVG = ProtoVoxel::Graphics;
namespace PVV = ProtoVoxel::Voxel;
namespace PVSG = ProtoVoxel::SceneGraph;

class TestScene : public PVSG::SceneBase
{
private:
public:
    TestScene() {}
    ~TestScene() {}

    PVV::MeshMalloc mesh_mem;
    PVV::Chunk chnks[1024];
    PVV::ChunkUpdater chnk_updater;
    PVV::DrawCmdList draw_cmds;
    std::shared_ptr<PVG::ShaderProgram> render_prog;
    std::shared_ptr<PVG::Framebuffer> fbuf;
    std::shared_ptr<PVG::GpuBuffer> pos_buf;
    PVG::Texture colorTgt;
    PVG::Texture depthTgt;
    PVG::GraphicsPipeline pipeline;
    int draw_count;

    void Initialize() override
    {
        PVSG::SceneBase::Initialize();
        pos_buf = std::make_shared<PVG::GpuBuffer>();
        pos_buf->SetStorage(4 * sizeof(uint32_t) * 1024, GL_DYNAMIC_STORAGE_BIT);
        glm::ivec4 positions[1024];
        mesh_mem.Initialize();
        uint32_t idx_offset = 0;

        siv::PerlinNoise noise(0);

        auto startTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        draw_cmds.BeginFrame();
        for (int i = 0; i < 512; i++)
        {
            auto posvec = glm::ivec3((i % 32) * 31 - 1, 0, (i / 32) * 31 - 1);
            positions[i] = glm::ivec4(posvec, 0);
            chnks[i].Initialize();
            chnks[i].SetPosition(posvec);

            chnk_updater.UnpackChunk(&chnks[i]);
            for (int x = 0; x < 32; x++)
                for (int z = 0; z < 32; z++)
                //for (int y = 0; y < 64; y++)
                //for (int q = 0; q < 60000; q++)
                {
                    //uint8_t x = rand() % 32;
                    //uint8_t y = rand() % 64;
                    //uint8_t z = rand() % 32;
                    auto d = noise.noise3D_0_1((posvec.x + x) * 0.05, 0.05, (posvec.z + z) * 0.05);
                    //if (d > 0.5)
                    chnk_updater.SetBlock(x, (uint8_t)(d * 50), z, 1);
                }
            //chnks[i].SetSingle(0, 1, 1, 1);
            //chnks[i].SetSingle(1, 0, 1, 1);
            //chnks[i].SetSingle(1, 1, 1, 1);
            //chnks[i].SetSingle(1, 2, 1, 1);
            //chnks[i].SetSingle(1, 3, 1, 1);
            //chnks[i].SetSingle(1, 4, 1, 1);

            uint32_t loopback_cntr = 0;
            auto count = chnk_updater.GetCompiledLength();
            auto mem_blk = mesh_mem.Alloc(count, &loopback_cntr);
            chnk_updater.Compile(mem_blk);

            if (count > 0)
            {
                draw_cmds.RecordDraw(count, idx_offset, 0, 0, 1);
                idx_offset += count;
            }
        }
        draw_count = draw_cmds.EndFrame();
        auto stopTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::cout << "Generation time: " << (stopTime - startTime) / 1000000.0 << "ms" << std::endl;
        pos_buf->Update(0, 16 * 1024, positions);

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

layout(std140, binding = 1) uniform ChunkOffsets_t{
        ivec4 v[1024];
} ChunkOffsets;

// Output data ; will be interpolated for each fragment.
out vec3 UV;

void main(){
            float x = float((gl_VertexID >> 19) & 0x1f);
            float y = float((gl_VertexID >> 8) & 0x3f);
            float z = float((gl_VertexID >> 14) & 0x1f);

            UV.x = mod(x, 2);
            UV.y = mod(y, 2);
            UV.z = mod(z, 2);

            gl_Position = GlobalParams.vp * vec4(x + ChunkOffsets.v[gl_DrawID].x, y + ChunkOffsets.v[gl_DrawID].y, z + ChunkOffsets.v[gl_DrawID].z, 1);
})");
        vert.Compile();

        frag.SetSource(
            R"(#version 460

// Interpolated values from the vertex shaders
in vec3 UV;

// Ouput data
layout(location = 0) out vec4 color;

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
            color = vec4(UV, 1);
            color.a = 1;
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
        pipeline.SetUBO(1, pos_buf, 0, 4 * sizeof(float) * 1024);
        pipeline.SetIndirectBuffer(draw_cmds.GetBuffer(), 0, PVV::DrawCmdList::ListSize);
        pipeline.SetIndexBuffer(mesh_mem.GetBuffer(), 0, PVV::DrawCmdList::ListSize);

        std::cout << "Draw Count: " << draw_count << std::endl;
    }

    void Update(double time) override
    {
        pipeline.SetUBO(0, PVSG::SceneBase::camera_buffer.GetBuffer(), 0, sizeof(PVC::GlobalParameters));

        PVSG::SceneBase::Update(time);
    }

    void Render(double time) override
    {
        PVG::GraphicsDevice::BindGraphicsPipeline(pipeline);
        PVG::GraphicsDevice::ClearAll();
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        PVG::GraphicsDevice::MulitDrawElementsIndirectCount(PVG::Topology::Triangles, PVG::IndexType::UInt, 16, 0, 0, draw_count);

        glDisable(GL_DEPTH_TEST);
        glBlitNamedFramebuffer(fbuf->GetID(), 0, 0, 0, 1024, 1024, 0, 0, 1024, 1024, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};

int main(int, char **)
{
    PVC::Window win(1024, 1024, "Test");
    win.InitGL();

    PVC::Input::RegisterWindow(&win);

    TestScene scene;
    scene.Initialize();

    srand(0);

    double lastTime = 0, nowTime = 0;
    while (!win.ShouldClose())
    {
        PVG::GraphicsDevice::ClearAll();
        win.StartFrame();
        auto delta = (nowTime - lastTime) * 1000;

        scene.UpdateStart(delta);
        scene.Update(delta);
        scene.Render(delta);

        win.SwapBuffers();
        lastTime = nowTime;
        nowTime = win.GetTime();
    }
}
