#include "window.h"
#include "glad/glad.h"
#include <stdio.h>

namespace PVC = ProtoVoxel::Core;
namespace PVG = ProtoVoxel::Graphics;

static void glfw_error_callback(int error, const char *desc) {
  fprintf(stderr, "Error: %s\n", desc);
}

static bool glfwInited = false;
static void initGLFW() {
  if (!glfwInited) {
    if (!glfwInit()) {
      fprintf(stderr, "ERROR: Could not init glfw3.");
      return;
    }
    glfwSetErrorCallback(glfw_error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwInited = true;
  }
}

static void termGLFW() {
  if (glfwInited) {
    glfwTerminate();
  }
}

PVC::Window::Window(int w, int h, const char *name) {
  initGLFW();

  winHndl = glfwCreateWindow(w, h, name, NULL, NULL);
}

void PVC::Window::InitGL() {
  glfwMakeContextCurrent(winHndl);

  gladLoadGL();
  const GLubyte *renderer = glGetString(GL_RENDERER);
  const GLubyte *version = glGetString(GL_VERSION);

  int w, h;
  glfwGetFramebufferSize(winHndl, &w, &h);
  this->fbuf = new PVG::Framebuffer(0, w, h);

  printf("GL Renderer: %s\n", renderer);
  printf("GL Version: %s\n", version);
}

void PVC::Window::SwapBuffers() {
  glfwSwapBuffers(winHndl);
  glfwPollEvents();
}

bool PVC::Window::ShouldClose() { return glfwWindowShouldClose(winHndl); }

PVC::Window::~Window() { glfwDestroyWindow(winHndl); }

PVG::Framebuffer *PVC::Window::GetFramebuffer() { return this->fbuf; }