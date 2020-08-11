#pragma once
#include <stddef.h>
#include <stdint.h>
#include "glad/glad.h"

#include "texture.h"

namespace ProtoVoxel::Core {
class Window;
}

namespace ProtoVoxel::Graphics {
class Framebuffer {
  friend class ProtoVoxel::Core::Window;

 private:
  uint32_t id;
  int w, h;
  Framebuffer(uint32_t id, int w, int h);

 public:
  Framebuffer(int w, int h);
  ~Framebuffer();

  void Attach(GLenum attachment, Texture const& tex, int level);
  void Attach(GLenum attachment, Texture const& tex, int level, int layer);
  void DrawBuffers(int n, const GLenum* attachments);

  uint32_t GetID() const { return id; }
};
}  // namespace ProtoVoxel::Graphics