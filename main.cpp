#include "core/window.h"
#include <iostream>

namespace PVC = ProtoVoxel::Core;

int main(int, char **) {
    PVC::Window win(640, 480, "Test");
    win.InitGL();

    while (!win.ShouldClose()) {
        glClear(GL_COLOR_BUFFER_BIT);
        win.StartFrame();
        win.SwapBuffers();
    }
}
