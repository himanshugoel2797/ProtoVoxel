#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "graphics/framebuffer.h"

namespace ProtoVoxel::Core {
class Window {
private:
  GLFWwindow *winHndl;
  ProtoVoxel::Graphics::Framebuffer *fbuf;

public:
  Window(int w, int h, const char *name);
  ~Window();

  void InitGL();
  void SwapBuffers();
  bool ShouldClose();
  ProtoVoxel::Graphics::Framebuffer *GetFramebuffer();
};
} // namespace ProtoVoxel::Core