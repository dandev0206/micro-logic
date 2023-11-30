#pragma once

#include <vk2d/system/window.h>
#include <vk2d/graphics/texture_view.h>
#include "custom_titlebar.h"

class MessageBox {
public:
	MessageBox();
	~MessageBox();

	void init(const vk2d::Window* owner = nullptr);

	const vk2d::Window& getWindow() const;
	const CustomTitleBar& getTitlebar() const;

	std::string showDialog();

	std::string          buttons; // separated by ;
	std::string          title;
	std::string          content;
	vk2d::TextureView    icon;
	const vk2d::Window*  owner;

private:
	vk2d::Window   window;
	CustomTitleBar titlebar;
};