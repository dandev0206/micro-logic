#pragma once

#include "color.hpp"

VK2D_BEGIN

struct Vertex {
	VK2D_INLINE Vertex() VK2D_NOTHROW :
		pos(),
		color(Colors::White),
		uv(1.f) {}

	VK2D_INLINE Vertex(const vec2& pos) VK2D_NOTHROW :
		pos(pos),
		color(Colors::White),
		uv(1.f) {}

	VK2D_INLINE Vertex(const vec2& pos, const Color& color) VK2D_NOTHROW :
		pos(pos),
		color(color),
		uv(1.f) {}

	VK2D_INLINE Vertex(const vec2& pos, const vec2& uv) VK2D_NOTHROW :
		pos(pos),
		color(Colors::White),
		uv(uv) {}

	VK2D_INLINE Vertex(const vec2& pos, const Color& color, const vec2& uv) VK2D_NOTHROW :
		pos(pos),
		color(color),
		uv(uv) {}

	vec2 pos;
	Color     color;
	vec2 uv;
};

VK2D_END