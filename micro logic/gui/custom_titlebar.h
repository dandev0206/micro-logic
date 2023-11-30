#pragma once

#include <vk2d/system/window.h>
#include <vk2d/graphics/texture.h>
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

	void showButtons();
	void showButtonsWithMenu();

	void setButtonStyle(bool minimize, bool maximize, bool close);

	Rect getCaptionRect() const;
	void setCaptionRect(const Rect& rect);

	TitleButton getHoveredButton() const;

private:
	TitleButton hovered_button;
	uint32_t    button_style;
	Rect        caption_rect;
};