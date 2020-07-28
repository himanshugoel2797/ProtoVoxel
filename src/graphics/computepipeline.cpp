#include "computepipeline.h"
#include "graphicsdevice.h"

namespace PVG = ProtoVoxel::Graphics;

PVG::ComputePipeline::ComputePipeline() {

  ssbos = new BufferBinding[GraphicsDevice::MAX_BUFFER_BINDPOINTS];
  ubos = new BufferBinding[GraphicsDevice::MAX_BUFFER_BINDPOINTS];
  for (int i = 0; i < GraphicsDevice::MAX_BUFFER_BINDPOINTS; i++) {
    ssbos[i].buffer = std::weak_ptr<GpuBuffer>();
    ubos[i].buffer = std::weak_ptr<GpuBuffer>();
  }
}

PVG::ComputePipeline::~ComputePipeline() {
  delete[] ubos;
  delete[] ssbos;
}

void PVG::ComputePipeline::SetIndirectBuffer(std::weak_ptr<GpuBuffer> indirect,
                                             size_t offset, size_t sz) {
  this->indirectBuffer.buffer = indirect;
  this->indirectBuffer.offset = offset;
  this->indirectBuffer.sz = sz;
  this->indirectBuffer.valid = !indirect.expired();
}

void PVG::ComputePipeline::SetShaderProgram(
    std::weak_ptr<ShaderProgram> program) {
  this->program = program;
}

void PVG::ComputePipeline::SetSSBO(int bindpoint,
                                   std::weak_ptr<GpuBuffer> buffer,
                                   size_t offset, size_t sz) {
  if (bindpoint >= GraphicsDevice::MAX_BUFFER_BINDPOINTS)
    throw std::invalid_argument("bindpoint out of range.");
  ssbos[bindpoint].buffer = buffer;
  ssbos[bindpoint].offset = offset;
  ssbos[bindpoint].sz = sz;
  ssbos[bindpoint].valid = !buffer.expired();
}

void PVG::ComputePipeline::SetUBO(int bindpoint,
                                  std::weak_ptr<GpuBuffer> buffer,
                                  size_t offset, size_t sz) {
  if (bindpoint >= GraphicsDevice::MAX_BUFFER_BINDPOINTS)
    throw std::invalid_argument("bindpoint out of range.");
  ubos[bindpoint].buffer = buffer;
  ubos[bindpoint].offset = offset;
  ubos[bindpoint].sz = sz;
  ubos[bindpoint].valid = !buffer.expired();
}