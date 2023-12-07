#include "side_menus.h"

#include <vk2d/system/cursor.h>

#include "gui/imgui_custom.h"
#include "main_window.h"
#include "command.h"
#include "math_utils.h"
#include "micro_logic_config.h"

#define ICON_TEXTURE_VAR main_window.textures[TEXTURE_ICONS_IDX]
#include "icons.h"

#define DRAG_THRESHOLD 5

using vk2d::Event;
using vk2d::Mouse;
using vk2d::Keyboard;
using vk2d::Key;

static Window_Sheet& getCurrentWindowSheet()
{
	return MainWindow::get().getCurrentWindowSheet();
}

static AABB point_to_AABB(const vec2& p) {
	static const vec2 padding(0.2f, 0.2f);
	return { p - padding, p + padding };
}

void SideMenu::menuButtonImpl(const vk2d::Texture& texture, const vk2d::Rect& rect, const vk2d::vec2& size)
{
	auto& main_window = MainWindow::get();

	auto bgColor = 
		main_window.curr_menu == this ?
		vk2d::Colors::Gray :
		vk2d::Colors::Transparent;

	if (ImGui::ImageButton(texture, rect, size, bgColor)) {
		if (main_window.curr_menu == this)
			main_window.setCurrentSideMenu(nullptr);
		else
			main_window.setCurrentSideMenu(this);
	}
}

void selectConnectedWire(const CircuitElement& elem, Command_Select& cmd)
{
	switch (elem.getType()) {
	case CircuitElement::LogicGate:
	case CircuitElement::LogicUnit:
		const auto& logic_elem = static_cast<const LogicElement&>(elem);

		for (const auto& layout : logic_elem.shared->pin_layouts) {
			auto& ws = getCurrentWindowSheet();

			// if (logic_elem.pins[layout.pinout - 1].net != nullptr) {
			// 
			// } TODO: uncomment later
		}
	}
}

Menu_Info::Menu_Info() :
	SideMenu("Info")
{}

void Menu_Info::loop()
{
}

void Menu_Info::eventProc(const vk2d::Event& e, float dt)
{
}

void Menu_Info::menuButton()
{
	auto& main_window = MainWindow::get();
	SideMenu::menuButtonImpl(ICON_INFORMATION, { 40, 40 });
}

Menu_Zoom::Menu_Zoom() :
	SideMenu("Zoom")
{}

void Menu_Zoom::loop()
{
	auto& ws = getCurrentWindowSheet();

	if (curr_zoom_type == 2) {
		ws.showDragRect(Mouse::Left);
	} else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		auto delta = ImGui::GetIO().MouseDelta;
		auto r = sqrtf(delta.x * delta.x + delta.y * delta.y);

		if (delta.x < 0) r = -r;

		if (curr_zoom_type == 0) {
			ws.setScale(ws.sheet->scale * powf(1.05f, r / 30.f));
			ws.update_grid = true;
		} else if (curr_zoom_type == 1) {
			ws.setScaleTo(ws.sheet->scale * powf(1.05f, r / 30.f), zoom_begin);
			ws.update_grid = true;
		}
	}
}

void Menu_Zoom::eventProc(const vk2d::Event& e, float dt)
{
	auto& ws = getCurrentWindowSheet();

	switch (e.type) {
	case Event::MousePressed: {
		if (e.mouseButton.button == Mouse::Left) {
			zoom_begin = ws.getCursorPos();
		} else if (e.mouseButton.button == Mouse::Right) {
			curr_zoom_type = (curr_zoom_type + 1) % 3;
		}
	} break;
	case Event::MouseReleased: {
		if (curr_zoom_type == 2) {
			auto rect = ws.getDragRect(Mouse::Left);

			if (rect.width > 5 && rect.height > 5) {
				ws.setPosition(ws.toPlane(rect.getPosition() + rect.getSize() / 2.f));
				ws.setScale(ws.sheet->scale * ws.window_rect.width / rect.width);
			}
		}
	} break;
	}
}

void Menu_Zoom::menuButton()
{
	auto& main_window = MainWindow::get();
	SideMenu::menuButtonImpl(ICON_MAG_GLASS, { 40, 40 });
}

void Menu_Zoom::upperMenu()
{
	auto& main_window  = MainWindow::get();
	auto& ws = getCurrentWindowSheet();

	if (ImGui::ImageButton(ICON_MAG_PLUS, { 35, 35 })) {
		ws.setScale(ws.sheet->scale * 1.05f);
	}
	if (ImGui::ImageButton(ICON_MAG_MINUS, { 35, 35 })) {
		ws.setScale(ws.sheet->scale / 1.05f);
	}
	ImGui::RadioImageButton(ICON_MAG_CENTER, { 35, 35 }, &curr_zoom_type, 0);
	ImGui::RadioImageButton(ICON_MAG_PIVOT, { 35, 35 }, &curr_zoom_type, 1);
	ImGui::RadioImageButton(ICON_MAG_RANGE, { 35, 35 }, &curr_zoom_type, 2);
}

