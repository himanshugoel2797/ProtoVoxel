#pragma once

#include "chunk.h"
#include "chunkpalette.h"
#include "chunkupdater.h"
#include "draw_cmdlist.h"
#include "glm/glm.hpp"
#include "graphics/framebuffer.h"
#include "graphics/gpubuffer.h"
#include "graphics/graphicspipeline.h"
#include "graphics/shaderprogram.h"
#include "graphics/texture.h"
#include "mesh_malloc.h"
#include <stdint.h>

namespace ProtoVoxel::Voxel
{
    class ChunkManager
    {
    private:
        static const int GridSide = 32;
        static const int GridHeight = 16;
        static const int GridLen = GridSide * GridSide * GridHeight;

        struct draw_data_t
        {
            uint32_t start_idx;
            uint32_t len;
            glm::ivec3 pos;
        };
        std::vector<struct draw_data_t> draws;
        glm::ivec4 positions[GridLen];

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
        void Update(glm::vec4 camPos, std::weak_ptr<ProtoVoxel::Graphics::GpuBuffer> camera_buffer);
        void Render(double time);
    };
} // namespace ProtoVoxel::Voxel