#pragma once
#include <cassert>
#include <stdexcept>

#define VK2D_BEGIN namespace vk2d {
#define VK2D_END }

#define VK2D_INLINE inline
#define VK2D_NOTHROW noexcept

#define VK2D_NOCOPY(cls) cls(const cls&) = delete;
#define VK2D_NOMOVE(cls) cls(cls&&) = delete;

#define VK2D_ASSERT(expression) assert(expression)
#define VK2D_ERROR(message) throw std::runtime_error(message);