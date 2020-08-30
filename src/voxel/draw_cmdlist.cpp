#include "draw_cmdlist.h"
#include <string.h>

#if DEBUG
#include <iostream>
#endif

namespace PVV = ProtoVoxel::Voxel;

PVV::DrawCmdList::DrawCmdList()
{
    cmd_list.SetStorage(sizeof(draw_cmd_list_t), GL_DYNAMIC_STORAGE_BIT);
    memset(&list, 0, sizeof(draw_cmd_list_t));

    draw_cmd_ptr = nullptr;
}

PVV::DrawCmdList::~DrawCmdList()
{
}

void PVV::DrawCmdList::BeginFrame()
{
    list.draw_cnt = 0;
    draw_cmd_ptr = list.cmds;
}

void PVV::DrawCmdList::RecordDraw(uint32_t count, uint32_t firstIndex, uint32_t baseVertex, uint32_t baseInstance, uint32_t instanceCount)
{
#ifdef DEBUG
    //if (list.draw_cnt >= DrawCmdCount - 1)
    //{
    //    std::cout << "Error: Exceeding draw cmd count." << std::endl;
    //    return;
    //}
#endif
    list.draw_cnt++;
    draw_cmd_ptr->count = count;
    draw_cmd_ptr->instanceCount = instanceCount;
    //draw_cmd_ptr->firstIndex = firstIndex;
    draw_cmd_ptr->baseVertex = baseVertex;
    draw_cmd_ptr->baseInstance = baseInstance;
    draw_cmd_ptr++;
}

uint32_t PVV::DrawCmdList::EndFrame()
{
    auto cnt = list.draw_cnt;
    draw_cmd_ptr = nullptr; //Ensures a segfault if any further writes are attempted
    cmd_list.Update(0, sizeof(draw_cmd_list_t), &list);
    return cnt;
}

ProtoVoxel::Graphics::GpuBuffer* PVV::DrawCmdList::GetBuffer()
{
    return &cmd_list;
}