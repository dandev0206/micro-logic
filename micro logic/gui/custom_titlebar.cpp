#include "custom_titlebar.h"

#include <vk2d/graphics/image.h>
#include <imgui.h>
#include "imgui_custom.h"
#include "../vector_type.h"
#include "../micro_logic_config.h"
#include "../platform/platform_impl.h"

#define ICON_MINIMIZE button_icons, { 20, 0, 160, 100 }
#define ICON_MAXIMIZE button_icons, { 220, 0, 160, 100 }
#define ICON_CLOSE    button_icons, { 420, 0, 160, 100 }

static vk2d::Texture button_icons;
static size_t ref_count = 0;

CustomTitleBar::CustomTitleBar() :
	hovered_button(TitleButton::None),
	button_style(0b111)
{
	if (ref_count++ == 0) {
		vk2d::Image image;
		image.loadFromFile(RESOURCE_DIR_NAME"title_buttons.png");
		
		button_icons.resize(600, 100);
		button_icons.update(image);
	}
}

CustomTitleBar::~CustomTitleBar()
{
	if (--ref_count == 0)
		button_icons.destroy();
}

void CustomTitleBar::bindWindow(vk2d::Window& window)
{
	this->window = &window;
	InjectTitleBar(this);
}

void CustomTitleBar::setButtonStyle(bool minimize, bool maximize, bool close)
{
	button_style = minimize << 0 | maximize << 1 | close << 2;
}

vk2d::Window& CustomTitleBar::getWindow() const
{
	return *window;
}

bool CustomTitleBar::beginTitleBar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
	
	if (ImGui::BeginMainMenuBar()) {
		if (!icon.empty()) {
			ImGui::SetCursorPos(ImVec2(10, 10));
			ImGui::Image(icon, vec2(25, 25));
			ImGui::SetCursorPosX(45);
		}

		if (!title.empty()) {
			ImGui::SetCursorPosY(3);
			ImGui::TextUnformatted(title.c_str());
		}

		return true;
	}

	return false;
}

void CustomTitleBar::endTitleBar()
{
	auto title_height  = ImGui::GetFrameHeight();

	vec2 button_size(title_height * 1.6f, title_height);

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.f, 1.f, 1.f, 1.f));

	hovered_button = TitleButton::None;
	
	auto cursor_pos_x = ImGui::GetWindowSize().x;
	if (button_style & (uint32_t)TitleButton::Close) {
		ImGui::SetCursorPos(ImVec2(cursor_pos_x -= button_size.x, 0.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.83f, 0.13f, 0.2f, 1.f));
		ImGui::ImageButton(ICON_CLOSE, button_size);
		if (ImGui::IsItemHovered())
			hovered_button = TitleButton::Close;
		ImGui::PopStyleColor();
	}
	if (button_style & (uint32_t)TitleButton::Maximize) {
		ImGui::SetCursorPos(ImVec2(cursor_pos_x -= button_size.x, 0.f));
		ImGui::ImageButton(ICON_MAXIMIZE, button_size);
		if (ImGui::IsItemHovered())
			hovered_button = TitleButton::Maximize;
	}
	if (button_style & (uint32_t)TitleButton::Minimize) {
		ImGui::SetCursorPos(ImVec2(cursor_pos_x -= button_size.x, 0.f));
		ImGui::ImageButton(ICON_MINIMIZE, button_size);
		if (ImGui::IsItemHovered())
			hovered_button = TitleButton::Minimize;
	}

	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar(3);
	ImGui::EndMainMenuBar();
}

vk2d::TextureView CustomTitleBar::getIcon() const
{
	return icon;
}

void CustomTitleBar::setIcon(const vk2d::TextureView& icon)
{
	this->icon = icon;
}

std::string CustomTitleBar::getTitle() const
{
	return title;
}

void CustomTitleBar::setTitle(const std::string& title)
{
	this->title = title;
}

Rect CustomTitleBar::getCaptionRect() const
{
	return caption_rect;
}

void CustomTitleBar::setCaptionRect(const Rect& rect)
{
	caption_rect = rect;
}

TitleButton CustomTitleBar::getHoveredButton() const
{
	return hovered_button;
}
