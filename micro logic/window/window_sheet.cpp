#include "window_sheet.h"

#include <vk2d/system/clipboard.h>
#include <imgui_internal.h>
#include <sstream>
#include <fstream>
#include "../main_window.h"
#include "../base64.h"

static inline ImRect to_ImRect(const vk2d::Rect& rect)
{
	return { to_ImVec2(rect.getPosition()), to_ImVec2(rect.getSize()) };
}

Window_Sheet::Window_Sheet() :
	sheet(nullptr),
	curr_command(-1),
	last_saved_command(-2),
	prev_position(0.f),
	prev_scale(1.f),
	capturing_mouse(false),
	update_grid(true),
	close_window(false),
	file_saved(false)
{}

Window_Sheet::Window_Sheet(SchematicSheet& sheet) :
	Window_Sheet()
{
	bindSchematicSheet(sheet);
}

Window_Sheet::~Window_Sheet()
{
}

void Window_Sheet::showUI()
{
	auto& main_window = MainWindow::get();

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoBackground;
	if (!file_saved) flags |= ImGuiWindowFlags_UnsavedDocument;

	DockingWindow::beginDockWindow(name.c_str(), nullptr, flags);

	ImGui::InvisibleButton("##test", { to_ImVec2(content_rect.getSize()) });
	capturing_mouse = ImGui::IsItemHovered();

	if (visible) {
		if (prev_position != sheet->position || prev_scale != sheet->scale) {
			prev_position = sheet->position;
			prev_scale    = sheet->scale;
			update_grid   = true;
		}

		content_center = content_rect.center();

		if (focussed)
			main_window.setCurrentWindowSheet(*this);
		if (main_window.curr_menu == 3 && hovered) {
			if (dynamic_cast<Menu_Library&>(*main_window.side_menus[3]).curr_gate != -1) {
				ImGui::SetWindowFocus();
				main_window.setCurrentWindowSheet(*this);
			}
		}
	}

	DockingWindow::endDockWindow();

	if (!window || !visible) return;

	if (main_window.settings.view.grid)
		showGrid();
	if (main_window.settings.debug.show_bvh)
		showBVH();

	auto transform = vk2d::Transform().
		translate(content_center - sheet->position * sheet->grid_pixel_size).
		scale(sheet->grid_pixel_size);

	for (auto& cmd : draw_list.commands) {
		cmd.options.transform = transform;
		cmd.options.scissor   = window_rect;
	}

	for (const auto& elem : sheet->bvh)
		elem.second->draw(draw_list);
}

void Window_Sheet::EventProc(const vk2d::Event& e, float dt)
{
	using vk2d::Event;
	using vk2d::Mouse;
	using vk2d::Key;

	if (!focussed) return;

	auto& main_window = MainWindow::get();

	switch (e.type) {
	case Event::KeyPressed: {
		switch (e.keyboard.key) {
		case Key::X:
			if (e.keyboard.ctrl)
				cutSelectedToClipboard();
			break;
		case Key::C:
			if (e.keyboard.ctrl)
				copySelectedToClipboard();
			break;
		case Key::V:
			if (e.keyboard.ctrl)
				pasteFromClipboard();
			break;
		case Key::Z:
			if (e.keyboard.ctrl && isUndoable())
				undo();
			break;
		case Key::Y:
			if (e.keyboard.ctrl && isRedoable())
				redo();
			break;
		}
	} break;
	case Event::MouseMoved: {
		if (main_window.curr_menu == -1 && Mouse::isPressed(Mouse::Left)) {
			auto delta       = e.mouseMove.delta;
			sheet->position -= (vec2)delta / sheet->grid_pixel_size;
		}
	} break;
	case Event::WheelScrolled: {
		if (!window_rect.contain(e.wheel.pos)) return;

		auto new_scale = sheet->scale * (e.wheel.delta.y < 0 ? 1.05f : 1 / 1.05f);
		setScaleTo(new_scale, (vec2)e.wheel.pos);
	} break;
	}
}

void Window_Sheet::saveSheet(const std::string& dir)
{
	std::ofstream of(dir + "\\" + sheet->name + ".mls");
	sheet->serialize(of);
	
	file_saved         = true;
	last_saved_command = curr_command;
}

void Window_Sheet::bindSchematicSheet(SchematicSheet& sheet)
{
	auto& main_window = MainWindow::get();

	command_stack.clear();
	curr_command       = -1;
	last_saved_command = -2;

	this->sheet   = &sheet;
	prev_position = sheet.position;
	prev_scale    = sheet.scale;
	update_grid   = true;

	draw_list.clear();
	draw_list.resize(2);
	for (auto& texture : main_window.textures) {
		auto& cmd = draw_list.commands.emplace_back();

		cmd.options.texture = &texture;
	}
	draw_list.commands.emplace_back();
}

