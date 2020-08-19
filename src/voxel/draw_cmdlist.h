#pragma once
#include <stdint.h>
#include <memory>
#include "../graphics/gpubuffer.h"

namespace ProtoVoxel::Voxel
{
    class DrawCmdList
    {
    public:
        static const int DrawCmdCount = 16 * 1024;

    private:
        typedef struct
        {
            uint32_t count;
            uint32_t instanceCount;
            uint32_t firstIndex;
            uint32_t baseVertex;
            uint32_t baseInstance;
        } draw_cmd_t;

        typedef struct
        {
            uint32_t draw_cnt;
            uint32_t pd0;
            uint32_t pd1;
            uint32_t pd2;
            draw_cmd_t cmds[DrawCmdCount];
        } draw_cmd_list_t;

        std::shared_ptr<ProtoVoxel::Graphics::GpuBuffer> cmd_list;
        draw_cmd_list_t list;
        draw_cmd_t *draw_cmd_ptr;

    public:
        static const int ListSize = sizeof(draw_cmd_list_t);

        DrawCmdList();
        ~DrawCmdList();

        void BeginFrame();
        void RecordDraw(uint32_t count, uint32_t firstIndex, uint32_t baseVertex, uint32_t baseInstance, uint32_t instanceCount);
        uint32_t EndFrame();
        std::weak_ptr<ProtoVoxel::Graphics::GpuBuffer> GetBuffer();
    };
} // namespace ProtoVoxel::Voxel