#include "imgui_custom.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include "../vector_type.h"

static ImTextureID get_texture_ID(const vk2d::Texture& texture)
{
	auto descriptor_set = texture.getDescriptorSet();
	return *(ImTextureID*)&descriptor_set;
}

int32_t create_hash(int32_t hash, const ImTextureID& val) 
{
	return hash ^ (intptr_t)val >> 0 ^ (intptr_t)val >> 32;
}

int32_t create_hash(int32_t hash, const Rect& val)
{
	auto* raw = (int32_t*)(&val);
	return hash ^ raw[0] ^ raw[1] ^ raw[2] ^ raw[3];
}

namespace ImGui {

void Image(const vk2d::Texture& texture, const vk2d::Rect& rect, const vk2d::vec2& size, const vk2d::Color& tintColor, const vk2d::Color& borderColor)
{
	auto texture_size = texture.size();
	auto texture_id   = get_texture_ID(texture);

	ImVec2 uv0(rect.left / texture_size.x, rect.top / texture_size.y);
	ImVec2 uv1((rect.left + rect.width) / texture_size.x, (rect.top + rect.height) / texture_size.y);

	Image(texture_id, ImVec2(size.x, size.y), uv0, uv1, to_ImColor(tintColor), to_ImColor(borderColor));
}

void Image(const vk2d::TextureView texture_view, const vk2d::vec2& size, const vk2d::Color& tintColor, const vk2d::Color& borderColor)
{
	Image(texture_view.getTexture(), texture_view.getRegion(), size, tintColor, borderColor);
}

bool ImageButton(const vk2d::Texture& texture, const vk2d::Rect& rect, const vk2d::vec2& size, const vk2d::Color& bgColor, const vk2d::Color& tintColor)
{
	auto window       = GetCurrentWindow();
	auto texture_size = texture.size();
	auto texture_id   = get_texture_ID(texture);

	ImVec2 uv0(rect.left / texture_size.x, rect.top / texture_size.y);
	ImVec2 uv1((rect.left + rect.width) / texture_size.x, (rect.top + rect.height) / texture_size.y);

	int hash = 0;
	hash = create_hash(hash, texture_id);
	hash = create_hash(hash, rect);

	return ImageButtonEx(
		window->GetID(hash),
		texture_id, 
		ImVec2(size.x, size.y),
		uv0,
		uv1,
		to_ImColor(bgColor),
		to_ImColor(tintColor));
}

bool ToggleImageButton(const vk2d::Texture& texture, const vk2d::Rect& rect, const vk2d::vec2& size, int* v, int v_button)
{
	vk2d::Color bgColor = (*v == v_button) ? vk2d::Colors::Gray : vk2d::Colors::Transparent;

	bool ret = false;

	PushID(v_button);

	if (ImageButton(texture, rect, size, bgColor)) {
		if (*v == v_button) {
			*v = -1;
		} else {
			*v = v_button;
		}
		ret = true;
	}

	PopID();
	return ret;
}

bool RadioButton(const char* label, int* v, int v_button, const vk2d::vec2& size)
{
	bool ret    = false;
	bool active = *v == v_button;
	
	if (active) 
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 1.f));
	
	if (Button(label, to_ImVec2(size))) {
		*v = v_button;
		ret = true;
	}
	
	if (active) 
		ImGui::PopStyleColor();

	return ret;
}

bool RadioImageButton(const vk2d::Texture& texture, const vk2d::Rect& rect, const vk2d::vec2& size, int* v, int v_button)
{
	vk2d::Color bgColor = (*v == v_button) ? vk2d::Colors::Gray : vk2d::Colors::Transparent;

	bool ret = false;

	PushID(v_button);

	if (ImageButton(texture, rect, size, bgColor)) {
		*v = v_button;
		ret = true;
	}

	PopID();
	return ret;
}

bool ZoomSlider(const char* label, float* v, float v_min, float v_max)
{
	auto flags = ImGuiSliderFlags_NoInput | ImGuiSliderFlags_Logarithmic;

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g         = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id        = window->GetID(label);
    const float w           = CalcItemWidth();
	const float h           = GetFontSize();

    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, h + style.FramePadding.y * 2.0f));

    ItemSize(frame_bb, style.FramePadding.y);
    if (!ItemAdd(frame_bb, id, &frame_bb))
        return false;

    const bool hovered     = ItemHoverable(frame_bb, id, g.LastItemData.InFlags);
	const bool clicked     = hovered && IsMouseClicked(0, id);
	const bool make_active = clicked || g.NavActivateId == id;

	if (make_active && clicked)
		SetKeyOwner(ImGuiKey_MouseLeft, id);

	if (make_active) {
		SetActiveID(id, window);
		SetFocusID(id, window);
		FocusWindow(window);
		g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
	}

    // Draw frame
	auto center   = (frame_bb.Min + frame_bb.Max) / 2.f;
	auto width    = (h + style.FramePadding.y * 2.0f) * 0.05f;
	auto line_bb  = ImRect(frame_bb.Min.x, center.y - width, frame_bb.Max.x, center.y + width);
	auto left_bb  = ImRect(frame_bb.Min.x, center.y - h / 2 , frame_bb.Min.x + width, center.y + h / 2);
	auto right_bb = ImRect(frame_bb.Max.x - width, center.y - h / 2, frame_bb.Max.x, center.y + h / 2);
	RenderNavHighlight(frame_bb, id);

	window->DrawList->AddRectFilled(line_bb.Min, line_bb.Max, 0xffffffff);
	window->DrawList->AddRectFilled(left_bb.Min, left_bb.Max, 0xffffffff);
	window->DrawList->AddRectFilled(right_bb.Min, right_bb.Max, 0xffffffff);
	window->DrawList->AddRectFilled(center - ImVec2(width / 2, h / 3), center + ImVec2(width / 2, h / 3),  0xffffffff);

    // Slider behavior
    ImRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, ImGuiDataType_Float, v, &v_min, &v_max, "", flags, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    // Render grab
	if (grab_bb.Max.x > grab_bb.Min.x) {
		window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);
	}

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
    return value_changed;
}

bool ColorEdit4(const char* label, vk2d::Color* col, int flags)
{
	float arr[] = {
		col->r / 255.f,
		col->g / 255.f,
		col->b / 255.f,
		col->a / 255.f,
	};

	if (ColorEdit4(label, arr, flags)) {
		col->r = (uint8_t)(arr[0] * 255.99f);
		col->g = (uint8_t)(arr[1] * 255.99f);
		col->b = (uint8_t)(arr[2] * 255.99f);
		col->a = (uint8_t)(arr[3] * 255.99f);
		return true;
	}

	return false;
}

}