void Window_Sheet::copySelectedToClipboard(bool is_copy)
{
	if (sheet->selections.empty()) return;

	std::stringstream ss;
	
	if (is_copy)
		ss << CLIPBOARD_COPY_IDENTIFICATION;
	else
		ss << CLIPBOARD_CUT_IDENTIFICATION;

	write_binary(ss, sheet->selections.size());

	AABB aabb = sheet->selections.front()->second->getAABB();
	for (auto iter = sheet->selections.begin() + 1; iter != sheet->selections.end(); ++iter)
		(*iter)->second->getAABB().union_of(aabb);

	write_binary(ss, aabb.center());

	for (auto iter : sheet->selections) {
		auto& elem = static_cast<CircuitElement&>(*iter->second);

		elem.serialize(ss);
	}

	vk2d::Clipboard::setString(Base64::encode(ss.str()));
}

void Window_Sheet::cutSelectedToClipboard()
{
	if (sheet->selections.empty()) return;

	copySelectedToClipboard(false);

	auto item_count = sheet->selections.size();

	auto cmd = std::make_unique<CommandGroup>();

	cmd->description = "Cut " + std::to_string(item_count) + " Item(s)";
	cmd->append(std::make_unique<Command_Delete>());
	
	pushCommand(std::move(cmd));
}

void Window_Sheet::pasteFromClipboard()
{
	auto& main_window = MainWindow::get();

	main_window.beginClipboardPaste();
}

void Window_Sheet::addToHoverList(bvh_iterator_t iter, const AABB& aabb)
{
	auto& elem = static_cast<CircuitElement&>(*iter->second);

	if (elem.isSelected()) return;

	elem.setHover(elem.getSelectFlags(aabb));

	hover_list.emplace_back(iter);
}

void Window_Sheet::clearHoverList()
{
	for (auto& iter : hover_list)
		iter->second->clearHover();

	hover_list.clear();
}

void Window_Sheet::pushCommand(std::unique_ptr<Command>&& command, bool skip_redo)
{
	while (curr_command != command_stack.size() - 1)
		command_stack.pop_back();

	command->onPush(*sheet);
	if (!skip_redo) command->redo(*sheet);
	command_stack.push_back(std::move(command));
	
	curr_command++;
	file_saved = false;
}

void Window_Sheet::redo()
{
	if (curr_command == command_stack.size() - 1) return;
	command_stack[++curr_command]->redo(*sheet);

	if (last_saved_command == curr_command)
		file_saved = true;
	else
		file_saved = false;
}

void Window_Sheet::undo()
{
	if (curr_command == -1) return;
	command_stack[curr_command--]->undo(*sheet);

	if (last_saved_command == curr_command) 
		file_saved = true;
	else 
		file_saved = false;
}

bool Window_Sheet::isRedoable() const
{
	return curr_command != command_stack.size() - 1;
}

bool Window_Sheet::isUndoable() const
{
	return curr_command != -1;
}

void Window_Sheet::clearCommand()
{
	command_stack.clear();
	curr_command = -1;
}

void Window_Sheet::deleteElement(const AABB& aabb)
{
	auto cmd0  = std::make_unique<Command_Select>();

	cmd0->type = Command_Select::SelectAppend;
	cmd0->aabb = aabb;

	getBVH().query(aabb, [&](auto iter) {
		auto& elem = static_cast<CircuitElement&>(*iter->second);

		cmd0->selections.emplace_back(elem.id, UINT_MAX);
		return false;
	});

	if (cmd0->selections.empty()) return;

	auto cmd_group = std::make_unique<CommandGroup>();
	auto cmd1      = std::make_unique<Command_Delete>();

	cmd_group->description = cmd1->what();
	cmd_group->append(std::move(cmd0));
	cmd_group->append(std::move(cmd1));

	pushCommand(std::move(cmd_group));
}

void Window_Sheet::draw()
{
	if (!window || !visible) return;
	window->draw(draw_list);

	for (size_t i = 1; i < draw_list.commands.size(); ++i)
		draw_list[i].clear(false);
}

void Window_Sheet::showGrid()
{
	if (window_updated || update_grid) {
		auto& settings = MainWindow::get().settings;
		auto& cmd = draw_list[0];

		auto plane_rect = toPlane(window_rect);

		float min_x = floor(plane_rect.left);
		float min_y = floor(plane_rect.top);
		float max_x = ceil(plane_rect.left + plane_rect.width);
		float max_y = ceil(plane_rect.top + plane_rect.height);	

		cmd.clear();

		if (settings.view.grid_axis) {
			float width = settings.view.grid_axis_width / sheet->grid_pixel_size / 2;
			auto color  = settings.view.grid_axis_color;

			cmd.addLine(vec2(min_x, 0.f), vec2(max_x, 0.f), width, color);
			cmd.addLine(vec2(0.f, min_y), vec2(0.f, max_y), width, color);
		}

		if (settings.view.grid_style == GridStyle::dot) {
		} else {
			float width = settings.view.grid_width / sheet->grid_pixel_size / 2;
			float delta = 1.f;
			auto color  = settings.view.grid_color;

			for (float y = min_y; y <= max_y; y += delta)
				cmd.addLine(vec2(min_x, y), vec2(max_x, y), width, color);

			for (float x = min_x; x < max_x; x += delta)
				cmd.addLine(vec2(x, min_y), vec2(x, max_y), width, color);

			/*auto rect = toPlane(content_rect);
			auto pos  = rect.getPosition();
			auto size = rect.getSize();

			vertices.push_back({ vec2(pos.x, pos.y), vk2d::Colors::Red });
			vertices.push_back({ vec2(pos.x + size.x, pos.y), vk2d::Colors::Green });
			vertices.push_back({ vec2(pos.x + size.x, pos.y + size.y), vk2d::Colors::Blue });
			vertices.push_back({ vec2(pos.x, pos.y), vk2d::Colors::Red });
			vertices.push_back({ vec2(pos.x, pos.y + size.y), vk2d::Colors::Green });
			vertices.push_back({ vec2(pos.x + size.x, pos.y + size.y), vk2d::Colors::Blue });*/
		}

		update_grid = false;
	}
}

