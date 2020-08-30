#pragma once

#include <memory>
#include "gpubuffer.h"

namespace ProtoVoxel::Graphics
{
    class ShadowedGpuBuffer
    {
    private:
        GpuBuffer* main_buffer;
        GpuBuffer* shadow_buffer;

    public:
        ShadowedGpuBuffer();
        ~ShadowedGpuBuffer();

        void SetStorage(size_t sz, GLbitfield flags);
        void Update(size_t offset, size_t sz, const void *data);
        void Invalidate(size_t offset, size_t sz);

        void Swap();
        GpuBuffer* GetBuffer();
    };
} // namespace ProtoVoxel::Graphics