SelectingSideMenu::SelectingSideMenu(const char* menu_name) :
	SideMenu(menu_name),
	is_working(false),
	blocked(false),
	start_pos(),
	last_pos(),
	dir(Direction::Up),
	last_dir(Direction::Up)
{}

void SelectingSideMenu::loop()
{
	auto& ws = getCurrentWindowSheet();

	if (is_working) {
		loopWork();
		return;
	}

	ws.clearHoverList();

	auto drag_rect = ws.getDragRect(Mouse::Left);
	if (length(drag_rect.getSize()) > DRAG_THRESHOLD) {
		AABB aabb = ws.toPlane(drag_rect);

		ws.showDragRect(Mouse::Left);

		if (ws.capturing_mouse && !Keyboard::isKeyPressed(Key::LShift)) {
			ws.sheet->bvh.query(aabb, [&](auto iter) {
				ws.addToHoverList(iter, aabb);
				BVH_CONTINUE;
			});
		}
	} else if (ws.capturing_mouse) {
		auto pos = ws.getCursorPlanePos();

		ws.sheet->bvh.query(pos, [&](auto iter) {
			ws.addToHoverList(iter, point_to_AABB(pos));
			BVH_CONTINUE;
		});
	}
}

void SelectingSideMenu::eventProc(const vk2d::Event& e, float dt)
{
	auto& ws  = getCurrentWindowSheet();

	switch (e.type) {
	case Event::KeyPressed: {
		auto tmp = (int)dir;
		switch (e.keyboard.key) {
		case Key::Up:
			dir = Direction::Up;
			break;
		case Key::Right:
			dir = Direction::Right;
			break;
		case Key::Down:
			dir = Direction::Down;
			break;
		case Key::Left:
			dir = Direction::Left;
			break;
		case Key::Delete:
			deleteSelectedElement();
			is_working = false;
			break;
		};
		last_dir = rotate_dir(dir, (Direction)tmp);
	} break;
	case Event::MousePressed: {
		if (!ws.capturing_mouse) break;

		bool ctrl = Keyboard::isKeyPressed(Key::LControl);
		bool shft = Keyboard::isKeyPressed(Key::LShift);

		if (e.mouseButton.button == Mouse::Left) {
			auto pos = ws.getCursorPlanePos();

			if (is_working) {
				if (!blocked) endWork();
			} else if (isSelectedElement(pos)) {
				ws.clearHoverList();
				beginWork();
			} else {
				if (selectElement(point_to_AABB(pos)) && !ctrl && !shft) {
					ws.clearHoverList();
					beginWork();
				}
			}
		} else if (e.mouseButton.button == Mouse::Right) {
			dir      = rotate_cw(dir);
			last_dir = Direction::Right;
		}
	} break;
	case Event::MouseReleased: {
		if (!ws.capturing_mouse) break;
		if (e.mouseButton.button != Mouse::Left || is_working) break;
		
		auto drag_rect = ws.getDragRect(Mouse::Left);

		if (length(drag_rect.getSize()) > DRAG_THRESHOLD) {
			selectElement((AABB)ws.toPlane(drag_rect));
		}
	} break;
	}
}

void SelectingSideMenu::upperMenu()
{
	if (is_working && last_dir == Direction::Up) {
		auto& main_window = MainWindow::get();

		int tmp = (int)dir;
		ImGui::SameLine();
		ImGui::RadioImageButton(ICON_DIR_UP, { 35, 35 }, &tmp, 0);
		ImGui::SameLine();
		ImGui::RadioImageButton(ICON_DIR_RIGHT, { 35, 35 }, &tmp, 1);
		ImGui::SameLine();
		ImGui::RadioImageButton(ICON_DIR_DOWN, { 35, 35 }, &tmp, 2);
		ImGui::SameLine();
		ImGui::RadioImageButton(ICON_DIR_LEFT, { 35, 35 }, &tmp, 3);

		last_dir = (Direction)(((int)tmp - (int)dir + 4) % 4);
		dir      = (Direction)tmp;
	}
}

bool SelectingSideMenu::isBusy() const
{
	return is_working;
}

void SelectingSideMenu::beginWork()
{
	auto& ws = getCurrentWindowSheet();

	is_working = true;
	start_pos  = ws.getClampedCursorPlanePos();
	last_pos   = start_pos;
	dir        = Direction::Up;
	last_dir   = Direction::Up;
}

void SelectingSideMenu::endWork()
{
	is_working = false;
}

bool SelectingSideMenu::isSelectedElement(const vec2& pos)
{
	auto& ws  = getCurrentWindowSheet();
	AABB aabb = point_to_AABB(pos);

	return ws.getBVH().query(pos, [&](auto iter) {
		auto& elem = static_cast<CircuitElement&>(*iter->second);

		auto flags = elem.getSelectFlags(aabb);
		
		return elem.getCurrSelectFlags() & flags;
	});
}

