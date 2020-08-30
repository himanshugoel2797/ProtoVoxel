#include "mesh_malloc.h"

namespace PVV = ProtoVoxel::Voxel;

PVV::MeshMalloc::MeshMalloc()
{
}

PVV::MeshMalloc::~MeshMalloc()
{
    mem_blk.Unmap();
}

void PVV::MeshMalloc::Initialize()
{
    mem_blk.SetStorage(MallocPoolSize, GL_DYNAMIC_STORAGE_BIT);
    mem_blk_ptr = new uint32_t[MallocPoolSize / sizeof(uint32_t)]; //static_cast<uint32_t *>(mem_blk->PersistentMap(0, MallocPoolSize, GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT));
    mem_blk_cursor = mem_blk_ptr;
    mem_blk_end = mem_blk_ptr + MallocPoolSize / sizeof(uint32_t);
}

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

    return retVal;
}

void PVV::MeshMalloc::Flush(uint32_t offset, uint32_t len, void *base_ptr)
{
    mem_blk.Update(offset * sizeof(uint32_t), len * sizeof(uint32_t), base_ptr);
}

void PVV::MeshMalloc::FreeRear(size_t sz)
{
    mem_blk_cursor -= sz;
}

uint32_t PVV::MeshMalloc::GetCurrentLoopbackCount()
{
    return loopback_cnt;
}

ProtoVoxel::Graphics::GpuBuffer* PVV::MeshMalloc::GetBuffer()
{
    return &mem_blk;
}