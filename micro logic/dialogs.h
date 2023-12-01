#pragma once

#include <vk2d/system/window.h>
#include <vk2d/graphics/texture_view.h>
#include "gui/dialog.h"

class MessageBox : public Dialog {
public:
	MessageBox();

	std::string showDialog();

	std::string       buttons; // separated by ;
	std::string       title;
	std::string       content;
	vk2d::TextureView icon;
};