bool SelectingSideMenu::selectElement(const AABB& aabb)
{
	auto& ws  = getCurrentWindowSheet();
	bool ctrl = Keyboard::isKeyPressed(Key::LControl);
	bool shft = Keyboard::isKeyPressed(Key::LShift);

	auto cmd_group = std::make_unique<CommandGroup>();
	auto cmd0      = std::make_unique<Command_Select>();
	bool selected  = false;

	if (!ctrl && !shft) { // clear and select
		if (!ws.sheet->selections.empty()) {
			auto cmd1 = std::make_unique<Command_Select>();
			cmd1->type = Command_Select::Clear;
			cmd_group->description = cmd1->what();
			cmd_group->append(std::move(cmd1));
		}

		cmd0->type = Command_Select::SelectAppend;
		cmd0->aabb = aabb;

		ws.getBVH().query(aabb, [&](auto iter) {
			auto& elem = static_cast<CircuitElement&>(*iter->second);

			auto flags = elem.getSelectFlags(aabb);

			if (!flags) BVH_CONTINUE;

			cmd0->selections.emplace_back(elem.id, flags);
			selectConnectedWire(elem, *cmd0);

			BVH_CONTINUE;
		});

		if (!cmd0->selections.empty()) {
			selected = true;
			cmd_group->description = cmd0->what();
			cmd_group->append(std::move(cmd0));
		}
	} else if (ctrl) { // select append
		cmd0->type = Command_Select::SelectAppend;
		cmd0->aabb = aabb;

		ws.getBVH().query(aabb, [&](auto iter) {
			auto& elem = static_cast<CircuitElement&>(*iter->second);

			auto flags = elem.getSelectFlags(aabb);
			flags  &= ~elem.getCurrSelectFlags();

			if (flags)
				cmd0->selections.emplace_back(elem.id, flags);

			BVH_CONTINUE;
		});

		if (!cmd0->selections.empty()) {
			selected = true;
			cmd_group->description = cmd0->what();
			cmd_group->append(std::move(cmd0));
		}
	} else if (shft) { // invert select
		if (ws.getBVH().query(aabb, [&](auto iter) { BVH_BREAK; })) {
			cmd0->type = Command_Select::SelectInvert;
			cmd0->aabb = aabb;

			cmd_group->description = cmd0->what();
			cmd_group->append(std::move(cmd0));
		}
	}

	if (!cmd_group->empty()) {
		cmd_group->modifying = false;
		ws.pushCommand(std::move(cmd_group));
	}
	return selected;
}

void SelectingSideMenu::deleteSelectedElement()
{
	auto& ws = getCurrentWindowSheet();
	ws.pushCommand(std::make_unique<Command_Delete>());
}

bool SelectingSideMenu::checkBlocked(const AABB& aabb) const
{
	auto& ws = getCurrentWindowSheet();

	return ws.getBVH().query(aabb, [&](auto iter) {
		auto& elem = static_cast<CircuitElement&>(*iter->second);

		return !elem.isWireBased() && !(elem.style & (CircuitElement::Selected | CircuitElement::Cut));
	});
}

Menu_Select::Menu_Select() :
	SelectingSideMenu("Select")
{}

void Menu_Select::loop()
{
	SelectingSideMenu::loop();
}

void Menu_Select::menuButton()
{
	auto& main_window = MainWindow::get();
	SideMenu::menuButtonImpl(ICON_ARROW_POINTER, { 40, 40 });
}

void Menu_Select::onClose()
{
	if (!is_working) return;

	auto& ws = getCurrentWindowSheet();

	for (auto& iter : ws.sheet->selections) {
		auto& elem = static_cast<CircuitElement&>(*iter->second);

		if (!elem.isWireBased())
			elem.style &= ~CircuitElement::Blocked;

		elem.transform({}, last_pos, invert_dir(dir));
		elem.transform(start_pos - last_pos, {}, Direction::Up);
	}

	SelectingSideMenu::endWork();
}

void Menu_Select::beginWork()
{
	SelectingSideMenu::beginWork();
}

void Menu_Select::endWork()
{
	auto& ws = getCurrentWindowSheet();

	auto delta = last_pos - start_pos;

	if (delta != vec2(0.f) || dir != Direction::Up) {
		auto cmd = std::make_unique<Command_Move>();

		cmd->delta  = delta;
		cmd->origin = last_pos;
		cmd->dir    = dir;

		for (auto iter : ws.sheet->selections)
			ws.getBVH().update_element(iter, iter->second->getAABB());

		ws.pushCommand(std::move(cmd), true);
	}

	SelectingSideMenu::endWork();
}

