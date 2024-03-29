#pragma once

#include "chunk.h"
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

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cuda_gl_interop.h>

#define  POS_SCALE_FACTOR 10000.0f

namespace ProtoVoxel::PointCloud
{
    class ChunkManager
    {
    private:
        static const int GridSide = 64;
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
        ProtoVoxel::PointCloud::ChunkJobManager jobManager;
        ProtoVoxel::PointCloud::DrawCmdList draw_cmds;
        ProtoVoxel::PointCloud::MeshMalloc mesh_mem;
        
        ProtoVoxel::Graphics::Framebuffer* fbuf;
        ProtoVoxel::Graphics::Texture colorTgt;

        ProtoVoxel::Graphics::Framebuffer* fbuf0;
        ProtoVoxel::Graphics::Texture colorTgt0;

        ProtoVoxel::Graphics::Texture* colorTgt_cur;
        ProtoVoxel::Graphics::Texture* colorTgt_n;
        ProtoVoxel::Graphics::Framebuffer* fbuf_cur;
        ProtoVoxel::Graphics::Framebuffer* fbuf_n;
        
        ProtoVoxel::Graphics::ComputePipeline splatPipeline;
        ProtoVoxel::Graphics::ShaderProgram splat_prog;
        ProtoVoxel::Graphics::GpuBuffer splat_cmdbuffer;
        ProtoVoxel::Graphics::Texture pointBuffer;

        ProtoVoxel::Graphics::GraphicsPipeline resolvePipeline;
        ProtoVoxel::Graphics::ShaderProgram resolve_prog;

        int draw_count;

        void splat(void* global_vars, void* splat_points, void* pointBuffer, cudaSurfaceObject_t colorBuffer, int w, int h, int draw_count);

    public:
        ChunkManager();
        ~ChunkManager();

        void Initialize();
        void Update(glm::vec4 camPos, glm::mat4 vp, ProtoVoxel::Graphics::GpuBuffer* camera_buffer);
        void Render(ProtoVoxel::Graphics::GpuBuffer* camera_buffer, double time);
    };
} // namespace ProtoVoxel::Voxel