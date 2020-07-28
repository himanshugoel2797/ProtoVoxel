#include "gpubuffer.h"

namespace PVG = ProtoVoxel::Graphics;

PVG::GpuBuffer::GpuBuffer() { glGenBuffers(1, &id); }

PVG::GpuBuffer::~GpuBuffer() { glDeleteBuffers(1, &id); }

void PVG::GpuBuffer::SetStorage(size_t sz, GLbitfield flags) {
  glNamedBufferStorage(id, sz, nullptr, flags);
}

void PVG::GpuBuffer::Update(size_t offset, size_t sz, const void *data) {
  glNamedBufferSubData(id, offset, sz, data);
}

void PVG::GpuBuffer::Invalidate(size_t offset, size_t sz) {
  glInvalidateBufferSubData(id, offset, sz);
}