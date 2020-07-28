#pragma once

#include <memory>

#include "gpubuffer.h"
#include "shaderprogram.h"

namespace ProtoVoxel::Graphics {
class GraphicsDevice;
class ComputePipeline {
private:
  struct BufferBinding {
    bool valid;
    std::weak_ptr<GpuBuffer> buffer;
    size_t offset;
    size_t sz;
  };

  std::weak_ptr<ShaderProgram> program;
  struct BufferBinding indirectBuffer;
  struct BufferBinding *ssbos;
  struct BufferBinding *ubos;

  friend class GraphicsDevice;

public:
  ComputePipeline();
  ~ComputePipeline();

  void SetShaderProgram(std::weak_ptr<ShaderProgram> program);
  void SetIndirectBuffer(std::weak_ptr<GpuBuffer> indirect, size_t offset,
                         size_t sz);

  void SetSSBO(int bindpoint, std::weak_ptr<GpuBuffer> buffer, size_t offset,
               size_t sz);
  void SetUBO(int bindpoint, std::weak_ptr<GpuBuffer> buffer, size_t offset,
              size_t sz);
};
} // namespace ProtoVoxel::Graphics