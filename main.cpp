#include "core/window.h"
#include "core/input.h"
#include "scenegraph/scenebase.h"
#include "graphics/graphicsdevice.h"
#include "voxel/chunk.h"
#include "voxel/chunk_malloc.h"
#include "voxel/mesh_malloc.h"
#include "voxel/draw_cmdlist.h"

#include <iostream>
#include <random>
#include <chrono>
#include <memory>
#include <windows.h>

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

    PVV::ChunkMalloc chunk_mem;
    PVV::MeshMalloc mesh_mem;
    PVV::Chunk chnks[128];
    PVV::DrawCmdList draw_cmds;
    std::shared_ptr<PVG::ShaderProgram> render_prog;
    std::shared_ptr<PVG::Framebuffer> fbuf;
    PVG::GraphicsPipeline pipeline;
    int draw_count;

    void Initialize() override
    {
        PVSG::SceneBase::Initialize();
        chunk_mem.Initialize();
        mesh_mem.Initialize();
        uint32_t idx_offset = 0;

        draw_cmds.BeginFrame();
        for (int i = 0; i < 128; i++)
        {
            chnks[i].Initialize(&chunk_mem);
            chnks[i].SetSingle(1, 1, 1, 1);

            uint32_t loopback_cntr = 0;
            auto count = chnks[i].GetCompiledLen();
            auto mem_blk = mesh_mem.Alloc(count, &loopback_cntr);
            //chnks[i].Compile(mem_blk);
            for (int j = 0; j < count; j++)
                mem_blk[j] = j;

            if (count > 0)
            {
                draw_cmds.RecordDraw(count, idx_offset, 0, 0, 1);
                idx_offset += count;
            }
        }
        draw_count = draw_cmds.EndFrame();

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

// Output data ; will be interpolated for each fragment.
out vec2 UV;

void main(){
            float x = -1.0f + float((gl_VertexID & 1) << 2);
            float y = -1.0f + float((gl_VertexID & 2) << 1);

            UV.x = (x + 1.0f) * 0.5f;
            UV.y = (y + 1.0f) * 0.5f;

            gl_Position = GlobalParams.vp * vec4(x, y, 0.0f, 1);
})");
        vert.Compile();

        frag.SetSource(
            R"(#version 460

// Interpolated values from the vertex shaders
in vec2 UV;

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
            color = vec4(1);
            color.a = 1;
}
)");
        frag.Compile();

        render_prog = std::make_shared<PVG::ShaderProgram>();
        render_prog->Attach(vert);
        render_prog->Attach(frag);
        render_prog->Link();

        fbuf = PVG::Framebuffer::GetDefault();

        pipeline.SetShaderProgram(render_prog);
        pipeline.SetFramebuffer(fbuf);
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
        PVG::GraphicsDevice::MulitDrawElementsIndirectCount(PVG::Topology::Triangles, PVG::IndexType::UInt, 16, 0, 0, draw_count);
    }
};

int main(int, char **)
{
    PVC::Window win(640, 480, "Test");
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
