#pragma once

#include "../rect.hpp"

VK2D_BEGIN

struct Glyph
{
public:
	Glyph() :
		advance(0),
		lsbDelta(0),
		rsbDelta(0),
		bounds(),
		textureRect()
	{}

	float advance;
	int   lsbDelta;
	int   rsbDelta;
	Rect  bounds;
	uRect textureRect;
};

VK2D_END