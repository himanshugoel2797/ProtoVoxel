#pragma once

#include "chunk.h"
#include "mesh_malloc.h"
#include "chunkupdater.h"
#include "draw_cmdlist.h"
#include "graphics/gpubuffer.h"
#include "graphics/shaderprogram.h"
#include "graphics/framebuffer.h"
#include "graphics/texture.h"
#include "graphics/graphicspipeline.h"
#include "chunkpalette.h"
#include <stdint.h>

namespace ProtoVoxel::Voxel
{
    class ChunkManager
    {
    private:
        static const int GridSide = 64;
        static const int GridHeight = 4;
        static const int GridLen = GridSide * GridSide * GridHeight;

        Chunk chnks[GridLen];
        ProtoVoxel::Voxel::MeshMalloc mesh_mem;
        ProtoVoxel::Voxel::ChunkUpdater chnk_updater;
        ProtoVoxel::Voxel::DrawCmdList draw_cmds;
        std::shared_ptr<ProtoVoxel::Graphics::ShaderProgram> render_prog;
        std::shared_ptr<ProtoVoxel::Graphics::Framebuffer> fbuf;
        std::shared_ptr<ProtoVoxel::Graphics::GpuBuffer> pos_buf;
        ProtoVoxel::Graphics::Texture colorTgt;
        ProtoVoxel::Graphics::Texture depthTgt;
        ProtoVoxel::Graphics::GraphicsPipeline pipeline;
        ProtoVoxel::Voxel::ChunkPalette palette;
        int draw_count;

    public:
        ChunkManager();
        ~ChunkManager();

        void Initialize(ProtoVoxel::Voxel::ChunkPalette &palette);
        void Update(std::weak_ptr<ProtoVoxel::Graphics::GpuBuffer> camera_buffer);
        void Render(double time);
    };
} // namespace ProtoVoxel::Voxel