void Menu_Select::loopWork()
{
	auto& ws = getCurrentWindowSheet();
	auto pos = ws.getClampedCursorPlanePos();

	blocked = false;
	for (auto& iter : ws.sheet->selections) {
		auto& elem = static_cast<CircuitElement&>(*iter->second);

		if (!elem.isWireBased()) {
			if (checkBlocked(elem.getAABB())) {
				elem.style |= CircuitElement::Blocked;
				blocked = true;
			} else
				elem.style &= ~CircuitElement::Blocked;
		}

		elem.transform(pos - last_pos, pos, last_dir);
	}

	last_pos = pos;
	last_dir = Direction::Up;
}

Menu_Library::Menu_Library() :
	SideMenu("Library"),
	curr_gate(-1),
	curr_dir(Direction::Up)
{}

void Menu_Library::loop()
{
	if (curr_gate == -1) return;

	auto& main_window  = MainWindow::get();
	auto& ws = getCurrentWindowSheet();

	auto pos = ws.getCursorPos();

	vk2d::Cursor::setVisible(!ws.capturing_mouse);

	pos = ws.getClampedCursorPos();
	if (ws.capturing_mouse) {
		pos           = ws.toPlane(pos);
		auto& bvh     = ws.sheet->bvh;
		auto& gate    = main_window.logic_gates[curr_gate];

		gate.pos = pos;
		gate.dir = curr_dir;

		auto overlap = bvh.query(gate.getAABB(), [&](auto iter) {
			return iter->second->getType() != CircuitElement::Wire;
		});

		if (overlap)
			gate.style |= CircuitElement::Blocked;
		else
			gate.style &= ~CircuitElement::Blocked;

		gate.draw(ws.draw_list);
	}
}

void Menu_Library::eventProc(const vk2d::Event& e, float dt)
{
	switch (e.type) {
	case Event::MousePressed: {
		auto& main_window  = MainWindow::get();
		auto& ws = getCurrentWindowSheet();

		if (e.mouseButton.button == Mouse::Left && ws.capturing_mouse) {
			if (curr_gate == -1) return;

			auto& gate = main_window.logic_gates[curr_gate];

			if (gate.style & CircuitElement::Style::Blocked) return;

			auto cmd = std::make_unique<Command_Add>();
			cmd->elements.emplace_back(gate.clone());

			ws.pushCommand(std::move(cmd));
		} else if (e.mouseButton.button == Mouse::Right) {
			curr_dir = rotate_cw(curr_dir);
		}
	}	break;
	}
}

void Menu_Library::menuButton()
{
	auto& main_window = MainWindow::get();
	SideMenu::menuButtonImpl(ICON_LIBRARY, { 40, 40 });
}

void Menu_Library::upperMenu()
{
	auto& main_window = MainWindow::get();

	if (ImGui::ImageButton(ICON_BOOK, { 35, 35 })) {
		main_window.window_library.show = true;
	}

	ImGui::SameLine();
	if (ImGui::ImageButton(ICON_ROTATE_CW, { 35, 35 })) {
		curr_dir = rotate_cw(curr_dir);
	}

	ImGui::SameLine();
	if (ImGui::ImageButton(ICON_ROTATE_CCW, { 35, 35 })) {
		curr_dir = rotate_ccw(curr_dir);
	}

	ImGui::SameLine();
	ImGui::RadioImageButton(ICON_DIR_UP, { 35, 35 }, (int*)&curr_dir, 0);
	ImGui::SameLine();
	ImGui::RadioImageButton(ICON_DIR_RIGHT, { 35, 35 }, (int*)&curr_dir, 1);
	ImGui::SameLine();
	ImGui::RadioImageButton(ICON_DIR_DOWN, { 35, 35 }, (int*)&curr_dir, 2);
	ImGui::SameLine();
	ImGui::RadioImageButton(ICON_DIR_LEFT, { 35, 35 }, (int*)&curr_dir, 3);
}

Menu_Unit::Menu_Unit() :
	SideMenu("Unit")
{
}

void Menu_Unit::loop()
{
}

void Menu_Unit::menuButton()
{
	auto& main_window = MainWindow::get();
	SideMenu::menuButtonImpl(ICON_MICROCHIP, { 40, 40 });
}

Menu_Shapes::Menu_Shapes() :
	SideMenu("Shapes")
{
}

void Menu_Shapes::loop()
{
}

void Menu_Shapes::menuButton()
{
	auto& main_window = MainWindow::get();
	SideMenu::menuButtonImpl(ICON_SHAPES, { 40, 40 });
}

Menu_Label::Menu_Label() :
	SideMenu("Label")
{
}

void Menu_Label::loop()
{
}

void Menu_Label::menuButton()
{
	auto& main_window = MainWindow::get();
	SideMenu::menuButtonImpl(ICON_LABEL, { 40, 40 });
}

Menu_Copy::Menu_Copy() :
	SelectingSideMenu("Copy"),
	from_clipboard(false),
	prev_menu(nullptr)
{}

