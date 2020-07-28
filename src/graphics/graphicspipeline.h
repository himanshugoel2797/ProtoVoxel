#pragma once
#include <memory>
#include <stddef.h>
#include <stdint.h>

#include "glad/glad.h"
#include "glm/vec4.hpp"

#include "framebuffer.h"
#include "gpubuffer.h"
#include "shaderprogram.h"

namespace ProtoVoxel::Graphics {
class GraphicsDevice;
class GraphicsPipeline {
private:
  struct BufferBinding {
    bool valid;
    std::weak_ptr<GpuBuffer> buffer;
    size_t offset;
    size_t sz;
  };

  GLenum depthTest;
  std::weak_ptr<Framebuffer> fbuf;
  std::weak_ptr<ShaderProgram> program;
  struct BufferBinding indirectBuffer;
  struct BufferBinding indexBuffer;
  struct BufferBinding *ssbos;
  struct BufferBinding *ubos;

  glm::vec4 clear_color;
  float clear_depth;

  friend class GraphicsDevice;

public:
  GraphicsPipeline();
  ~GraphicsPipeline();

  void SetClearColor(glm::vec4 const &vec);
  void SetDepth(float clearDepth);

  void SetDepthTest(GLenum depthTest);
  void SetFramebuffer(std::weak_ptr<Framebuffer> fbuf);
  void SetShaderProgram(std::weak_ptr<ShaderProgram> program);
  void SetIndirectBuffer(std::weak_ptr<GpuBuffer> indirect, size_t offset,
                         size_t sz);
  void SetIndexBuffer(std::weak_ptr<GpuBuffer> index, size_t offset, size_t sz);

  void SetSSBO(int bindpoint, std::weak_ptr<GpuBuffer> buffer, size_t offset,
               size_t sz);
  void SetUBO(int bindpoint, std::weak_ptr<GpuBuffer> buffer, size_t offset,
              size_t sz);
};
} // namespace ProtoVoxel::Graphics