#include "../include/vk2d/graphics/render_states.h"

VK2D_BEGIN

RenderStates::RenderStates() 
{
}

void RenderStates::reset(const RenderOptions& options)
{
	this->options = options;

	if (options.transform.has_value())
		transform = default_transform * options.transform.value();
	else
		transform = default_transform;

	if (options.viewport.has_value()) {
		auto rect = (Rect)options.viewport.value();
		auto size = (vec2)rect.getSize();

		viewport = vk::Viewport(rect.left, rect.top, size.x, size.y);
	} else 
		viewport = defualt_viewport;

	if (options.scissor.has_value()) {
		auto rect = options.scissor.value();
		auto size = rect.getSize();

		scissor = vk::Rect2D(vk::Offset2D(rect.left, rect.top), vk::Extent2D(size.x, size.y));
	} else 
		scissor = defualt_scissor;
}

void RenderStates::bindTVS(vk::CommandBuffer& cmd_buffer, vk::PipelineLayout layout) const
{
	// push transform
	cmd_buffer.pushConstants(
		layout,
		vk::ShaderStageFlagBits::eVertex,
		0, sizeof(Transform),
		(void*)&transform
	);

	// set viewport / scissor
	cmd_buffer.setViewport(0, 1, &viewport);
	cmd_buffer.setScissor(0, 1, &scissor);
}

VK2D_END
