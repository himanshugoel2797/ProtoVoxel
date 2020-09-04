#pragma once
#include "graphics/gpubuffer.h"
#include <memory>
#include <mutex>
#include <atomic>
#include "glm/glm.hpp"
#include <stdint.h>

namespace ProtoVoxel::Voxel
{
    class DrawCmdList
    {
    public:
        static const int DrawCmdCount = 512 * 1024;

    private:
        typedef struct
        {
            uint32_t count;
            uint32_t instanceCount;
            uint32_t baseVertex;
            uint32_t baseInstance;
            glm::ivec4 position;
            glm::ivec4 min_bnd;
            glm::ivec4 max_bnd;
        } draw_cmd_t;

        typedef struct
        {
            uint32_t draw_cnt;
            uint32_t pd0;
            uint32_t pd1;
            uint32_t pd2;
            draw_cmd_t cmds[DrawCmdCount];
        } draw_cmd_list_t;

        ProtoVoxel::Graphics::GpuBuffer cmd_list;
        draw_cmd_list_t list;
        draw_cmd_t *draw_cmd_ptr;
        std::atomic_uint32_t draw_count;

    public:
        static const int ListSize = sizeof(draw_cmd_list_t);
        static const int Stride = sizeof(draw_cmd_t);

        DrawCmdList();
        ~DrawCmdList();

        void BeginFrame();
        void RecordDraw(uint32_t count, uint32_t firstIndex, uint32_t baseVertex, uint32_t baseInstance, uint32_t instanceCount, glm::ivec3 &pos, glm::ivec3 &min_bnd, glm::ivec3 &max_bnd);
        uint32_t EndFrame();
        ProtoVoxel::Graphics::GpuBuffer* GetBuffer();
    };
} // namespace ProtoVoxel::Voxel