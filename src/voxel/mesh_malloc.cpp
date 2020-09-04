#include "mesh_malloc.h"

namespace PVV = ProtoVoxel::Voxel;

PVV::MeshMalloc::MeshMalloc() : alloc_lock()
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

uint32_t* PVV::MeshMalloc::Alloc(size_t sz, uint32_t* loopback_cnt_ret)
{
	alloc_lock.lock();
	if (mem_blk_cursor + sz >= mem_blk_end)
	{
		mem_blk_cursor = mem_blk_ptr;
		loopback_cnt++;
	}
	auto retVal = mem_blk_cursor;
	mem_blk_cursor += sz;
	*loopback_cnt_ret = loopback_cnt;
	alloc_lock.unlock();

	return retVal;
}

void PVV::MeshMalloc::Flush(uint32_t* offset, uint32_t len)
{
	FlushData flush_dat;
	flush_dat.offset = offset - mem_blk_ptr;
	flush_dat.len = len;

	alloc_lock.lock();
	pending_updates.push_back(flush_dat);
	alloc_lock.unlock();
}

void PVV::MeshMalloc::Update() {
	alloc_lock.lock();
	for (int i = 0; i < pending_updates.size(); i++) {
		auto o = pending_updates[i];
		if (o.len > 0)
			mem_blk.Update(o.offset * sizeof(uint32_t), o.len * sizeof(uint32_t), mem_blk_ptr + o.offset);
	}
	pending_updates.clear();
	alloc_lock.unlock();
}

uint32_t PVV::MeshMalloc::GetOffset(uint32_t* ptr) {
	return ptr - mem_blk_ptr;
}

uint32_t PVV::MeshMalloc::GetCurrentLoopbackCount()
{
	return loopback_cnt;
}

ProtoVoxel::Graphics::GpuBuffer* PVV::MeshMalloc::GetBuffer()
{
	return &mem_blk;
}