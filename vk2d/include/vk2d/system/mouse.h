#pragma once

#include "../vector_type.h"

VK2D_BEGIN

class Window;

class Mouse {
public:
	enum Button {
		Left,
		Right,
		Middle,
		XButton1,
		XButton2
	};

	static ivec2 getPosition();
	static ivec2 getPosition(const Window& rel_window);
	static void setPosition(const ivec2& pos);
	static void setPosition(const ivec2& pos, const Window& rel_window);
	static bool isPressed(Button button);
	static bool isDragging(Button button);
	static bool getDragRect(Button button);
	static bool getDragRect(Button button, const Window& rel_window);
};

VK2D_END