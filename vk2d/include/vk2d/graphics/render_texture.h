#pragma once

#include "texture.h"
#include "render_target.h"
#include "render_states.h"

VK2D_BEGIN

class RenderTexture : public RenderTarget {
public:
	RenderTexture() VK2D_NOTHROW;
	RenderTexture(RenderTexture&& rhs) VK2D_NOTHROW;
	RenderTexture(uint32_t width, uint32_t height);
	~RenderTexture();

	void draw(const Drawable& drawable) override;
	void draw(const Drawable& drawable, const RenderOptions& options) override;
	void display() override;

	void resize(uint32_t width, uint32_t height, const Color& color = Colors::Black);
	void resize(const uvec2& size);
	void destroy();

	Texture& getTexture();
	Texture release();

	vec2 size() const;

	RenderTexture& operator=(Texture&& rhs) VK2D_NOTHROW;

private:
	void beginRenderPass() override;
	vk::CommandBuffer getCommandBuffer() override;
	uint32_t getCurrentFrameIdx() const override;
	uint32_t getFrameCount() const override;

private:
	Texture texture;

	vk::Framebuffer   frame_buffer;
	vk::CommandPool   cmd_pool;
	vk::CommandBuffer cmd_buffer;
	vk::Fence         frame_fence;
	vk::Semaphore     render_complete;

	RenderStates      states;
	bool              render_begin;
};

VK2D_END