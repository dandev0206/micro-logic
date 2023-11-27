#include "../../include/vk2d/system/mouse.h"

#include "../../include/vk2d/system/window.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

VK2D_BEGIN

ivec2 Mouse::getPosition()
{
	POINT point;
	if (!GetCursorPos(&point))
		VK2D_ERROR("can't get cursor pos");

	return { point.x, point.y };
}

ivec2 Mouse::getPosition(const Window& rel_window)
{
	HWND hwnd = (HWND)rel_window.getNativeHandle();
	if (hwnd) {
	    POINT point;
	    GetCursorPos(&point);
	    ScreenToClient(hwnd, &point);
	    return ivec2(point.x, point.y);
	}
	return {};
}

void Mouse::setPosition(const ivec2& pos)
{
	SetCursorPos(pos.x, pos.y);
}

void Mouse::setPosition(const ivec2& pos, const Window& rel_window)
{
	HWND hwnd = (HWND)rel_window.getNativeHandle();
	if (hwnd) {
		POINT point{};
		ClientToScreen(hwnd, &point);
		SetCursorPos(point.x, point.y);
	}
}

bool Mouse::isPressed(Button button)
{
	int virtualKey = 0;
	switch (button)
	{
	case Mouse::Left:     virtualKey = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON; break;
	case Mouse::Right:    virtualKey = GetSystemMetrics(SM_SWAPBUTTON) ? VK_LBUTTON : VK_RBUTTON; break;
	case Mouse::Middle:   virtualKey = VK_MBUTTON;  break;
	case Mouse::XButton1: virtualKey = VK_XBUTTON1; break;
	case Mouse::XButton2: virtualKey = VK_XBUTTON2; break;
	default:              VK2D_ERROR("unknown mouse button"); break;
	}

	return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
}

bool Mouse::isDragging(Button button)
{
	return false;
}

bool Mouse::getDragRect(Button button)
{
	return false;
}

bool Mouse::getDragRect(Button button, const Window& rel_window)
{
	return false;
}

VK2D_END