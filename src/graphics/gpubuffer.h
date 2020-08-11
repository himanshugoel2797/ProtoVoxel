#pragma once
#include <stddef.h>
#include <stdint.h>
#include "glad/glad.h"

namespace ProtoVoxel::Graphics {
class GpuBuffer {
 private:
  uint32_t id;

 public:
  GpuBuffer();
  ~GpuBuffer();

  void SetStorage(size_t sz, GLbitfield flags);
  void Update(size_t offset, size_t sz, const void* data);
  void Invalidate(size_t offset, size_t sz);
  uint32_t GetID() const { return id; }
};
}  // namespace ProtoVoxel::Graphics