#pragma once

#include <vk2d/system/event.hpp>

class SideMenu abstract {
public:
	SideMenu(const char* menu_name, uint32_t menu_idx) :
		menu_name(menu_name),
		menu_idx(menu_idx)
	{}

	virtual void loop() = 0;
	virtual void eventProc(const vk2d::Event& e, float dt) {};
	virtual void menuButton() = 0;
	virtual void upperMenu() {};
	virtual void onBegin() {};
	virtual void onClose() {};

	const char* menu_name;
	uint32_t    menu_idx;
};