#include "core/window.h"
#include "graphics/graphicsdevice.h"
#include <iostream>
#include <random>
#include <chrono>

namespace PVC = ProtoVoxel::Core;
namespace PVG = ProtoVoxel::Graphics;

int main(int, char **)
{
    PVC::Window win(640, 480, "Test");
    win.InitGL();

    while (!win.ShouldClose())
    {
        PVG::GraphicsDevice::ClearAll();
        win.StartFrame();
        win.SwapBuffers();
    }
}
