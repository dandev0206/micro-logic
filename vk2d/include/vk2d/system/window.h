#pragma once

#include "../core/rect.h"
#include "../system/event.h"
#include "../graphics/render_target.h"

VK2D_BEGIN
VK2D_PRIV_BEGIN

class WindowImpl;

VK2D_PRIV_END

using WindowStyleFlags = uint32_t;

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

	Window() VK2D_NOTHROW;
	Window(Window&& rhs) VK2D_NOTHROW;
	Window(uint32_t width, uint32_t height, const char* title, WindowStyleFlags style = Style::Default);
	Window(uint32_t width, uint32_t height, const char* title, const Window& parent, WindowStyleFlags style = Style::Default);
	~Window();

	void draw(const Drawable& drawable) override;
	void draw(const Drawable& drawable, const RenderOptions& options) override;
	void display() override;

	bool pollEvent(Event& event);
	bool waitEvent(Event& event);
	bool hasEvent() const;

	void create(uint32_t width, uint32_t height, const char* title, WindowStyleFlags style = Style::Default);
	void create(uint32_t width, uint32_t height, const char* title, const Window& parent, WindowStyleFlags style = Style::Default);
	void close();

	bool isClosed() const;

	ivec2 getPosition() const;
	void setPosition(const ivec2 pos);

	uvec2 getSize() const;
	void setSize(const uvec2& size) const;

	uvec2 getFrameBufferSize() const;
	void setFrameBufferSize(const uvec2& size) const;

	uvec2 getMinSizeLimit() const;
	uvec2 getMaxSizeLimit() const;
	void setMinSizeLimit(const uvec2& size); // set (0, 0) to unlimit
	void setMaxSizeLimit(const uvec2& size); // set (0, 0) to unlimit

	float getTransparency() const;
	void setTransparency(float value);

	bool isVisible() const;
	void setVisible(bool value);

	bool isResizable() const;
	void setResizable(bool value);

	void setParent(const vk2d::Window& window = {});

	void enableVSync(bool value);

	bool isDisabled() const;
	void setDisabled(bool value);

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
	VK2D_PRIV_NAME::WindowImpl* impl;
};

VK2D_END