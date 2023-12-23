#pragma once

#include "../gui/docking_window.h"
#include "../side_menus.h"

class Window_Library : public DockingWindow {
public:
	Window_Library();

	void showUI();
	void EventProc(const vk2d::Event& e, float dt) override;

	void bindMenuLibrary(Menu_Library& menu);
	void updateElementTree();
	
private:
	void showSelectable(const LogicGate& gate);
	void updateRecentlyUsed();
	void updateFilter();
	void resetPreview();

public:
	struct ElementNode {
		void display(Window_Library& wl);

		std::string name;
		std::string description;
		LogicGate*  logic_gate;

		bool show;

		std::vector<ElementNode> childs;
	};

	ElementNode   tree;
	Menu_Library* menu;

	std::vector<int32_t> recently_used;
	std::string          filter;

	Rect preview_rect;
	vec2 preview_pos;
	float preview_scale;
	bool mouse_captured;
	bool is_moving;
};