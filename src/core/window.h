#pragma once
#define GLFW_INCLUDE_NONE
#include "imgui/imgui.h"
#include <GLFW/glfw3.h>

#include "graphics/framebuffer.h"

namespace ProtoVoxel::Core {
class Window {
private:
  GLFWwindow *winHndl;
  ProtoVoxel::Graphics::Framebuffer *fbuf;
  ImGuiIO *io;

public:
  Window(int w, int h, const char *name);
  ~Window();

  void InitGL();
  void StartFrame();
  void SwapBuffers();
  bool ShouldClose();
  void ResizeHandler(int w, int h);
  ProtoVoxel::Graphics::Framebuffer *GetFramebuffer();
};
} // namespace ProtoVoxel::Core