#include "shadowed_gpubuffer.h"

namespace PVG = ProtoVoxel::Graphics;

PVG::ShadowedGpuBuffer::ShadowedGpuBuffer()
{
    main_buffer = new PVG::GpuBuffer();
    shadow_buffer = new PVG::GpuBuffer();
}

PVG::ShadowedGpuBuffer::~ShadowedGpuBuffer() {}

void PVG::ShadowedGpuBuffer::SetStorage(size_t sz, GLbitfield flags)
{
    main_buffer->SetStorage(sz, flags);
    shadow_buffer->SetStorage(sz, flags);
}

void PVG::ShadowedGpuBuffer::Update(size_t offset, size_t sz, const void *data)
{
    main_buffer->Update(offset, sz, data);
}

void PVG::ShadowedGpuBuffer::Invalidate(size_t offset, size_t sz)
{
    main_buffer->Invalidate(offset, sz);
}

void PVG::ShadowedGpuBuffer::Swap()
{
    auto tmp = shadow_buffer;
    shadow_buffer = main_buffer;
    main_buffer = tmp;
}

PVG::GpuBuffer* PVG::ShadowedGpuBuffer::GetBuffer()
{
    return main_buffer;
}