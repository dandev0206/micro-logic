#include "window_library.h"

#include <imgui.h>
#include <imgui_internal.h>
#include "../gui/imgui_custom.h"
#include "../main_window.h"
#include "../side_menus.h"
#include "../micro_logic_config.h"

#define ICON_TEXTURE_VAR main_window.textures[TEXTURE_ICONS_IDX]
#include "../icons.h"

Window_Library::Window_Library() :
	menu(nullptr),
	preview_rect(),
	preview_pos(0.f, 0.f),
	preview_scale(1.f),
	mouse_captured(false),
	is_moving(false)
{}

void Window_Library::showUI()
{
	if (!show) return;

	auto& main_window = MainWindow::get();

	DockingWindow::beginDockWindow("Library", &show);

	auto child_flags = ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY;
	auto preview_flags = ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoScrollbar;

	ImGui::Text("preview");
	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 45);
	if (ImGui::ImageButton(ICON_ROTATE_CW, { 20, 20 })) {
		resetPreview();
	}

	ImGui::BeginChild("preview", { 0, 130 }, child_flags, preview_flags);
	if (menu->curr_gate != -1) {
		auto& gate    = main_window.logic_gates[menu->curr_gate];
		auto& texture = main_window.textures[gate.shared->texture_id];
		auto rect     = gate.shared->texture_coord;
		auto offset   = gate.shared->extent.getPosition() * DEFAULT_GRID_SIZE;
		auto size     = gate.shared->extent.getSize() * DEFAULT_GRID_SIZE;

		preview_rect.setPosition(to_vec2(ImGui::GetWindowContentRegionMin()));
		preview_rect.setSize(to_vec2(ImGui::GetWindowContentRegionMax()) - preview_rect.getPosition());
		preview_rect.setPosition(preview_rect.getPosition() + to_vec2(ImGui::GetWindowPos()) - to_vec2(viewport->Pos));

		ImGui::SetCursorPos(to_ImVec2(preview_rect.getSize() / 2.f + preview_scale * (preview_pos + offset)));
		ImGui::Image(texture, rect, size * preview_scale);

		if (mouse_captured)
			ImGui::SetKeyOwner(ImGuiKey_MouseWheelY, ImGui::GetItemID(), ImGuiInputFlags_LockThisFrame);
	}
	ImGui::EndChild();

	auto table_flags =
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_BordersV |
		ImGuiTableFlags_BordersOuterH |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_NoBordersInBody |
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingFixedFit;

	ImGui::Text("recently used");
	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 45);
	if (ImGui::ImageButton(ICON_TRASH_CAN, { 20, 20 })) {
		recently_used.clear();
		if (menu->curr_gate != -1)
			recently_used.emplace_back(menu->curr_gate);
	}

	ImGui::BeginChild("recently used##Child", { 0, 150.f }, child_flags);
	if (ImGui::BeginTable("recently used", 1, table_flags))
	{
		ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		for (auto iter = recently_used.rbegin(); iter != recently_used.rend(); ++iter) {
			auto& gate = main_window.logic_gates[*iter];

			ImGui::TableNextRow(ImGuiTableRowFlags_None);
			ImGui::TableSetColumnIndex(0);
			showSelectable(gate);
		}

		ImGui::EndTable();
	}
	ImGui::EndChild();

	ImGui::Text("parts");
	ImGui::BeginChild("parts##Child", { 0, 300.f }, child_flags);
	if (ImGui::BeginTable("table", 2, table_flags))
	{
		ImGui::TableSetupColumn("name");
		ImGui::TableSetupColumn("description");
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		for (int i = 0; i < main_window.logic_gates.size(); ++i) {
			auto& gate = main_window.logic_gates[i];

			ImGui::TableNextRow(ImGuiTableRowFlags_None);
			ImGui::TableSetColumnIndex(0);
			showSelectable(gate);

			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted(gate.shared->description.c_str());
		}

		ImGui::EndTable();
	}
	ImGui::EndChild();

	updateRecentlyUsed();

	DockingWindow::endDockWindow();
}

void Window_Library::EventProc(const vk2d::Event& e, float dt)
{
	using vk2d::Event;
	using vk2d::Mouse;

	switch (e.type) {
	case Event::MousePressed: {
		if (e.mouseButton.button != Mouse::Left || !mouse_captured) break;

		is_moving = true;
	} break;
	case Event::MouseReleased: {
		is_moving = false;
	} break;
	case Event::MouseMoved: {
		mouse_captured = menu->curr_gate != -1 && preview_rect.contain(e.mouseMove.pos);

		if (!is_moving) break;

		preview_pos += (vec2)e.mouseMove.delta / preview_scale;
	} break;
	case Event::WheelScrolled: {
		if (!preview_rect.contain(e.wheel.pos)) break;

		auto mouse_pos = (vec2)e.wheel.pos;

		auto pos = (mouse_pos - preview_rect.center()) / preview_scale;
		
		if (e.wheel.delta.y < 0)
			preview_scale = std::min(preview_scale * 1.1f, 10.f);
		else if (e.wheel.delta.y > 0)
			preview_scale = std::max(preview_scale / 1.1f, 0.1f);

		preview_pos -= pos - (mouse_pos - preview_rect.center()) / preview_scale;
	} break;
	}
}

void Window_Library::bindMenuLibrary(Menu_Library& menu)
{
	this->menu = &menu;
}

void Window_Library::updateRecentlyUsed()
{
	auto gate_idx = menu->curr_gate;

	if (gate_idx == -1) return;
	if (!recently_used.empty() && recently_used.back() == gate_idx) return;

	auto iter = std::find(recently_used.begin(), recently_used.end(), gate_idx);

	if (iter != recently_used.end())
		recently_used.erase(iter);

	recently_used.emplace_back(gate_idx);
}

void Window_Library::showSelectable(const LogicGate& gate)
{
	auto gate_idx = gate.shared->element_id;
	auto flags    = ImGuiSelectableFlags_SpanAllColumns;

	ImGui::PushID((int)gate_idx);

	if (ImGui::Selectable(gate.shared->name.c_str(), gate_idx == menu->curr_gate, flags)) {
		if (menu->curr_gate == gate_idx)
			menu->curr_gate = -1;
		else {
			auto& main_window = MainWindow::get();

			menu->curr_gate = (int32_t)gate_idx;
			resetPreview();
			auto menu = main_window.findSideMenu<Menu_Library>();
			main_window.setCurrentSideMenu(menu);
		}
	}

	ImGui::PopID();
}

void Window_Library::resetPreview()
{
	preview_pos   = vec2(0.f, 0.f);
	preview_scale = 1.f;
}


