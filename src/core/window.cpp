#include "window.h"
#include "glad/glad.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "voxel/mortoncode.h"
#include <stdio.h>

namespace PVC = ProtoVoxel::Core;
namespace PVG = ProtoVoxel::Graphics;

static void glfw_error_callback(int error, const char *desc)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, desc);
}

static void glfw_windowsize_callback(GLFWwindow *window, int w, int h)
{
    auto winPtr = (PVC::Window *)glfwGetWindowUserPointer(window);
    winPtr->ResizeHandler(w, h);
}

static void gl_debug_callback(GLenum src, GLenum type, GLuint id,
                              GLenum severity, GLsizei length,
                              const GLchar *message, const void *userParam)
{
    printf("OpenGL Debug Message: %s\n", message);
}

static bool glfwInited = false;
static bool varraySetup = false;
static void initGLFW()
{
    if (!glfwInited)
    {
        glfwSetErrorCallback(glfw_error_callback);
        if (!(bool)glfwInit())
        {
            fprintf(stderr, "ERROR: Could not init glfw3.");
            return;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
#ifdef DEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#else
        glfwWindowHint(GLFW_CONTEXT_NO_ERROR, GLFW_TRUE);
#endif

        glfwInited = true;
    }
}

static void termGLFW()
{
    if (glfwInited)
    {
        glfwTerminate();
    }
}

PVC::Window::Window(int w, int h, const char *name)
{
    initGLFW();

    winHndl = glfwCreateWindow(w, h, name, NULL, NULL);
    glfwSetWindowUserPointer(winHndl, this);
}

void PVC::Window::InitGL()
{
    glfwMakeContextCurrent(winHndl);
    glfwSwapInterval(0);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
#ifdef DEBUG
    glDebugMessageCallback(gl_debug_callback, nullptr);
    glEnable(GL_DEBUG_OUTPUT);
#endif
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);

    if (!varraySetup)
    {
        uint32_t varray = 0;
        glGenVertexArrays(1, &varray);
        glBindVertexArray(varray);
        varraySetup = true;
    }

    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    glMaxShaderCompilerThreadsARB(0xffffffff);

    int w,
        h;
    glfwGetFramebufferSize(winHndl, &w, &h);
    this->fbuf = new PVG::Framebuffer(0, w, h);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = &ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(winHndl, true);
    ImGui_ImplOpenGL3_Init("#version 460 core");

    printf("GL Renderer: %s\n", renderer);
    printf("GL Version: %s\n", version);
}

void PVC::Window::StartFrame()
{
    glfwPollEvents();

    bool show_demo_window = true;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

#ifdef DEBUG
    ImGui::ShowMetricsWindow(&show_demo_window);
// ImGui::ShowDemoWindow(&show_demo_window);
#endif
}

void PVC::Window::SwapBuffers()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(winHndl);
}

void PVC::Window::ResizeHandler(int w, int h)
{
    fbuf->w = w;
    fbuf->h = h;
}

bool PVC::Window::ShouldClose()
{
    return (bool)glfwWindowShouldClose(winHndl);
}

PVC::Window::~Window()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(winHndl);
}

PVG::Framebuffer *PVC::Window::GetFramebuffer()
{
    return this->fbuf;
}

double PVC::Window::GetTime()
{
    return glfwGetTime();
}