#pragma once

#include "../core/vk2d_def.h"

VK2D_BEGIN

class RenderTarget;
class RenderStates;

class Drawable {
public:
	virtual ~Drawable() {};

	virtual void draw(RenderTarget& target, RenderStates& states) const = 0;
};

VK2D_END