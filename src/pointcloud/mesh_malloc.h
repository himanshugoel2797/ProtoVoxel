#pragma once

#include <stdint.h>
#include <memory>
#include <mutex>
#include <vector>
#include "../graphics/gpubuffer.h"

namespace ProtoVoxel::PointCloud
{
    class MeshMalloc
    {
    private:
        ProtoVoxel::Graphics::GpuBuffer mem_blk;
        uint32_t *mem_blk_ptr;
        uint32_t *mem_blk_end;
        uint32_t *mem_blk_cursor;
        uint32_t loopback_cnt = 0;

        struct FlushData {
            uint32_t offset;
            uint32_t len;
        };

        std::vector<struct FlushData> pending_updates;
        std::mutex alloc_lock;
        //We expect to loop through this buffer such that older entries are naturally out of range by the time they're overwritten

    public:
        static const uint64_t MallocPoolSize = 2ull * 512 * 1024 * 1024; //1GiB
        
        MeshMalloc();
        ~MeshMalloc();

        void Initialize();
        uint32_t *Alloc(size_t sz, uint32_t &loopback_cnt_ret);
        uint32_t GetCurrentLoopbackCount();
        void Flush(uint32_t* ptr, uint32_t len);

        void Update();

        uint32_t GetOffset(uint32_t* ptr);

        ProtoVoxel::Graphics::GpuBuffer* GetBuffer();
    };
} // namespace ProtoVoxel::Voxel