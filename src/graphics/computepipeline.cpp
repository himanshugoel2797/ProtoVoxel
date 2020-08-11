#include "computepipeline.h"
#include "graphicsdevice.h"

#include <stdexcept>

namespace PVG = ProtoVoxel::Graphics;

PVG::ComputePipeline::ComputePipeline() {
  ssbos = new BufferBinding[GraphicsDevice::MAX_BINDPOINTS];
  ubos = new BufferBinding[GraphicsDevice::MAX_BINDPOINTS];
  textures = new TextureBinding[GraphicsDevice::MAX_BINDPOINTS];
  for (int i = 0; i < GraphicsDevice::MAX_BINDPOINTS; i++) {
    ssbos[i].valid = false;
    ubos[i].valid = false;
    textures[i].valid = false;
  }
}

PVG::ComputePipeline::~ComputePipeline() {
  delete[] textures;
  delete[] ubos;
  delete[] ssbos;
}

void PVG::ComputePipeline::SetIndirectBuffer(std::weak_ptr<GpuBuffer> indirect,
                                             size_t offset,
                                             size_t sz) {
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
                                   size_t offset,
                                   size_t sz) {
  if (bindpoint >= GraphicsDevice::MAX_BINDPOINTS)
    throw std::invalid_argument("bindpoint out of range.");
  ssbos[bindpoint].buffer = buffer;
  ssbos[bindpoint].offset = offset;
  ssbos[bindpoint].sz = sz;
  ssbos[bindpoint].valid = !buffer.expired();
}

void PVG::ComputePipeline::SetUBO(int bindpoint,
                                  std::weak_ptr<GpuBuffer> buffer,
                                  size_t offset,
                                  size_t sz) {
  if (bindpoint >= GraphicsDevice::MAX_BINDPOINTS)
    throw std::invalid_argument("bindpoint out of range.");
  ubos[bindpoint].buffer = buffer;
  ubos[bindpoint].offset = offset;
  ubos[bindpoint].sz = sz;
  ubos[bindpoint].valid = !buffer.expired();
}

void PVG::ComputePipeline::SetTexture(int bindpoint,
                                      std::weak_ptr<Texture> texture) {
  if (bindpoint >= GraphicsDevice::MAX_BINDPOINTS)
    throw std::invalid_argument("bindpoint out of range.");
  textures[bindpoint].texture = texture;
  textures[bindpoint].valid = !texture.expired();
}