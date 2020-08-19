#include "input.h"

namespace PVC = ProtoVoxel::Core;

PVC::Window *PVC::Input::win;
std::vector<int> PVC::Input::bindings;

void PVC::Input::RegisterWindow(Window *w)
{
    win = w;
    //glfwSetInputMode(win->winHndl, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

int PVC::Input::RegisterBinding(int key)
{
    bindings.push_back(key);
    return bindings.size() - 1;
}

bool PVC::Input::IsKeyDown(int binding)
{
    if (binding < bindings.size())
    {
        auto k = glfwGetKey(win->winHndl, bindings[binding]);
        if (k == GLFW_PRESS)
            return true;
    }
    return false;
}

void PVC::Input::GetMousePosition(glm::vec2 &pos)
{
    double x, y;
    glfwGetCursorPos(win->winHndl, &x, &y);
    pos.x = x;
    pos.y = y;
}

bool PVC::Input::IsMouseDown(PVC::MouseButton btn)
{
    int key = 0;
    switch (btn)
    {
    case MouseButton::Left:
        key = GLFW_MOUSE_BUTTON_LEFT;
        break;
    case MouseButton::Right:
        key = GLFW_MOUSE_BUTTON_RIGHT;
        break;
    }

    int state = glfwGetMouseButton(win->winHndl, key);
    if (state == GLFW_PRESS)
        return true;
    return false;
}