#pragma once

#include "render_target.h"
#include "render_states.h"

VK2D_BEGIN

class Drawable {
public:
	virtual ~Drawable() {};

	virtual void draw(RenderTarget& target, RenderStates& states) const = 0;
};

VK2D_END