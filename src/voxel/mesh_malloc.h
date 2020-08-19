#pragma once

#include <stdint.h>
#include <memory>
#include "../graphics/gpubuffer.h"

namespace ProtoVoxel::Voxel
{
    class MeshMalloc
    {
    private:
        std::shared_ptr<ProtoVoxel::Graphics::GpuBuffer> mem_blk;
        uint32_t *mem_blk_ptr;
        uint32_t *mem_blk_end;
        uint32_t *mem_blk_cursor;
        uint32_t loopback_cnt = 0;
        const int MallocPoolSize = 3 * 512 * 1024 * 1024; //1.5GiB

        //We expect to loop through this buffer such that older entries are naturally out of range by the time they're overwritten

    public:
        MeshMalloc();
        ~MeshMalloc();

        void Initialize();
        uint32_t *Alloc(size_t sz, uint32_t *loopback_cnt_ret);
        uint32_t GetCurrentLoopbackCount();

        std::weak_ptr<ProtoVoxel::Graphics::GpuBuffer> GetBuffer();
    };
} // namespace ProtoVoxel::Voxel