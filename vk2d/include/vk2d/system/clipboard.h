#pragma once

#include <string>
#include "../graphics/image.h"

VK2D_BEGIN

class Clipboard {
public:
	static bool available();
	static std::string getString();
	static void setString(const std::string& str);
};

VK2D_END