void Menu_Copy::loop()
{
	SelectingSideMenu::loop();
}

void Menu_Copy::menuButton()
{
	auto& main_window = MainWindow::get();
	SideMenu::menuButtonImpl(ICON_COPY, { 40, 40 });
}

void Menu_Copy::onBegin()
{
	auto& ws = getCurrentWindowSheet();

	if (!ws.sheet->selections.empty())
		beginWork();
}

void Menu_Copy::onClose()
{
	if (!is_working) return;

	auto& ws = getCurrentWindowSheet();

	for (auto iter : ws.sheet->selections) {
		auto& elem = static_cast<CircuitElement&>(*iter->second);

		elem.style |= CircuitElement::Selected;
	}

	elements.clear();

	SelectingSideMenu::endWork();
}

void Menu_Copy::beginWork()
{
	auto& ws = getCurrentWindowSheet();

	SelectingSideMenu::beginWork();

	ws.copySelectedToClipboard();

	beginPaste(false);
}

void Menu_Copy::endWork()
{
	auto& ws = getCurrentWindowSheet();

	for (auto iter : ws.sheet->selections) {
		auto& elem = static_cast<CircuitElement&>(*iter->second);

		elem.style |= CircuitElement::Selected;
	}

	if (from_clipboard) {
		auto cmd_group = std::make_unique<CommandGroup>();
		auto cmd       = std::make_unique<Command_Add>();

		for (auto& elem : elements)
			elem->unselect();

		cmd->elements.swap(elements);
		cmd->elements.shrink_to_fit();

		cmd_group->description = "Paste item(s) from Clipboard";
		cmd_group->append(std::move(cmd));

		ws.pushCommand(std::move(cmd_group));
	} else {
		auto delta = last_pos - start_pos;

		if (delta != vec2(0.f) || dir != Direction::Up) {
			auto cmd = std::make_unique<Command_Copy>();

			cmd->delta  = delta;
			cmd->origin = last_pos;
			cmd->dir    = dir;

			ws.pushCommand(std::move(cmd));
		}
	}

	elements.clear();

	SelectingSideMenu::endWork();

	if (from_clipboard && prev_menu) {
		auto& main_window = MainWindow::get();
		main_window.setCurrentSideMenu(prev_menu);
	}
}

void Menu_Copy::loopWork()
{
	auto& ws = getCurrentWindowSheet();
	auto pos = ws.getClampedCursorPlanePos();

	blocked = false;
	for (auto& elem : elements) {
		if (!elem->isWireBased()) {
			if (checkBlocked(elem->getAABB())) {
				elem->style |= CircuitElement::Blocked;
				blocked = true;
			} else
				elem->style &= ~CircuitElement::Blocked;
		}

		elem->transform(pos - last_pos, pos, last_dir);
		elem->draw(ws.draw_list);
	}

	last_pos = pos;
	last_dir = Direction::Up;
}

void Menu_Copy::beginPaste(bool from_clipboard)
{
	auto& ws = getCurrentWindowSheet();

	this->from_clipboard = from_clipboard;

	if (from_clipboard) {
		for (auto iter : ws.sheet->selections) {
			auto& elem = static_cast<CircuitElement&>(*iter->second);

			elem.style &= ~CircuitElement::Selected;
		}
	} else {
		for (auto iter : ws.sheet->selections) {
			auto& elem = static_cast<CircuitElement&>(*iter->second);

			elements.emplace_back(elem.clone())->select();

			elem.style &= ~CircuitElement::Selected;
		}
	}
}

void Menu_Copy::beginClipboardPaste(std::istream& is)
{
	auto& main_window = MainWindow::get();
	auto& ws          = getCurrentWindowSheet();

	SelectingSideMenu::beginWork();

	beginPaste(true);

	prev_menu = main_window.curr_menu;
	main_window.setCurrentSideMenu(this);

	size_t element_count;
	vec2   delta;
	read_binary(is, element_count);
	read_binary(is, delta);

	delta = start_pos - delta;

	for (size_t i = 0; i < element_count; ++i) {
		auto& new_elem = elements.emplace_back(CircuitElement::create(is));

		new_elem->select();
		new_elem->transform(delta, {}, Direction::Up);
	}
}

Menu_Cut::Menu_Cut() :
	SelectingSideMenu("Cut"),
	from_clipboard(false),
	prev_menu(nullptr)
{}

void Menu_Cut::loop()
{
	SelectingSideMenu::loop();
}

void Menu_Cut::menuButton()
{
	auto& main_window = MainWindow::get();
	SideMenu::menuButtonImpl(ICON_CUT, { 40, 40 });
}

void Menu_Cut::onBegin()
{
	auto& ws = getCurrentWindowSheet();

	if (!ws.sheet->selections.empty())
		beginWork();
}

