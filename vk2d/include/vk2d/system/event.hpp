#pragma once

#include "keyboard.h"
#include "mouse.h"

VK2D_BEGIN

struct Event {
	enum Type {
		Resize,
		Move,
		Close,
		LostFocus,
		Focussed,
		KeyPressed,
		KeyReleased,
		WheelScrolled,
		MousePressed,
		MouseReleased,
		MouseMoved,
		MouseLeft,
		MouseEntered,
		TextEntered
	};

	Type type;
	
	union {
		struct {
			uvec2 size;
		} resize;

		struct {
			ivec2 pos;
		} move;

		struct {
			Key key;
			bool ctrl;
			bool alt;
			bool shft;
			bool sys;
		} keyboard;

		struct {
			Mouse::Button button;
			ivec2         pos;
		} mouseButton;

		struct {
			vec2 delta;
			ivec2 pos;
		} wheel;

		struct {
			ivec2 pos;
			ivec2 delta;
		} mouseMove;

		struct {
			int unicode;
		} text;
	};
};

VK2D_END