#include "input.h"

namespace PVC = ProtoVoxel::Core;

PVC::Window* PVC::Input::win;
std::vector<int> PVC::Input::bindings;
int PVC::Input::escape_binding;
bool PVC::Input::esc_down;
bool PVC::Input::cursor_locked;

void PVC::Input::RegisterWindow(Window* w)
{
	win = w;
	glfwSetInputMode(win->winHndl, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	escape_binding = RegisterBinding(GLFW_KEY_ESCAPE);
	esc_down = false;
	cursor_locked = true;
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

void PVC::Input::Update(double time) {
	if (esc_down && !IsKeyDown(escape_binding)) {
		if (cursor_locked)
			glfwSetInputMode(win->winHndl, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		else
			glfwSetInputMode(win->winHndl, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		cursor_locked = !cursor_locked;
		esc_down = false;
	}
	if (IsKeyDown(escape_binding))
		esc_down = true;
}

bool PVC::Input::IsCursorLocked() {
	return cursor_locked;
}

void PVC::Input::GetMousePosition(glm::vec2& pos)
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