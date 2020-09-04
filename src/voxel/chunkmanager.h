#pragma once

#include "chunk.h"
#include "chunkpalette.h"
#include "chunkupdater.h"
#include "draw_cmdlist.h"
#include "glm/glm.hpp"
#include "chunkjobmanager.h"
#include "graphics/framebuffer.h"
#include "graphics/gpubuffer.h"
#include "graphics/graphicspipeline.h"
#include "graphics/computepipeline.h"
#include "graphics/shaderprogram.h"
#include "graphics/texture.h"
#include "graphics/hiz.h"
#include "mesh_malloc.h"
#include <stdint.h>

namespace ProtoVoxel::Voxel
{
    class ChunkManager
    {
    private:
        static const int GridSide = 64;
        static const int GridHeight = 32;
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
        ProtoVoxel::Voxel::ChunkJobManager jobManager;
        ProtoVoxel::Voxel::ChunkPalette palette;
        ProtoVoxel::Voxel::DrawCmdList draw_cmds;
        ProtoVoxel::Voxel::MeshMalloc mesh_mem;

        ProtoVoxel::Graphics::GraphicsPipeline pipeline;
        ProtoVoxel::Graphics::ShaderProgram render_prog;
        
        ProtoVoxel::Graphics::Framebuffer* fbuf;
        ProtoVoxel::Graphics::Texture colorTgt;
        ProtoVoxel::Graphics::Texture depthTgt;
        
        ProtoVoxel::Graphics::HiZ prev_mip_pyramid;
        ProtoVoxel::Graphics::HiZ cur_mip_pyramid;

        ProtoVoxel::Graphics::ComputePipeline bucketPipeline;
        ProtoVoxel::Graphics::ShaderProgram bucket_prog;
        ProtoVoxel::Graphics::GpuBuffer out_draw_buffer;
        ProtoVoxel::Graphics::GpuBuffer out_splats_buffer;

        ProtoVoxel::Graphics::ComputePipeline splatPipeline;
        ProtoVoxel::Graphics::ShaderProgram splat_prog;
        ProtoVoxel::Graphics::Texture pointBuffer;

        ProtoVoxel::Graphics::GraphicsPipeline resolvePipeline;
        ProtoVoxel::Graphics::ShaderProgram resolve_prog;

        ProtoVoxel::Graphics::GpuBuffer out_occluded_draw_buf;
        ProtoVoxel::Graphics::ShaderProgram occludedCheck_prog;
        ProtoVoxel::Graphics::ComputePipeline occludedTestPipeline;
        ProtoVoxel::Graphics::GraphicsPipeline occludedPipeline;

        int draw_count;

    public:
        ChunkManager();
        ~ChunkManager();

        void Initialize(ProtoVoxel::Voxel::ChunkPalette &palette);
        void Update(glm::vec4 camPos, glm::mat4 vp, ProtoVoxel::Graphics::GpuBuffer* camera_buffer);
        void Render(ProtoVoxel::Graphics::GpuBuffer* camera_buffer, double time);
    };
} // namespace ProtoVoxel::Voxel