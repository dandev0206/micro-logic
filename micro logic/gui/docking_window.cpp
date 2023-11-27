#include "docking_window.h"

#include "imgui_impl_vk2d.h"
#include <imgui_internal.h>

static inline ImRect to_ImRect(const vk2d::Rect& rect) 
{
	return { to_ImVec2(rect.getPosition()), to_ImVec2(rect.getSize()) };
}

static void event_proc(const vk2d::Event& e, float dt, void* user_data)
{
	static_cast<DockingWindow*>(user_data)->EventProcWrapper(e, dt);
}

DockingWindow::DockingWindow() :
	window_name(nullptr),
	id(0),
	viewport(nullptr),
	window(nullptr),
	window_updated(false),
	hovered(false),
	focussed(false),
	show(true),
	visible(false)
{}

void DockingWindow::EventProcWrapper(const vk2d::Event& e, float dt)
{
	using vk2d::Event;

	if (e.type == Event::WheelScrolled && window_rect.contain(e.wheel.pos)) {
		if (visible && !focussed) ImGui::SetWindowFocus(window_name);
	}

	EventProc(e, dt);
}

vec2 DockingWindow::getCursorPos() const
{
	return (vec2)vk2d::Mouse::getPosition(*window);
}

void DockingWindow::beginDockWindow(const char* name, bool* p_open, ImGuiWindowFlags flags)
{	
	auto prev_size    = window_rect.getSize();
	auto prev_visible = visible; 
	window_updated    = false;

	if (visible = ImGui::Begin(name, p_open, flags)) {
		auto* viewport = ImGui::GetWindowViewport();
		ImGui::VK2D::setViewportEventProc(viewport, event_proc, this);
	}

	window_updated |= prev_visible != visible;

	auto pos      = to_vec2(ImGui::GetWindowPos()) - to_vec2(ImGui::GetWindowViewport()->Pos);
	auto padding  = to_vec2(ImGui::GetStyle().WindowPadding);

	window_name    = name;
	id		       = ImGui::GetID(this);
	viewport       = ImGui::GetCurrentWindow()->Viewport;
	window	       = (vk2d::Window*)ImGui::GetWindowViewport()->PlatformHandle;

	window_rect.setPosition(to_vec2(ImGui::GetWindowContentRegionMin()) + pos - padding);
	window_rect.setSize(to_vec2(ImGui::GetWindowContentRegionMax()) + pos + padding - window_rect.getPosition());
	window_updated |= prev_size != window_rect.getSize();
	
	content_rect.setPosition(to_vec2(ImGui::GetWindowContentRegionMin()) + pos);
	content_rect.setSize(to_vec2(ImGui::GetWindowContentRegionMax()) + pos - content_rect.getPosition());
	
	focussed = ImGui::IsWindowFocused();
	hovered  = ImGui::IsWindowHovered();
}

void DockingWindow::endDockWindow()
{
	ImGui::End();
}
