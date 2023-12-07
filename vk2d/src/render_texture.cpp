#include "../include/vk2d/graphics/render_texture.h"

#include "../include/vk2d/graphics/drawable.h"
#include "../include/vk2d/core/vk2d_context_impl.h"

VK2D_BEGIN

RenderTexture::RenderTexture() VK2D_NOTHROW :
	texture(),
	render_begin(false)
{}

RenderTexture::RenderTexture(RenderTexture&& rhs) VK2D_NOTHROW :
	texture(std::move(rhs.texture)),
	cmd_pool(std::exchange(rhs.cmd_pool, nullptr)),
	cmd_buffer(std::exchange(rhs.cmd_buffer, nullptr)),
	frame_fence(std::exchange(rhs.frame_fence, nullptr)),
	render_complete(std::exchange(rhs.render_complete, nullptr)),
	render_begin(std::exchange(rhs.render_begin, false))
{}

RenderTexture::RenderTexture(uint32_t width, uint32_t height)
{
	resize(width, height);
}

RenderTexture::~RenderTexture()
{
	destroy();
}

void RenderTexture::draw(const Drawable& drawable)
{
	draw(drawable, RenderOptions());
}

void RenderTexture::draw(const Drawable& drawable, const RenderOptions& options)
{
	if (!render_begin)
		beginRenderPass();

	states.reset(options);
	drawable.draw(*this, states);
}

void RenderTexture::display()
{
	if (!render_begin) return;

	auto& inst = VK2DContext::get();
	auto& device = inst.device;

	cmd_buffer.endRenderPass();
	cmd_buffer.end();

	vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::SubmitInfo submit_info = {
		0, nullptr, {},
		1, &cmd_buffer,
		1, &render_complete
	};
	
	(void)inst.queues[inst.graphics_queue_family_idx].submit(1, &submit_info, frame_fence);

	render_begin = false;

	//uint64_t timeout = UINT64_MAX;
	//vk::SemaphoreWaitInfo wait_info = {
	//	{}, 1, &render_complete, &timeout
	//};

	//device.waitSemaphores(wait_info, timeout);
	device.waitIdle();
}

void RenderTexture::resize(uint32_t width, uint32_t height, const Color& color)
{
	auto& inst   = VK2DContext::get();
	auto& device = inst.device;
	
	device.waitIdle();
	device.destroy(frame_buffer);

	texture.resize_impl(width, height, color,
		vk::ImageUsageFlagBits::eSampled | 
		vk::ImageUsageFlagBits::eTransferSrc |
		vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eColorAttachment
	);

	if (!cmd_pool) {
		vk::CommandPoolCreateInfo pool_info = {
			vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			inst.graphics_queue_family_idx
		};

		cmd_pool = device.createCommandPool(pool_info);

		vk::CommandBufferAllocateInfo buffer_info = {
			cmd_pool, vk::CommandBufferLevel::ePrimary, 1
		};

		cmd_buffer = device.allocateCommandBuffers(buffer_info).front();

		frame_fence     = device.createFence({ vk::FenceCreateFlagBits::eSignaled });
		render_complete = device.createSemaphore({ });
	}

	vk::FramebufferCreateInfo frame_buffer_info = {
		{},
		inst.renderpass,
		1, &texture.image_view,
		width, height, 1,
	};

	frame_buffer = device.createFramebuffer(frame_buffer_info);
}

void RenderTexture::resize(const uvec2& size)
{
	resize(size.x, size.y);
}

void RenderTexture::destroy()
{	
	auto& device = VK2DContext::get().device;

	device.waitIdle();
	device.destroy(frame_buffer);
	if (cmd_pool)
		device.free(cmd_pool, 1, &cmd_buffer);
	device.destroy(cmd_pool);
	device.destroy(frame_fence);
	device.destroy(render_complete);

	frame_buffer    = nullptr;
	cmd_buffer      = nullptr;
	cmd_pool        = nullptr;
	frame_fence     = nullptr;
	render_complete = nullptr;

	texture.destroy();
}

Texture& RenderTexture::getTexture()
{
	return texture;
}

Texture RenderTexture::release()
{
	auto& inst   = VK2DContext::get();
	auto& device = inst.device;
	
	auto ret = std::move(texture);
	destroy();

	auto cmd_buffer = inst.beginSingleTimeCommmand();

	inst.transitionImageLayout(
		cmd_buffer,
		ret.image,
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eShaderReadOnlyOptimal);

	inst.endSingleTimeCommmand(cmd_buffer);

	return std::move(ret);
}

vec2 RenderTexture::size() const
{
	return texture.size();
}

void RenderTexture::beginRenderPass()
{
	auto& inst   = VK2DContext::get();
	auto& device = inst.device;
	auto size    = texture.size();
	
	(void)device.waitForFences(1, &frame_fence, true, UINT64_MAX);
	(void)device.resetFences(1, &frame_fence);

	device.resetCommandPool(cmd_pool);

	{ // reset frame states
		states.defualt_viewport  = vk::Viewport(0.f, 0.f, (float)size.x, (float)size.y);
		states.defualt_scissor   = vk::Rect2D({ 0, 0 }, { size.x, size.y });
		states.default_transform = Transform::identity().
			translate(-1.f, -1.f).
			scale(2.f / size.x, 2.f / size.y);
	}

	cmd_buffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

	vk::RenderPassBeginInfo renderpass_info = {
		inst.renderpass,
		frame_buffer,
		{ { 0, 0 }, { size.x, size.y } },
		1, &clear_value
	};

	cmd_buffer.beginRenderPass(renderpass_info, vk::SubpassContents::eInline);

	render_begin = true;
}

vk::CommandBuffer RenderTexture::getCommandBuffer()
{
	return cmd_buffer;
}

uint32_t RenderTexture::getCurrentFrameIdx() const
{
	return 0;
}

uint32_t RenderTexture::getFrameCount() const
{
	return 1;
}

VK2D_END