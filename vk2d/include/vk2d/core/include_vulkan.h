#pragma once

#include "vk2d_def.h"

#ifdef VK2D_USE_PLATFORM
#ifdef VK2D_PLATFORM_WINDOWS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#endif

#include <vulkan/vulkan.hpp>

#define VK2D_CHECK(expression) VK2D_ASSERT((expression) == vk::Result::eSuccess)