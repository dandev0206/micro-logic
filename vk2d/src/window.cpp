#ifdef _WIN32
#include "platform/windows_window_impl.h"
#endif

#include "../include/vk2d/graphics/drawable.h"

VK2D_BEGIN

Window::Window(uint32_t width, uint32_t height, const char* title, int32_t style)
{
	create(width, height, title, style);
}

Window::Window(uint32_t width, uint32_t height, const char* title, const Window& parent, int32_t style)
{
	create(width, height, title, parent, style);
}

Window::~Window()
{
	close();
}

void Window::draw(const Drawable& drawable)
{
	draw(drawable, RenderOptions());
}

void Window::draw(const Drawable& drawable, const RenderOptions& options)
{
	if (impl->minimized) return;

	if (!impl->render_begin)
		beginRenderPass();

	auto& frame = impl->frames[impl->frame_idx];
	frame.states.reset(options);
	drawable.draw(*this, frame.states);
}

void Window::display()
{
	if (!impl->render_begin || impl->minimized) return;

	auto& inst   = VKInstance::get();
	auto& device = inst.device;
	auto& frame  = impl->frames[impl->frame_idx];
	
	auto& cmd_buffer      = frame.cmd_buffer;
	auto& semaphores      = impl->semaphores[impl->semaphore_idx];
	auto& image_acquired  = semaphores.image_acquired;
	auto& render_complete = semaphores.render_complete;

	cmd_buffer.endRenderPass();
	cmd_buffer.end();

	vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::SubmitInfo submit_info = {
		1, &image_acquired,
		&wait_stage,
		1, &cmd_buffer,
		1, &render_complete
	};

	(void)inst.queues[inst.graphics_queue_family_idx].submit(1, &submit_info, frame.fence);

	vk::PresentInfoKHR present_info = {
		1, &render_complete,
		1, &impl->swapchain,
		&impl->frame_idx,
	};

	(void)inst.queues[impl->present_queue_family_idx].presentKHR(&present_info);
	impl->semaphore_idx = (impl->semaphore_idx + 1) % impl->semaphores.size();
	impl->render_begin = false;
}

bool Window::pollEvent(Event& event)
{
	if (!impl) return false;
	return impl->pollEvent(event);
}

bool Window::waitEvent(Event& event)
{
	return impl->waitEvent(event);
}

void Window::create(uint32_t width, uint32_t height, const char* title, int32_t style)
{
	VK2D_ASSERT(!impl && "window already created");
	
	impl = new WindowImpl(width, height, title, nullptr, style);
}

void Window::create(uint32_t width, uint32_t height, const char* title, const Window& parent, int32_t style)
{
	VK2D_ASSERT(!impl && "window already created");
	VK2D_ASSERT(parent.impl && "parent is closed window");

	impl = new WindowImpl(width, height, title, parent.impl, style);
}

void Window::close()
{
	delete impl;
	impl = nullptr;
}

bool Window::isClosed() const
{
	return impl == nullptr;
}

glm::ivec2 Window::getPosition() const
{
	return impl->position;
}

void Window::setPosition(const glm::ivec2 pos)
{
	impl->setPosition(pos);
}

glm::uvec2 Window::getSize() const
{
	return impl->size;
}

void Window::setSize(const glm::uvec2& size) const
{
	impl->setSize(size);
}

uvec2 Window::getFrameBufferSize() const
{
	return impl->fb_size;
}

void Window::setFrameBufferSize(const uvec2& size) const
{
	impl->setFraneBufferSize(size);
}

uvec2 Window::getMinSizeLimit() const
{
	return impl->size_limit_min;
}

uvec2 Window::getMaxSizeLimit() const
{
	return impl->size_limit_max;
}

void Window::setMinSizeLimit(const uvec2& size)
{
	impl->setMinSizeLimit(size);
}

void Window::setMaxSizeLimit(const uvec2& size)
{
	impl->setMaxSizeLimit(size);
}

float Window::getTransparency() const
{
	return impl->transparency;
}

void Window::setTransparency(float value)
{
	return impl->setTransparency(value);
}

bool Window::isVisible() const
{
	return impl->visible;
}

void Window::setVisible(bool value)
{
	impl->setVisible(value);
}

bool Window::isResizable() const
{
	return impl->isResizable();
}

void Window::setResizable(bool value)
{
	return impl->setResizable(value);
}

void Window::setParent(const vk2d::Window& window)
{
	if (window.impl)
		impl->setParent(window.impl);
	else
		impl->setParent(nullptr);
}

void Window::enableVSync(bool value)
{
	if (value)
		impl->present_mode = vk::PresentModeKHR::eFifo;
	else
		impl->present_mode = vk::PresentModeKHR::eImmediate;

	impl->update_swapchain = true;
}

bool Window::isDisabled() const
{
	return impl->isDisabled();
}

void Window::setDisabled(bool value)
{
	impl->setDisabled(value);
}

const char* Window::getTitle() const
{
	return impl->title.c_str();
}

void Window::setTitle(const char* title)
{
	impl->setTitle(title);
}

bool Window::isMinimized() const
{
	return impl->minimized;
}

void Window::minimize()
{
	impl->minimize();
}

bool Window::isMaximized() const
{
	return impl->maximized;
}

void Window::maximize()
{
	impl->maximize();
}

bool Window::hasFocus() const
{
	return impl->focussed;
}

void Window::setFocus()
{
	impl->setFocus();
}

void Window::restore()
{
	impl->restore();
}

void* Window::getUserPtr()
{
	return impl->user_ptr;
}

void Window::setUserPtr(void* ptr)
{
	impl->user_ptr = ptr;
}

void* Window::getNativeHandle() const
{
#ifdef _WIN32
	return impl->hwnd;
#endif
}

void Window::beginRenderPass()
{
	if (impl->render_begin) return;

	auto& inst   = VKInstance::get();
	auto& device = inst.device;

	impl->acquireSwapchainImage();
	
	auto& frame = impl->frames[impl->frame_idx];

	(void)device.waitForFences(1, &frame.fence, true, UINT64_MAX);
	(void)device.resetFences(1, &frame.fence);

	auto fb_size     = impl->fb_size;
	auto& cmd_buffer = frame.cmd_buffer;
	auto& cmd_pool   = frame.cmd_pool;
	device.resetCommandPool(cmd_pool);

	{ // reset frame states
		frame.states.defualt_viewport  = vk::Viewport(0.f, 0.f, (float)fb_size.x, (float)fb_size.y);
		frame.states.defualt_scissor   = vk::Rect2D({ 0, 0 }, { fb_size.x, fb_size.y });
		frame.states.default_transform = Transform::identity().
			translate(-1.f, -1.f).
			scale(2.f / fb_size.x, 2.f / fb_size.y);
	}

	cmd_buffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

	vk::RenderPassBeginInfo renderpass_info = {
		inst.renderpass,
		frame.frameBuffer,
		{ { 0, 0 }, { fb_size.x, fb_size.y } },
		1, &clear_value
	};

	cmd_buffer.beginRenderPass(renderpass_info, vk::SubpassContents::eInline);

	impl->render_begin = true;
}

vk::CommandBuffer Window::getCommandBuffer()
{
	return impl->frames[impl->frame_idx].cmd_buffer;
}

uint32_t Window::getCurrentFrameIdx() const
{
	return impl->frame_idx;
}

uint32_t Window::getFrameCount() const
{
	return (uint32_t)impl->frames.size();
}

VK2D_END