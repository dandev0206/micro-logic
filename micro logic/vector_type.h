#pragma once

#include <imgui.h>
#include <vk2d/core/vector_type.h>
#include <vk2d/core/rect.h>
#include <vk2d/core/color.h>

using vec2  = vk2d::vec2;
using ivec2 = vk2d::ivec2;
using uvec2 = vk2d::uvec2;
using vec3  = vk2d::vec3;
using ivec3 = vk2d::ivec3;
using uvec3 = vk2d::uvec3;
using vec4  = vk2d::vec4;
using ivec4 = vk2d::ivec4;
using uvec4 = vk2d::uvec4;

using Rect  = vk2d::Rect;
using iRect = vk2d::iRect;
using uRect = vk2d::uRect;

using Color = vk2d::Color;

static inline ImVec2 to_ImVec2(const vec2& v) {
	return { v.x, v.y };
}

static inline ImVec2 to_ImVec2(const ivec2& v) {
	return { (float)v.x, (float)v.y };
}

static inline ImVec2 to_ImVec2(const uvec2& v) {
	return { (float)v.x, (float)v.y };
}

static inline vec2 to_vec2(const ImVec2& v) {
	return { v.x, v.y };
}

static inline ivec2 to_ivec2(const ImVec2& v) {
	return { (int32_t)v.x, (int32_t)v.y };
}

static inline uvec2 to_uvec2(const ImVec2& v) {
	return { (uint32_t)v.x, (uint32_t)v.y };
}

static inline ImColor to_ImColor(const vk2d::Color& color) {
	return { color.r, color.g, color.b, color.a };
}

static inline Rect scale_rect(const Rect& rect, float scale) {
	return { rect.left * scale, rect.top * scale, rect.width * scale, rect.height * scale };
}