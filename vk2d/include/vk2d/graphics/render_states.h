#pragma once

#include "transform.h"
#include "render_options.h"

VK2D_BEGIN

class RenderStates {
public:
	RenderStates();

	void reset(const RenderOptions& options);

	void bindTVS(vk::CommandBuffer& cmd_buffer, vk::PipelineLayout layout) const;

	RenderOptions   options;

	Transform    transform;
	vk::Viewport viewport;
	vk::Rect2D   scissor;

	Transform    default_transform;
	vk::Viewport defualt_viewport;
	vk::Rect2D   defualt_scissor;
};

VK2D_END