#pragma once

#include "../core/vector_type.h"
#include "../core/color.h"

VK2D_BEGIN

class Font;

struct TextStyle {
	TextStyle() VK2D_NOTHROW;

	const Font* font;
	uint32_t    size;
	Color       color;
	Color       background_color;
	vec2        align;
	float       letter_spacing;
	float       line_spacing;
	float       outline_thickness;
	Color       outline_color;
	bool        bold;
	bool        under_line;
	bool        strike_through;
	bool        italic;
};

VK2D_END