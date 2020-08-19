#pragma once
#include <vector>
#include "glm/glm.hpp"

#include "window.h"

namespace ProtoVoxel::Core
{
    enum class MouseButton
    {
        Left = 1 << 0,
        Right = 1 << 1,
    };

    class Input
    {
    private:
        static ProtoVoxel::Core::Window *win;
        static std::vector<int> bindings;

    public:
        static void RegisterWindow(Window *w);
        static int RegisterBinding(int key);
        static bool IsKeyDown(int binding);
        static void GetMousePosition(glm::vec2 &pos);
        static bool IsMouseDown(MouseButton btn);
    };
} // namespace ProtoVoxel::Core