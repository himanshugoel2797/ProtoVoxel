#include "gpubuffer.h"

namespace PVG = ProtoVoxel::Graphics;

PVG::GpuBuffer::GpuBuffer()
{
    glCreateBuffers(1, &id);
}

PVG::GpuBuffer::~GpuBuffer()
{
    glDeleteBuffers(1, &id);
    cudaGraphicsUnregisterResource(cuda_res);
}

void PVG::GpuBuffer::SetStorage(size_t sz, GLbitfield flags)
{
    glNamedBufferStorage(id, sz, nullptr, flags);
    auto err = cudaGraphicsGLRegisterBuffer(&cuda_res, id, cudaGraphicsRegisterFlagsNone);
}

void PVG::GpuBuffer::Update(size_t offset, size_t sz, const void *data)
{
    glNamedBufferSubData(id, offset, sz, data);
}

void PVG::GpuBuffer::Invalidate(size_t offset, size_t sz)
{
    glInvalidateBufferSubData(id, offset, sz);
}

void *PVG::GpuBuffer::PersistentMap(size_t offset, size_t sz, GLbitfield flags)
{
    return glMapNamedBufferRange(id, offset, sz, flags);
}

void PVG::GpuBuffer::Flush(uint32_t offset, uint32_t sz)
{
    glFlushMappedNamedBufferRange(id, offset, sz);
}

void PVG::GpuBuffer::Clear(uint32_t offset, uint32_t sz)
{
    glClearNamedBufferSubData(id, GL_R8UI, offset, sz, GL_RED, GL_UNSIGNED_BYTE, nullptr);
}

void PVG::GpuBuffer::Unmap()
{
    glUnmapNamedBuffer(id);
}

void* PVG::GpuBuffer::GetCudaDevicePointer()
{
    void* ptr;
    size_t sz;
    auto err = cudaGraphicsMapResources(1, &cuda_res);
    cudaGraphicsResourceGetMappedPointer(&ptr, &sz, cuda_res);
    return ptr;
}

void PVG::GpuBuffer::UnmapCudaDevicePointer()
{
    cudaGraphicsUnmapResources(1, &cuda_res);
}