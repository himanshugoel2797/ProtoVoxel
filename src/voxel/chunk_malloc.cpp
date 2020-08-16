#include "chunk_malloc.h"
#include "chunk.h"

#include <string.h>
#include <windows.h>

//Use VirtualAlloc to reserve address space for chunk data
//Hand out full regions to chunks
//Chunks have to request regions to become backed/freed

namespace PVV = ProtoVoxel::Voxel;

const uint32_t BlockCount = (1 << 18);
const uint64_t FullSize = BlockCount * (PVV::Chunk::ChunkLen + PVV::Chunk::ChunkSide * PVV::Chunk::ChunkSide * sizeof(uint32_t));

PVV::ChunkMalloc::ChunkMalloc()
{
}

PVV::ChunkMalloc::~ChunkMalloc()
{
}

void PVV::ChunkMalloc::Initialize()
{
    data_base = static_cast<uint8_t *>(VirtualAlloc(nullptr, FullSize, MEM_RESERVE, PAGE_NOACCESS));
    if (data_base != nullptr)
    {
        for (uint32_t i = 0; i < BlockCount; i++)
            block_presence_bmp.push(i);
    }
}

uint8_t *PVV::ChunkMalloc::AllocChunkBlock()
{
    if (block_presence_bmp.size() == 0)
    {
        return nullptr;
    }

    uint32_t chnk = block_presence_bmp.top();
    block_presence_bmp.pop();
    return data_base + chnk * Chunk::ChunkLen;
}

void PVV::ChunkMalloc::FreeChunkBlock(uint8_t *block)
{
    VirtualFree(block, Chunk::ChunkLen, MEM_DECOMMIT);
    block_presence_bmp.push((uint32_t)((block - data_base) / Chunk::ChunkLen));
}

uint8_t *PVV::ChunkMalloc::CommitChunkRegion(uint8_t *block, int region_idx)
{
    auto r_ptr = static_cast<uint8_t *>(VirtualAlloc(block + region_idx * Chunk::RegionSize, Chunk::RegionSize, MEM_COMMIT, PAGE_READWRITE));
    return r_ptr;
}

void PVV::ChunkMalloc::DecommitChunkBlock(uint8_t *block, int region_idx)
{
    VirtualFree(block + region_idx * Chunk::RegionSize, Chunk::RegionSize, MEM_DECOMMIT);
}