void Window_Sheet::showBVH()
{
	static const vk2d::Color colors[] = {
		vk2d::Color(255, 0, 0),
		vk2d::Color(255, 128, 0),
		vk2d::Color(255, 255, 0),
		vk2d::Color(128, 255, 0),
		vk2d::Color(0, 255, 0),
		vk2d::Color(0, 255, 128),
		vk2d::Color(0, 255, 255),
		vk2d::Color(0, 128, 255),
		vk2d::Color(0, 0, 255),
		vk2d::Color(128, 0, 255),
		vk2d::Color(255, 0, 255),
		vk2d::Color(255, 0, 128)
	};

	sheet->bvh.traverse([&](const AABB& aabb, uint32_t level) {
		auto& cmd = draw_list.commands.back();

		Rect rect  = aabb;
		auto color = colors[std::min(level, 11u)];
		
		cmd.addRect(rect.getPosition(), rect.getSize(), 1 / DEFAULT_GRID_SIZE, color);
		return true;
	});
}

void Window_Sheet::showDragRect(vk2d::Mouse::Button button)
{
	if (!ImGui::IsMouseDragging(button)) return;

	ImVec2 p_min, p_max;

	auto* draw_list = ImGui::GetForegroundDrawList(viewport);
	auto rect       = getDragRect(button);

	p_min.x = viewport->Pos.x + rect.left;
	p_min.y = viewport->Pos.y + rect.top;
	p_max.x = viewport->Pos.x + rect.left + rect.width;
	p_max.y = viewport->Pos.y + rect.top + rect.height;

	draw_list->AddRectFilled(p_min, p_max, ImColor(0.8f, 0.8f, 0.8f, 0.1f));
	draw_list->AddRect(p_min, p_max, ImColor(0.7f, 0.7f, 0.7f), 0, 0, 3.f);
}

void Window_Sheet::setPosition(const vec2& pos)
{
	sheet->position = pos;
}

void Window_Sheet::setScale(float scale)
{
	sheet->setScale(scale);
}

void Window_Sheet::setScaleTo(float scale, vec2 center)
{
	scale = std::clamp(scale, MIN_SCALE, MAX_SCALE);

	auto pos               = toPlane(center);
	sheet->scale           = scale;
	sheet->grid_pixel_size = DEFAULT_GRID_SIZE * scale;
	sheet->position       += pos - toPlane(center);
}


vec2 Window_Sheet::toScreen(const vec2& pos) const
{
	return (pos - sheet->position) * sheet->grid_pixel_size + content_center;
}

vec2 Window_Sheet::toPlane(const vec2& pos) const
{
	return (pos - content_center) / sheet->grid_pixel_size + sheet->position;
}

Rect Window_Sheet::toPlane(const Rect& rect) const
{
	return { toPlane(rect.getPosition()), rect.getSize() / sheet->grid_pixel_size };
}

Rect Window_Sheet::getDragRect(vk2d::Mouse::Button button)
{
	ImVec2 p_min, p_max;

	auto cursor_pos = ImGui::GetMousePos();
	auto pos        = ImVec2(cursor_pos.x - viewport->Pos.x, cursor_pos.y - viewport->Pos.y);
	auto delta      = ImGui::GetMouseDragDelta(button);

	p_min.x = std::min(pos.x, pos.x - delta.x);
	p_min.y = std::min(pos.y, pos.y - delta.y);
	p_max.x = std::max(pos.x, pos.x - delta.x);
	p_max.y = std::max(pos.y, pos.y - delta.y);

	return { to_vec2(p_min), to_vec2(p_max) - to_vec2(p_min) };
}

vec2 Window_Sheet::getCursorPlanePos() const
{
	return toPlane(getCursorPos());
}

vec2 Window_Sheet::getClampedCursorPlanePos() const
{
	auto pos = getCursorPlanePos();
	pos.x    = roundf(2.f * pos.x) / 2.f;
	pos.y    = roundf(2.f * pos.y) / 2.f;
	return pos;
}

vec2 Window_Sheet::getClampedCursorPos() const
{
	return toScreen(getClampedCursorPlanePos());
}

BVH<std::unique_ptr<CircuitElement>>& Window_Sheet::getBVH()
{
	return sheet->bvh;
}
