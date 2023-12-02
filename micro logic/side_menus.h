#pragma once

#include "side_menu.h"
#include "circuit_element.h"
#include "bvh.hpp"

class Menu_Info : public SideMenu
{
public:
	Menu_Info();

	void loop() override;
	void eventProc(const vk2d::Event& e, float dt) override;
	void menuButton() override;
};

class Menu_Zoom : public SideMenu
{
public:
	Menu_Zoom();

	void loop() override;
	void eventProc(const vk2d::Event& e, float dt) override;
	void menuButton() override;
	void upperMenu() override;

	int curr_zoom_type;
	vec2 zoom_begin;
};

class SelectingSideMenu abstract : public SideMenu {
public:
	SelectingSideMenu(const char* menu_name);

	void loop() override;
	void eventProc(const vk2d::Event& e, float dt) override;
	void upperMenu() override;

	virtual void beginWork();
	virtual void endWork();
	virtual void loopWork() = 0;

	bool isSelectedElement(const vec2& pos);
	bool selectElement(const AABB& aabb);
	void deleteSelectedElement();
	bool checkBlocked(const AABB& elem) const;

	bool                        is_working;
	bool                        blocked;
	vec2                        start_pos;
	vec2                        last_pos;
	Direction                   path;
	Direction                   last_dir;
};

class Menu_Select : public SelectingSideMenu
{
public:
	Menu_Select();

	void loop() override;
	void menuButton() override;
	void onClose() override;

	void beginWork() override;
	void endWork() override;
	void loopWork() override;
};

class Menu_Library : public SideMenu
{
public:
	Menu_Library();

	void loop() override;
	void eventProc(const vk2d::Event& e, float dt) override;
	void menuButton() override;
	void upperMenu() override;

	int32_t   curr_gate;
	Direction curr_dir;
};

class Menu_Unit : public SideMenu
{
public:
	Menu_Unit();

	void loop() override;
	void menuButton() override;
};

class Menu_Shapes : public SideMenu
{
public:
	Menu_Shapes();

	void loop() override;
	void menuButton() override;
};

class Menu_Label : public SideMenu
{
public:
	Menu_Label();

	void loop() override;
	void menuButton() override;
};

class Menu_Copy : public SelectingSideMenu
{
public:
	Menu_Copy();

	void loop() override;
	void menuButton() override;
	void onBegin() override;
	void onClose() override;

	void beginWork() override;
	void endWork() override;
	void loopWork() override;

	void beginPaste(bool from_clipboard);
	void beginClipboardPaste(std::istream& is);

	std::vector<std::unique_ptr<CircuitElement>> elements;
	SideMenu* prev_menu;
	bool from_clipboard;
};

class Menu_Cut : public SelectingSideMenu
{
public:
	Menu_Cut();

	void loop() override;
	void menuButton() override;
	void onBegin() override;
	void onClose() override;

	void beginWork() override;
	void endWork() override;
	void loopWork() override;

	void beginPaste(bool from_clipboard);
	void beginClipboardPaste(std::istream& is);

	std::vector<std::unique_ptr<CircuitElement>> elements;
	SideMenu* prev_menu;
	bool from_clipboard;
};

class Menu_Delete : public SideMenu
{
public:
	Menu_Delete();

	void loop() override;
	void eventProc(const vk2d::Event& e, float dt) override;
	void menuButton() override;
	void onBegin() override;
};

class WiringSideMenu abstract : public SideMenu
{
public:
	WiringSideMenu(const char* menu_name);

	vec2 getMiddle(const vec2& p0, const vec2& p1) const;
	bool checkContinueWiring(const vec2& pos) const;

	int curr_wire_type;
	bool is_wiring;
	vec2 start_pos;
};

class Menu_Wire : public WiringSideMenu
{
public:
	Menu_Wire();

	void loop() override;
	void eventProc(const vk2d::Event& e, float dt) override;
	void menuButton() override;
	void upperMenu() override;

	void addWires(std::vector<Wire>&& stack);
	bool checkWireCrossing(const vec2& pos) const;
};

class Menu_Net : public WiringSideMenu
{
public:
	Menu_Net();

	void loop() override;
	void eventProc(const vk2d::Event& e, float dt) override;
	void menuButton() override;
	void upperMenu() override;
};

class Menu_SplitWire : public SideMenu
{
public:
	Menu_SplitWire();

	void loop() override;
	void eventProc(const vk2d::Event& e, float dt) override;
	void menuButton() override;
};