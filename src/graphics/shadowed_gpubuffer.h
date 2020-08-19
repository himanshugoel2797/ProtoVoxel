#pragma once

#include <memory>
#include "gpubuffer.h"

namespace ProtoVoxel::Graphics
{
    class ShadowedGpuBuffer
    {
    private:
        std::shared_ptr<GpuBuffer> main_buffer;
        std::shared_ptr<GpuBuffer> shadow_buffer;

    public:
        ShadowedGpuBuffer();
        ~ShadowedGpuBuffer();

        void SetStorage(size_t sz, GLbitfield flags);
        void Update(size_t offset, size_t sz, const void *data);
        void Invalidate(size_t offset, size_t sz);

        void Swap();
        std::weak_ptr<GpuBuffer> GetBuffer();
    };
} // namespace ProtoVoxel::Graphics