void Menu_Cut::onClose()
{
	if (!is_working) return;

	auto& ws = getCurrentWindowSheet();

	for (auto iter : ws.sheet->selections) {
		auto& elem = static_cast<CircuitElement&>(*iter->second);

		elem.style |= CircuitElement::Selected;
		elem.style &= ~CircuitElement::Cut;
	}

	elements.clear();

	SelectingSideMenu::endWork();
}

void Menu_Cut::beginWork()
{
	auto& ws = getCurrentWindowSheet();
	
	SelectingSideMenu::beginWork();

	ws.copySelectedToClipboard();

	beginPaste(false);
}

void Menu_Cut::endWork()
{
	auto& ws = getCurrentWindowSheet();

	for (auto iter : ws.sheet->selections) {
		auto& elem = static_cast<CircuitElement&>(*iter->second);

		elem.style |= CircuitElement::Selected;
		elem.style &= ~CircuitElement::Cut;
	}


	if (from_clipboard) {
		auto cmd_group = std::make_unique<CommandGroup>();
		auto cmd = std::make_unique<Command_Add>();

		for (auto& elem : elements)
			elem->unselect();

		cmd->elements.swap(elements);
		cmd->elements.shrink_to_fit();
		
		cmd_group->description = "Paste item(s) from Clipboard";
		cmd_group->append(std::move(cmd));
		
		ws.pushCommand(std::move(cmd_group));
	} else {
		auto cmd = std::make_unique<Command_Cut>();

		cmd->delta  = last_pos - start_pos;
		cmd->origin = last_pos;
		cmd->dir    = dir;

		ws.pushCommand(std::move(cmd));
	}

	elements.clear();

	SelectingSideMenu::endWork();

	if (from_clipboard && prev_menu) {
		auto& main_window = MainWindow::get();
		main_window.setCurrentSideMenu(prev_menu);
	}
}

void Menu_Cut::loopWork()
{
	auto& ws = getCurrentWindowSheet();
	auto pos = ws.getClampedCursorPlanePos();

	blocked = false;
	for (auto& elem : elements) {
		if (!elem->isWireBased()) {
			if (checkBlocked(elem->getAABB())) {
				elem->style |= CircuitElement::Blocked;
				blocked = true;
			} else
				elem->style &= ~CircuitElement::Blocked;
		}

		elem->transform(pos - last_pos, pos, last_dir);
		elem->draw(ws.draw_list);
	}

	last_pos = pos;
	last_dir = Direction::Up;
}

void Menu_Cut::beginPaste(bool from_clipboard) 
{
	auto& ws = getCurrentWindowSheet();

	this->from_clipboard = from_clipboard;

	if (from_clipboard) {
		for (auto iter : ws.sheet->selections) {
			auto& elem = static_cast<CircuitElement&>(*iter->second);

			elem.style &= ~CircuitElement::Selected;
		}
	} else {
		for (auto iter : ws.sheet->selections) {
			auto& elem = static_cast<CircuitElement&>(*iter->second);

			auto& new_elem = elements.emplace_back(elem.clone());

			elem.style &= ~CircuitElement::Selected;
			elem.style |= CircuitElement::Cut;

			new_elem->select();
		}
	}
}

void Menu_Cut::beginClipboardPaste(std::istream& is)
{
	auto& main_window = MainWindow::get();
	auto& ws          = getCurrentWindowSheet();

	SelectingSideMenu::beginWork();

	beginPaste(true);

	prev_menu = main_window.curr_menu;
	main_window.setCurrentSideMenu(this);

	size_t element_count;
	vec2   delta;
	read_binary(is, element_count);
	read_binary(is, delta);

	delta = start_pos - delta;

	for (size_t i = 0; i < element_count; ++i) {
		auto& new_elem = elements.emplace_back(CircuitElement::create(is));

		new_elem->select();
		new_elem->transform(delta, {}, Direction::Up);
	}
}

Menu_Delete::Menu_Delete() :
	SideMenu("Delete")
{
}

void Menu_Delete::loop()
{
	auto& ws       = getCurrentWindowSheet();
	auto drag_rect = ws.getDragRect(Mouse::Left);

	if (length(drag_rect.getSize()) > DRAG_THRESHOLD) {
		AABB aabb = ws.toPlane(drag_rect);

		ws.showDragRect(Mouse::Left);

		ws.sheet->bvh.query(aabb, [&](auto iter) {
			ws.addToHoverList(iter, aabb);
			BVH_CONTINUE;
		});
	} else {
		auto pos = ws.getCursorPlanePos();

		ws.sheet->bvh.query(pos, [&](auto iter) {
			ws.addToHoverList(iter, point_to_AABB(pos));
			BVH_CONTINUE;
		});
	}
}

