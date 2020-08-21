#include "core/window.h"
#include "core/input.h"
#include "scenegraph/scenebase.h"
#include "graphics/graphicsdevice.h"
#include "voxel/chunk.h"
#include "voxel/chunk_malloc.h"
#include "voxel/mesh_malloc.h"
#include "voxel/draw_cmdlist.h"
#include "voxel/mortoncode.h"

#include "voxel/PerlinNoise.h"

#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <memory>
#include <windows.h>
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

    PVV::ChunkMalloc chunk_mem;
    PVV::MeshMalloc mesh_mem;
    PVV::Chunk chnks[128];
    PVV::DrawCmdList draw_cmds;
    std::shared_ptr<PVG::ShaderProgram> render_prog;
    std::shared_ptr<PVG::Framebuffer> fbuf;
    PVG::Texture colorTgt;
    PVG::Texture depthTgt;
    PVG::GraphicsPipeline pipeline;
    int draw_count;

    void Initialize() override
    {
        PVSG::SceneBase::Initialize();
        chunk_mem.Initialize();
        mesh_mem.Initialize();
        uint32_t idx_offset = 0;

        siv::PerlinNoise noise(0);

        draw_cmds.BeginFrame();
        for (int i = 0; i < 1; i++)
        {
            chnks[i].Initialize(&chunk_mem);

            for (int x = 0; x < 32; x++)
                for (int z = 0; z < 32; z++)
                    for (int y = 0; y < 64; y++)
                    //for (int q = 0; q < 60000; q++)
                    {
                        //uint8_t x = rand() % 32;
                        //uint8_t y = rand() % 64;
                        //uint8_t z = rand() % 32;
                        auto d = noise.noise3D_0_1(x * 0.012345, y * 0.012345, z * 0.012345);
                        if (d > 0.5)
                            chnks[i].SetSingle(x, y, z, (int)(d * 10));
                    }
            //chnks[i].SetSingle(0, 1, 1, 1);
            //chnks[i].SetSingle(1, 0, 1, 1);
            //chnks[i].SetSingle(1, 1, 0, 1);
            //chnks[i].SetSingle(1, 1, 1, 1);

            auto raw_Data_vis = (uint64_t *)chnks[i].vismasks;
            auto raw_Data = (uint8_t *)chnks[i].GetRawData();
            auto raw_Data_u64 = (uint64_t *)chnks[i].GetRawData();

            uint8_t comp_data_space[32768] = {0};
            int rle_len = 0;
            int rle_len_packed = 0;
            const int chnk_len = 32 * 32 * 64;
            {
                auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
                for (int q = 0; q < 10000; q++)
                {
                    rle_len = 0;

                    uint64_t cur_val = 0;
                    uint64_t next_val = raw_Data_u64[0];
                    uint32_t c_runLen = 0;
                    uint32_t c_val = next_val & 0xff;
                    uint8_t *comp_ptr = (uint8_t *)comp_data_space;
                    for (int j = 1; j < chnk_len / sizeof(uint64_t); j++)
                    {
                        cur_val = next_val;
                        next_val = raw_Data_u64[j];
                        uint64_t cur_val_shifted = (cur_val >> 8) | (next_val << 56);
                        uint64_t equality_mask = cur_val_shifted ^ cur_val;
                        //count matched parts
                        if (equality_mask == 0)
                        {
                            uint8_t proc_byte = cur_val;
                            if (c_val == proc_byte && c_runLen + 64 <= 256 * 8)
                                c_runLen += 64;
                            else
                            {
                                *(comp_ptr++) = c_runLen / 8 - 1;
                                *(comp_ptr++) = c_val;

                                c_runLen = 64;
                                c_val = proc_byte;
                            }
                        }
                        else
                            for (int32_t avail_bits = 64; avail_bits != 0;)
                            {
                                uint8_t proc_byte = cur_val;
                                int32_t matched_bits = __tzcnt_u64(equality_mask) & ~7;
                                matched_bits += 8;
                                if (matched_bits > avail_bits)
                                    matched_bits = avail_bits;

                                if (c_val == proc_byte && c_runLen + matched_bits <= 256 * 8)
                                    c_runLen += matched_bits;
                                else
                                {
                                    *(comp_ptr++) = c_runLen / 8 - 1;
                                    *(comp_ptr++) = c_val;

                                    c_runLen = matched_bits;
                                    c_val = proc_byte;
                                }

                                avail_bits -= matched_bits;
                                cur_val >>= matched_bits;
                                equality_mask >>= matched_bits;
                                equality_mask &= ~0xff;
                            }
                    }

                    cur_val = next_val;
                    uint64_t cur_val_shifted = (cur_val >> 8);
                    uint64_t equality_mask = cur_val_shifted ^ cur_val;
                    //count matched parts
                    if (equality_mask == 0)
                    {
                        uint8_t proc_byte = cur_val;
                        if (c_val == proc_byte && c_runLen + 64 <= 256 * 8)
                            c_runLen += 64;
                        else
                        {
                            *(comp_ptr++) = c_runLen / 8 - 1;
                            *(comp_ptr++) = c_val;

                            //last write, no need to save values further
                            c_runLen = 64;
                            c_val = proc_byte;
                        }
                    }
                    else
                        for (int32_t avail_bits = 64; avail_bits != 0;)
                        {
                            uint8_t proc_byte = cur_val;
                            int32_t matched_bits = __tzcnt_u64(equality_mask) & ~7;
                            matched_bits += 8;
                            if (matched_bits > avail_bits)
                                matched_bits = avail_bits;

                            if (c_val == proc_byte && c_runLen + matched_bits <= 256 * 8)
                                c_runLen += matched_bits;
                            else
                            {
                                *(comp_ptr++) = c_runLen / 8 - 1;
                                *(comp_ptr++) = c_val;

                                c_runLen = matched_bits;
                                c_val = proc_byte;
                            }

                            avail_bits -= matched_bits;
                            cur_val >>= matched_bits;
                            equality_mask >>= matched_bits;
                            equality_mask &= ~0xff;
                        }
                    if (c_runLen != 0)
                    {
                        *(comp_ptr++) = c_runLen / 8 - 1;
                        *(comp_ptr++) = c_val;
                    }

                    rle_len = comp_ptr - comp_data_space;
                }
                auto stop = std::chrono::high_resolution_clock::now().time_since_epoch().count();
                std::cout << "Custom RLE Len: " << rle_len << std::endl;
                std::cout << "Set Voxel Count: " << chnks[i].GetVoxelCount() << std::endl;
                std::cout << "Encode Time Taken: " << (stop - start) / (1000.0 * 10000) << "us" << std::endl;
            }
            {
                auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
                uint8_t decomp_pool[64 * 1024];
                for (int q = 0; q < 10000; q++)
                {
                    uint8_t *tmp_ptr = (uint8_t *)decomp_pool;
                    uint64_t *comp_ptr = (uint64_t *)comp_data_space;
                    uint32_t rle_len2 = (rle_len + 7) / 8;
                    uint32_t iter = 0;

                    for (int j = 0; j < rle_len2; j++)
                    {
                        uint64_t v_net = *(comp_ptr++);

                        int32_t len;
                        uint8_t v;
                        uint64_t v_64;
                        uint64_t *tmp_ptr_64;
                        uint64_t mask;
                        uint64_t v_64_s;

                        //Align the value being written so the pointer can be aligned
                        //std::cout << "len: " << std::dec << len << " v: " << v << " iter: " << iter << std::hex << " v_64_s: " << v_64_s << std::endl;
#define INNER_LOOP(off)                              \
    len = ((v_net >> (off * 8)) & 0xff) + 1;         \
    v = (v_net >> ((off + 1) * 8)) & 0xff;           \
    v_64 = v * 0x0101010101010101;                   \
    tmp_ptr_64 = (uint64_t *)&tmp_ptr[iter & ~7];    \
    mask = 0xffffffffffffffff << ((iter & 0x7) * 8); \
    v_64_s = v_64 & mask;                            \
    v_64_s |= (*tmp_ptr_64) & ~mask;                 \
    *(tmp_ptr_64++) = v_64_s;                        \
    iter += len;                                     \
    len = (len + 7) / 8;                             \
    while (len-- > 0)                                \
        *(tmp_ptr_64++) = v_64;

                        INNER_LOOP(0)
                        INNER_LOOP(2)
                        INNER_LOOP(4)
                        INNER_LOOP(6)
                    }

                    //for (int j = 0; j < 64 * 1024; j++)
                    //    if (decomp_pool[j] != raw_Data[j])
                    //    {
                    //        std::cout << "Not matched at " << j << " Value found decomp[j]: " << (int)decomp_pool[j] << "\r\n";
                    //        break;
                    //    }
                }
                auto stop = std::chrono::high_resolution_clock::now().time_since_epoch().count();
                std::cout << "Decode Time Taken: " << (stop - start) / (1000.0 * 10000) << "us" << std::endl;
            }

            //std::cout << __bextr_u64(0x0123456789abcdef, 0x808) << std::endl;

            auto fi = fopen("chnk.bin", "wb");
            //fwrite(raw_Data_vis, 1, 4096, fi);
            //fwrite(raw_Data, 1, 32 * 32 * 64, fi);
            fwrite(comp_data_space, 1, rle_len, fi);
            fclose(fi);
            fi = fopen("chnk_raw.bin", "wb");
            //fwrite(raw_Data_vis, 1, 4096, fi);
            fwrite(raw_Data, 1, 32 * 32 * 64, fi);
            //fwrite(comp_data_space2, 1, rle_len_packed, fi);
            fclose(fi);

            uint32_t loopback_cntr = 0;
            auto count = chnks[i].GetCompiledLen();
            auto mem_blk = mesh_mem.Alloc(count, &loopback_cntr);
            chnks[i].Compile(mem_blk);

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

layout(std140, binding = 1) uniform ChunkOffsets_t{
        vec4 v[4096];
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

            gl_Position = GlobalParams.vp * vec4(x, y, z, 1);
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

        fbuf = std::make_shared<PVG::Framebuffer>(640, 480);

        colorTgt.SetStorage(GL_TEXTURE_2D, 1, GL_RGBA8, 640, 480);
        depthTgt.SetStorage(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, 640, 480);

        GLenum fbuf_drawbufs[] = {GL_COLOR_ATTACHMENT0};
        fbuf->Attach(GL_COLOR_ATTACHMENT0, colorTgt, 0);
        fbuf->Attach(GL_DEPTH_ATTACHMENT, depthTgt, 0);
        fbuf->DrawBuffers(1, fbuf_drawbufs);

        pipeline.SetDepth(0);
        pipeline.SetDepthTest(GL_GEQUAL);
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
        PVG::GraphicsDevice::ClearAll();
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        PVG::GraphicsDevice::MulitDrawElementsIndirectCount(PVG::Topology::Triangles, PVG::IndexType::UInt, 16, 0, 0, draw_count);

        glDisable(GL_DEPTH_TEST);
        glBlitNamedFramebuffer(fbuf->GetID(), 0, 0, 0, 640, 480, 0, 0, 640, 480, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

    /*{
        double avg_duration = 0;
        int sample_cnt = 10000;
        int iter_cnt = 60000;

        auto x_v_2 = new uint8_t[sample_cnt * iter_cnt * 3];
        for (int i = 0; i < sample_cnt * iter_cnt; i++)
        {
            uint8_t x = rand() % 32;
            uint8_t y = rand() % 64;
            uint8_t z = rand() % 32;

            x_v_2[i * 3 + 0] = x;
            x_v_2[i * 3 + 1] = y;
            x_v_2[i * 3 + 2] = z;
        }

        PVV::ChunkMalloc chnk_malloc;
        chnk_malloc.Initialize();

        auto x_v = (uint8_t *)x_v_2;
        for (int samples = 0; samples < sample_cnt; samples++)
        {
            PVV::Chunk chnk;
            chnk.Initialize(&chnk_malloc);

            auto start_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            for (int i = 0; i < iter_cnt; i++)
            {
                uint8_t x = *x_v++;
                uint8_t y = *x_v++;
                uint8_t z = *x_v++;

                chnk.SetSingle(x, y, z, 1);
            }
            auto stop_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();

            avg_duration += (stop_ns - start_ns) / 1000.0;
        }
        std::cout << "[SetSingle] Duration: " << avg_duration / sample_cnt << "us, Per Insert: " << avg_duration / (sample_cnt * iter_cnt) * 1000 << "ns" << std::endl;

        avg_duration = 0;
        x_v = (uint8_t *)x_v_2;
        for (int samples = 0; samples < sample_cnt; samples++)
        {
            PVV::Chunk chnk;
            chnk.Initialize(&chnk_malloc);

            for (int i = 0; i < iter_cnt; i++)
            {
                uint8_t x = *x_v++;
                uint8_t y = *x_v++;
                uint8_t z = *x_v++;

                chnk.SetSingle(x, y, z, 1);
            }

            auto start_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            auto resvec = new uint32_t[chnk.GetCompiledLen()];
            chnk.Compile(resvec);
            auto stop_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            delete[] resvec;

            avg_duration += (stop_ns - start_ns) / 1000.0;
        }
        std::cout << "[SetSingle + UpdateMasks] Duration: " << avg_duration / sample_cnt << "us, Per Insert: " << avg_duration / (sample_cnt * iter_cnt) * 1000 << "ns" << std::endl;
    }*/

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
