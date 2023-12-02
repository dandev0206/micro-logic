#pragma once

#include "../vk2d_config.h"

VK2D_BEGIN

class RenderTarget;
class RenderStates;

class Drawable {
public:
	virtual ~Drawable() {};

	virtual void draw(RenderTarget& target, RenderStates& states) const = 0;
};

VK2D_END