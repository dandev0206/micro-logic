#pragma once

#include <vk2d/system/event.hpp>
#include <string>

#include "../vector_type.h"

class DockingWindow {
public:
	DockingWindow();

	virtual void EventProc(const vk2d::Event& e, float dt) {}
	void EventProcWrapper(const vk2d::Event& e, float dt);

	vec2 getCursorPos() const;

protected:
	void beginDockWindow(const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0);
	void endDockWindow();

public:
	ImGuiID     id;

	Rect window_rect;
	Rect content_rect;

	ImGuiViewport* viewport;
	vk2d::Window*  window;

	bool window_updated;
	bool hovered;
	bool focussed;
	bool show;
	bool visible;

private:
	const char* window_name;
};