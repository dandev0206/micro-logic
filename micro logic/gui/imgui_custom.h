#pragma once

#include <vk2d/graphics/texture.h>
#include <vk2d/graphics/texture_view.h>

namespace ImGui
{
	void Image(const vk2d::Texture& texture,
		const vk2d::Rect& rect,
		const vk2d::vec2& size,
		const vk2d::Color& tintColor   = vk2d::Colors::White,
		const vk2d::Color& borderColor = vk2d::Colors::Transparent);

	void Image(const vk2d::TextureView texture_view,
		const vk2d::vec2& size,
		const vk2d::Color& tintColor = vk2d::Colors::White,
		const vk2d::Color& borderColor = vk2d::Colors::Transparent);

	bool ImageButton(const vk2d::Texture& texture,
		const vk2d::Rect& rect,
		const vk2d::vec2& size,
		const vk2d::Color& bgColor   = vk2d::Colors::Transparent,
		const vk2d::Color& tintColor = vk2d::Colors::White);

	bool ToggleImageButton(const vk2d::Texture& texture,
		const vk2d::Rect& rect,
		const vk2d::vec2& size,
		int* v, int v_button);

	bool RadioButton(const char* label,
		int* v, int v_button, const vk2d::vec2& size);

	bool RadioImageButton(const vk2d::Texture& texture,
		const vk2d::Rect& rect,
		const vk2d::vec2& size,
		int* v, int v_button);

	bool ZoomSlider(const char* label, float* v, float v_min, float v_max);

	bool ColorEdit4(const char* label, vk2d::Color* col, int flags = 0);

	void MakeTabVisible(const char* window_name);
}