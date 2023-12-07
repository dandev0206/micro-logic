#pragma once

#include <vk2d/graphics/texture.h>
#include <vk2d/system/event.h>

class SideMenu abstract {
public:
	SideMenu(const char* menu_name) :
		menu_name(menu_name) {}

	virtual void loop() = 0;
	virtual void eventProc(const vk2d::Event& e, float dt) {};
	virtual void menuButton() = 0;
	virtual void upperMenu() {};
	virtual void onBegin() {};
	virtual void onClose() {};

protected:
	void menuButtonImpl(const vk2d::Texture& texture, const vk2d::Rect& rect, const vk2d::vec2& size);
	
public:
	const char* menu_name;
};