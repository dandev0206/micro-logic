#pragma once

#include <memory>
#include "../graphics/render_target.h"
#include "../system/event.hpp"
#include "../rect.hpp"

VK2D_BEGIN

class WindowImpl;

class Window : public RenderTarget
{
	VK2D_NOCOPY(Window)

public:
	enum Style {
		None      = 0,
		Resizable = 1 << 0,
		TopMost   = 1 << 1,
		Visible   = 1 << 2,
		Minimize  = 1 << 3,
		Maximize  = 1 << 4,
		Close     = 1 << 5,
		Titlebar  = Minimize | Maximize | Close,
		Default   = Titlebar | Resizable | Visible
	};

	Window() VK2D_NOTHROW = default;
	Window(Window&& rhs) VK2D_NOTHROW = default;
	Window(uint32_t width, uint32_t height, const char* title, int32_t style = Style::Default);
	Window(uint32_t width, uint32_t height, const char* title, const Window& parent, int32_t style = Style::Default);
	~Window();

	void draw(const Drawable& drawable) override;
	void draw(const Drawable& drawable, const RenderOptions& options) override;
	void display() override;

	bool pollEvent(Event& event);
	bool waitEvent(Event& event);

	void create(uint32_t width, uint32_t height, const char* title, int32_t style = Style::Default);
	void create(uint32_t width, uint32_t height, const char* title, const Window& parent, int32_t style = Style::Default);
	void close();

	bool isClosed() const;

	ivec2 getPosition() const;
	void setPosition(const ivec2 pos);

	uvec2 getSize() const;
	void setSize(const uvec2& size) const;

	uvec2 getFrameBufferSize() const;
	void setFrameBufferSize(const uvec2& size) const;

	float getTransparency() const;
	void setTransparency(float value);

	bool isVisible() const;
	void setVisible(bool value);

	const char* getTitle() const;
	void setTitle(const char* title);

	bool isMinimized() const;
	void minimize();

	bool isMaximized() const;
	void maximize();

	bool hasFocus() const;
	void setFocus();

	void restore();

	void* getUserPtr();
	void setUserPtr(void* ptr);

	void* getNativeHandle() const;

private:
	void beginRenderPass() override;
	vk::CommandBuffer getCommandBuffer() override;
	uint32_t getCurrentFrameIdx() const override;
	uint32_t getFrameCount() const override;

private:
	WindowImpl* impl;
};

VK2D_END