void Menu_Delete::eventProc(const vk2d::Event& e, float dt)
{
	auto& ws = getCurrentWindowSheet();

	switch (e.type) {
	case Event::MousePressed: {
		if (e.mouseButton.button != Mouse::Left) break;

		ws.clearHoverList();

		auto pos = ws.getCursorPlanePos();
		ws.deleteElement(point_to_AABB(pos));
	} break;
	case Event::MouseReleased: {
		if (e.mouseButton.button != Mouse::Left) break;

		auto drag_rect = ws.getDragRect(Mouse::Left);

		if (length(drag_rect.getSize()) > DRAG_THRESHOLD) {
			ws.clearHoverList();
			
			AABB aabb = ws.toPlane(drag_rect);
			ws.deleteElement(aabb);
		}
	} break;
	}
}

void Menu_Delete::menuButton()
{
	auto& main_window = MainWindow::get();
	SideMenu::menuButtonImpl(ICON_TRASH_CAN, { 40, 40 });
}

void Menu_Delete::onBegin()
{
	auto& ws = getCurrentWindowSheet();

	if (ws.sheet->selections.empty()) return;

	auto cmd = std::make_unique<Command_Delete>();
	ws.pushCommand(std::move(cmd));
}

WiringSideMenu::WiringSideMenu(const char* menu_name) :
	SideMenu(menu_name),
	curr_wire_type(0),
	is_wiring(false),
	start_pos()
{}

vec2 WiringSideMenu::getMiddle(const vec2& p0, const vec2& p1) const
{
	if (curr_wire_type == 0) {
		return { p1.x, p0.y };
	} else if (curr_wire_type == 4) {
		return { p0.x, p1.y };
	} else if (curr_wire_type != 2) {
		auto dx = abs(p1.x - p0.x);
		auto dy = abs(p1.y - p0.y);

		if (curr_wire_type == 1) {
			if (dx < dy)
				return { p0.x, p1.y + (p0.y < p1.y ? -dx : dx) };
			else
				return { p1.x + (p0.x < p1.x ? -dy : dy), p0.y };
		} else {
			if (dx < dy)
				return { p1.x, p0.y + (p0.y < p1.y ? dx : -dx) };
			else
				return { p0.x + (p0.x < p1.x ? dy : -dy), p1.y };
		}
	}
	
	return (p0 + p1) / 2.f;
}

bool WiringSideMenu::checkContinueWiring(const vec2& pos) const
{
	auto& ws = getCurrentWindowSheet();

	auto result = ws.sheet->bvh.query(pos, [&](auto iter) {
		auto& [aabb, elem] = *iter;

		switch (elem->getType()) {
		case CircuitElement::Wire:
			BVH_BREAK;
		case CircuitElement::LogicGate:
		case CircuitElement::LogicUnit:
			if (auto* pin = elem->getPin(pos))
				BVH_BREAK;
			break;
		}

		BVH_CONTINUE;
	});

	return !result;
}

Menu_Wire::Menu_Wire() :
	WiringSideMenu("Wire")
{}

void Menu_Wire::loop()
{
	static const vk2d::Color color(50, 177, 108);

	auto& ws  = getCurrentWindowSheet();
	auto pos  = ws.getClampedCursorPlanePos();
	auto& cmd = ws.draw_list.commands.back();

	cmd.addFilledCircle(pos, 3.f / DEFAULT_GRID_SIZE, color);

	if (is_wiring) {
		auto mid = getMiddle(start_pos, pos);

		Wire wire0(start_pos, mid);
		Wire wire1(mid, pos);
		wire0.select();
		wire1.select();

		cmd.addFilledCircle(start_pos, 3.f / DEFAULT_GRID_SIZE, color);

		wire0.draw(ws.draw_list);
		wire1.draw(ws.draw_list);
	}
}

void Menu_Wire::eventProc(const vk2d::Event& e, float dt)
{
	switch (e.type) {
	case Event::MousePressed: {
		auto& ws = getCurrentWindowSheet();

		if (e.mouseButton.button == Mouse::Left && ws.capturing_mouse) {
			auto pos = ws.getClampedCursorPlanePos();

			if (!is_wiring) {
				is_wiring = true;
				start_pos = pos;
				return;
			} else if (pos != start_pos) {
				auto is_vert  = is_vertical(start_pos, pos);
				auto is_horiz = is_horizontal(start_pos, pos);

				std::vector<Wire> wires;
				if (curr_wire_type == 2 || is_vert || is_horiz) {
					if (length(start_pos - pos) > 0.f)
						wires.emplace_back(start_pos, pos);
				} else {
					auto mid = getMiddle(start_pos, pos);

					if (length(start_pos - mid) > 0.f)
						wires.emplace_back(start_pos, mid);
					if (length(mid - pos) > 0.f)
						wires.emplace_back(mid, pos);
				}

				bool wiring = checkContinueWiring(pos);
				addWires(std::move(wires));

				if (wiring) {
					start_pos = pos;
					return;
				}
			}
			
			is_wiring = false;
		} else if (e.mouseButton.button == Mouse::Right)
			curr_wire_type = (curr_wire_type + 1) % 5;
	}break;
	}
}

