#include "draw_cmdlist.h"
#include <string.h>

#if DEBUG
#include <iostream>
#endif

namespace PPC = ProtoVoxel::PointCloud;

PPC::DrawCmdList::DrawCmdList()
{
    cmd_list.SetStorage(sizeof(draw_cmd_list_t), GL_DYNAMIC_STORAGE_BIT);
    memset(&list, 0, sizeof(draw_cmd_list_t));

    draw_cmd_ptr = nullptr;
}

PPC::DrawCmdList::~DrawCmdList()
{
}

void PPC::DrawCmdList::BeginFrame()
{
    list.draw_cnt = 0;
    draw_count = 0;
    draw_cmd_ptr = list.cmds;

    list.pd0 = 1;
    list.pd1 = 1;
    list.pd2 = 1;

}

void PPC::DrawCmdList::RecordDraw(uint32_t count, uint32_t firstIndex, uint32_t baseVertex, uint32_t baseInstance, uint32_t instanceCount, glm::ivec3 &pos, glm::ivec3 &min_bnd, glm::ivec3 &max_bnd)
{
#ifdef DEBUG
    //if (list.draw_cnt >= DrawCmdCount - 1)
    //{
    //    std::cout << "Error: Exceeding draw cmd count." << std::endl;
    //    return;
    //}
#endif

    if (count == 0)
        return;

    uint32_t idx = draw_count++;

    draw_cmd_ptr[idx].count = count;
    draw_cmd_ptr[idx].instanceCount = instanceCount;
    //draw_cmd_ptr->firstIndex = firstIndex;
    draw_cmd_ptr[idx].baseVertex = baseVertex;
    draw_cmd_ptr[idx].baseInstance = baseInstance;
    draw_cmd_ptr[idx].position = glm::ivec4(pos, 0);
    draw_cmd_ptr[idx].min_bnd = glm::ivec4(min_bnd, 0);
    draw_cmd_ptr[idx].max_bnd = glm::ivec4(max_bnd, 0);
}

uint32_t PPC::DrawCmdList::EndFrame()
{
    auto cnt = list.draw_cnt = draw_count;
    draw_cmd_ptr = nullptr; //Ensures a segfault if any further writes are attempted
    cmd_list.Update(0, sizeof(draw_cmd_t) * cnt + 4 * sizeof(uint32_t), &list);
    return cnt;
}

ProtoVoxel::Graphics::GpuBuffer* PPC::DrawCmdList::GetBuffer()
{
    return &cmd_list;
}