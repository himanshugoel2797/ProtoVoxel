#pragma once
#include <stddef.h>
#include <stdint.h>
#include <memory>
#include "glad/glad.h"

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cuda_gl_interop.h>

namespace ProtoVoxel::Graphics
{
    class GpuBuffer
    {
    private:
        uint32_t id;
        cudaGraphicsResource_t cuda_res;

    public:
        GpuBuffer();
        ~GpuBuffer();

        void SetStorage(size_t sz, GLbitfield flags);
        void Update(size_t offset, size_t sz, const void *data);
        void Invalidate(size_t offset, size_t sz);
        void *PersistentMap(size_t offset, size_t sz, GLbitfield flags);
        void Unmap();
        void Flush(uint32_t offset, uint32_t len);
        void Clear(uint32_t offset, uint32_t len);

        void *GetCudaDevicePointer();
        void UnmapCudaDevicePointer();

        inline void SetName(const char* name);

        uint32_t GetID() const
        {
            return id;
        }
    };
} // namespace ProtoVoxel::Graphics

void ProtoVoxel::Graphics::GpuBuffer::SetName(const char* name) {
    glObjectLabel(GL_BUFFER, id, strnlen_s(name, 16384), name);
}