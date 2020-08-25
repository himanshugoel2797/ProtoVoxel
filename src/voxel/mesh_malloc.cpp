#include "mesh_malloc.h"

namespace PVV = ProtoVoxel::Voxel;

PVV::MeshMalloc::MeshMalloc()
{
    mem_blk = std::make_shared<ProtoVoxel::Graphics::GpuBuffer>();
}

PVV::MeshMalloc::~MeshMalloc()
{
    mem_blk->Unmap();
}

void PVV::MeshMalloc::Initialize()
{
    mem_blk->SetStorage(MallocPoolSize, GL_DYNAMIC_STORAGE_BIT);
    mem_blk_ptr = new uint32_t[MallocPoolSize / sizeof(uint32_t)]; //static_cast<uint32_t *>(mem_blk->PersistentMap(0, MallocPoolSize, GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT));
    mem_blk_cursor = mem_blk_ptr;
    mem_blk_end = mem_blk_ptr + MallocPoolSize / sizeof(uint32_t);
}

uint32_t *lastAlloc;

uint32_t *PVV::MeshMalloc::Alloc(size_t sz, uint32_t *loopback_cnt_ret)
{
    if (mem_blk_cursor + sz >= mem_blk_end)
    {
        mem_blk_cursor = mem_blk_ptr;
        loopback_cnt++;
    }
    auto retVal = mem_blk_cursor;
    mem_blk_cursor += sz;
    *loopback_cnt_ret = loopback_cnt;

    lastAlloc = retVal;
    return retVal;
}

void PVV::MeshMalloc::Flush(uint32_t offset, uint32_t len)
{
    mem_blk->Update(offset, len, lastAlloc);
    //mem_blk->Flush(offset, len);
}

uint32_t PVV::MeshMalloc::GetCurrentLoopbackCount()
{
    return loopback_cnt;
}

std::weak_ptr<ProtoVoxel::Graphics::GpuBuffer> PVV::MeshMalloc::GetBuffer()
{
    return mem_blk;
}