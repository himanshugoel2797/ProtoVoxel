#include "graphicsdevice.h"
#include "glad/glad.h"

namespace PVG = ProtoVoxel::Graphics;

static std::shared_ptr<PVG::Framebuffer> boundFramebuffer;
static std::shared_ptr<PVG::ShaderProgram> boundProgram;

static std::shared_ptr<PVG::GpuBuffer> boundIndirectBuffer;
static std::shared_ptr<PVG::GpuBuffer> boundIndexBuffer;
static std::shared_ptr<PVG::GpuBuffer>
    boundSSBOs[PVG::GraphicsDevice::MAX_BINDPOINTS];
static std::shared_ptr<PVG::GpuBuffer>
    boundUBOs[PVG::GraphicsDevice::MAX_BINDPOINTS];

void PVG::GraphicsDevice::ClearColor() { glClear(GL_COLOR_BUFFER_BIT); }

void PVG::GraphicsDevice::ClearDepth() { glClear(GL_DEPTH_BUFFER_BIT); }

void PVG::GraphicsDevice::ClearAll() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PVG::GraphicsDevice::BindGraphicsPipeline(
    PVG::GraphicsPipeline const &pipeline) {

  if (pipeline.fbuf.expired())
    throw std::invalid_argument("Framebuffer has expired.");
  if (pipeline.program.expired())
    throw std::invalid_argument("Shader Program has expired.");

  boundFramebuffer = pipeline.fbuf.lock();
  auto fbuf_ptr = boundFramebuffer.get();

  boundProgram = pipeline.program.lock();
  auto prog_ptr = boundProgram.get();

  if (pipeline.indirectBuffer.valid) {
    if (!pipeline.indirectBuffer.buffer.expired()) {
      boundIndirectBuffer = pipeline.indirectBuffer.buffer.lock();
      auto indir_ptr = boundIndirectBuffer.get();
      glBindBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, indir_ptr->GetID(),
                        pipeline.indirectBuffer.offset,
                        pipeline.indirectBuffer.sz);
    } else
      throw std::invalid_argument("Indirect Buffer has expired.");
  } else {
    boundIndirectBuffer = std::shared_ptr<GpuBuffer>(nullptr);
  }

  if (pipeline.indexBuffer.valid) {
    if (!pipeline.indexBuffer.buffer.expired()) {
      boundIndexBuffer = pipeline.indexBuffer.buffer.lock();
      auto index_ptr = boundIndexBuffer.get();
      glBindBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, index_ptr->GetID(),
                        pipeline.indexBuffer.offset, pipeline.indexBuffer.sz);
    } else
      throw std::invalid_argument("Index Buffer has expired.");
  } else {
    boundIndexBuffer = std::shared_ptr<GpuBuffer>(nullptr);
  }

  for (int i = 0; i < MAX_BINDPOINTS; i++) {
    if (pipeline.ssbos[i].valid) {
      if (!pipeline.ssbos[i].buffer.expired()) {
        boundSSBOs[i] = pipeline.ssbos[i].buffer.lock();
        auto ssbo_ptr = boundSSBOs[i].get();
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, i, ssbo_ptr->GetID(),
                          pipeline.ssbos[i].offset, pipeline.ssbos[i].sz);
      } else
        throw std::invalid_argument("SSBO has expired.");
    } else
      boundSSBOs[i] = std::shared_ptr<GpuBuffer>(nullptr);

    if (pipeline.ubos[i].valid) {
      if (!pipeline.ubos[i].buffer.expired()) {
        boundUBOs[i] = pipeline.ubos[i].buffer.lock();
        auto ubo_ptr = boundUBOs[i].get();
        glBindBufferRange(GL_UNIFORM_BUFFER, i, ubo_ptr->GetID(),
                          pipeline.ubos[i].offset, pipeline.ubos[i].sz);
      } else
        throw std::invalid_argument("UBO has expired.");
    } else
      boundUBOs[i] = std::shared_ptr<GpuBuffer>(nullptr);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, fbuf_ptr->GetID());
  glUseProgram(prog_ptr->GetID());

  glClearColor(pipeline.clear_color.r, pipeline.clear_color.g,
               pipeline.clear_color.b, pipeline.clear_color.a);

  glClearDepthf(pipeline.clear_depth);
}

void PVG::GraphicsDevice::BindComputePipeline(
    PVG::ComputePipeline const &pipeline) {

  if (pipeline.program.expired())
    throw std::invalid_argument("Shader Program has expired.");

  boundProgram = pipeline.program.lock();
  auto prog_ptr = boundProgram.get();

  if (pipeline.indirectBuffer.valid) {
    if (!pipeline.indirectBuffer.buffer.expired()) {
      boundIndirectBuffer = pipeline.indirectBuffer.buffer.lock();
      auto indir_ptr = boundIndirectBuffer.get();
      glBindBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, indir_ptr->GetID(),
                        pipeline.indirectBuffer.offset,
                        pipeline.indirectBuffer.sz);
    } else
      throw std::invalid_argument("Indirect Buffer has expired.");
  } else {
    boundIndirectBuffer = std::shared_ptr<GpuBuffer>(nullptr);
  }

  for (int i = 0; i < MAX_BINDPOINTS; i++) {
    if (pipeline.ssbos[i].valid) {
      if (!pipeline.ssbos[i].buffer.expired()) {
        boundSSBOs[i] = pipeline.ssbos[i].buffer.lock();
        auto ssbo_ptr = boundSSBOs[i].get();
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, i, ssbo_ptr->GetID(),
                          pipeline.ssbos[i].offset, pipeline.ssbos[i].sz);
      } else
        throw std::invalid_argument("SSBO has expired.");
    } else
      boundSSBOs[i] = std::shared_ptr<GpuBuffer>(nullptr);

    if (pipeline.ubos[i].valid) {
      if (!pipeline.ubos[i].buffer.expired()) {
        boundUBOs[i] = pipeline.ubos[i].buffer.lock();
        auto ubo_ptr = boundUBOs[i].get();
        glBindBufferRange(GL_UNIFORM_BUFFER, i, ubo_ptr->GetID(),
                          pipeline.ubos[i].offset, pipeline.ubos[i].sz);
      } else
        throw std::invalid_argument("UBO has expired.");
    } else
      boundUBOs[i] = std::shared_ptr<GpuBuffer>(nullptr);
  }

  glUseProgram(prog_ptr->GetID());
}