void Menu_Wire::menuButton()
{
	auto& main_window = MainWindow::get();
	SideMenu::menuButtonImpl(ICON_WIRE, { 40, 40 });
}

void Menu_Wire::upperMenu()
{
	auto& main_window = MainWindow::get();

	ImGui::RadioImageButton(ICON_WIRE_TYPE0, { 35, 35 }, &curr_wire_type, 0);
	ImGui::SameLine();
	ImGui::RadioImageButton(ICON_WIRE_TYPE1, { 35, 35 }, &curr_wire_type, 1);
	ImGui::SameLine();
	ImGui::RadioImageButton(ICON_WIRE_TYPE2, { 35, 35 }, &curr_wire_type, 2);
	ImGui::SameLine();
	ImGui::RadioImageButton(ICON_WIRE_TYPE3, { 35, 35 }, &curr_wire_type, 3);
	ImGui::SameLine();
	ImGui::RadioImageButton(ICON_WIRE_TYPE4, { 35, 35 }, &curr_wire_type, 4);
}

void Menu_Wire::addWires(std::vector<Wire>&& stack)
{
	auto& ws = getCurrentWindowSheet();
	auto cmd = std::make_unique<Command_Add>();

	while (!stack.empty()) {
		auto wire = stack.back(); stack.pop_back();

		auto result = ws.sheet->bvh.query(wire.getAABB(), [&](auto iter) {
			CircuitElement& elem = *iter->second;

			if (elem.getType() != CircuitElement::Wire) BVH_CONTINUE;

			float t0, t1;
			auto& item = static_cast<Wire&>(elem);

			if (item.overlap(wire, t0, t1)) {
				if (t0 == t1) BVH_CONTINUE;

				if (t0 == 0.f && t1 != 1.f) {
					wire.p0 = lerp(wire.p0, wire.p1, t1);
					stack.push_back(wire);
				} else if (t0 != 0.f && t1 == 1.f) {
					wire.p1 = lerp(wire.p0, wire.p1, t0);
					stack.push_back(wire);
				} else if (t0 != 0.f && t1 != 1.f) {
					stack.emplace_back(wire.p0, lerp(wire.p0, wire.p1, t0));
					stack.emplace_back(lerp(wire.p0, wire.p1, t1), wire.p1);
				}
				BVH_BREAK;
			}

			BVH_CONTINUE;
		});

		if (!result) {
			wire.dot0 = checkWireCrossing(wire.p0);
			wire.dot1 = checkWireCrossing(wire.p1);
			cmd->elements.emplace_back(wire.clone());
		}
	}

	ws.pushCommand(std::move(cmd));
}

bool Menu_Wire::checkWireCrossing(const vec2& pos) const
{
	auto& ws  = getCurrentWindowSheet();
	int count = 0;

	ws.sheet->bvh.query(pos, [&](auto iter) {
		CircuitElement& elem = *iter->second;

		if (elem.getType() != CircuitElement::Wire) BVH_CONTINUE;

		auto& wire = static_cast<Wire&>(elem);

		if (wire.p0 == pos || wire.p1 == pos) 
			++count;
		else if (wire.hit(pos))
			count += 2;

		return count > 1;
	});

	return count > 1;
}

Menu_Net::Menu_Net() :
	WiringSideMenu("Net")
{
}

void Menu_Net::loop()
{
}

void Menu_Net::eventProc(const vk2d::Event& e, float dt)
{
}

void Menu_Net::menuButton()
{
	auto& main_window = MainWindow::get();
	SideMenu::menuButtonImpl(ICON_NET, { 40, 40 });
}

void Menu_Net::upperMenu()
{
	auto& main_window = MainWindow::get();

	ImGui::RadioImageButton(ICON_NET_TYPE0, { 35, 35 }, &curr_wire_type, 0);
	ImGui::SameLine();
	ImGui::RadioImageButton(ICON_NET_TYPE1, { 35, 35 }, &curr_wire_type, 1);
	ImGui::SameLine();
	ImGui::RadioImageButton(ICON_NET_TYPE2, { 35, 35 }, &curr_wire_type, 2);
	ImGui::SameLine();
	ImGui::RadioImageButton(ICON_NET_TYPE3, { 35, 35 }, &curr_wire_type, 3);
	ImGui::SameLine();
	ImGui::RadioImageButton(ICON_NET_TYPE4, { 35, 35 }, &curr_wire_type, 4);
}

Menu_SplitWire::Menu_SplitWire() :
	SideMenu("Split Wire")
{
}

void Menu_SplitWire::loop()
{
}

void Menu_SplitWire::eventProc(const vk2d::Event& e, float dt)
{
}

void Menu_SplitWire::menuButton()
{
	auto& main_window = MainWindow::get();
	SideMenu::menuButtonImpl(ICON_SPLIT_WIRE, { 40, 40 });
}