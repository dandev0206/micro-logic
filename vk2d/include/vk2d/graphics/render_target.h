#pragma once

#include "render_options.h"

VK2D_BEGIN

class Drawable;

class RenderTarget {
public:
	virtual void draw(const Drawable& drawable) = 0;
	virtual void draw(const Drawable& drawable, const RenderOptions& options) = 0;
	virtual void display() = 0;

	Color getClearColor();
	void setClearColor(const Color& color);

	virtual void beginRenderPass() = 0;
	virtual vk::CommandBuffer getCommandBuffer() = 0;
	virtual uint32_t getCurrentFrameIdx() const = 0;
	virtual uint32_t getFrameCount() const = 0;

protected:
	vk::ClearValue clear_value;
};

VK2D_END