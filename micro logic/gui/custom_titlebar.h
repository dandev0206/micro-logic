#pragma once

#include <vk2d/system/window.h>
#include <vk2d/graphics/texture_view.h>
#include "../vector_type.h"

enum class TitleButton {
	None     = 0,
	Minimize = 1 << 0,
	Maximize = 1 << 1,
	Close    = 1 << 2
};

class CustomTitleBar {
public:
	CustomTitleBar();
	~CustomTitleBar();

	void bindWindow(vk2d::Window& window);
	void setButtonStyle(bool minimize, bool maximize, bool close);

	vk2d::Window& getWindow() const;

	bool beginTitleBar();
	void endTitleBar();

	vk2d::TextureView getIcon() const;
	void setIcon(const vk2d::TextureView& icon);

	std::string getTitle() const;
	void setTitle(const std::string& title);

	Rect getCaptionRect() const;
	void setCaptionRect(const Rect& rect);

	TitleButton getHoveredButton() const;

private:
	vk2d::Window*     window;
	vk2d::TextureView icon;
	std::string       title;
	TitleButton       hovered_button;
	uint32_t          button_style;
	Rect              caption_rect;
};