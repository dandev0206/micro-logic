#pragma once

#include "core/include_vulkan.h"

VK2D_BEGIN
VK2D_PRIV_BEGIN

class VK2DContextImpl;

VK2D_PRIV_END

class VK2DContext {
	VK2D_NOCOPY(VK2DContext)
	VK2D_NOMOVE(VK2DContext)

public:
	static VK2D_INLINE VK2D_PRIV_NAME::VK2DContextImpl& get() { return *impl; }

public:
	VK2DContext(const vk::ApplicationInfo& info, std::vector<const char*> layers, std::vector<const char*> extensions, bool debug_enable = false);
	~VK2DContext();

private:
	static VK2D_PRIV_NAME::VK2DContextImpl* impl;
};

VK2D_END