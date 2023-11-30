#include "window_history.h"

#include "../main_window.h"
#include "../gui/imgui_custom.h"
#include "../micro_logic_config.h"

#define ICON_TEXTURE_VAR main_window.textures[TEXTURE_ICONS_IDX]
#include "../icons.h"

Window_History::Window_History() :
	show(true)
{}

void Window_History::showUI()
{
	if (!show) return;

	DockingWindow::beginDockWindow("History", &show);

	char buf[10];

	auto table_flags =
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_BordersV |
		ImGuiTableFlags_BordersOuterH |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_NoBordersInBody |
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingFixedFit;

	auto item_flags = 
		ImGuiSelectableFlags_SpanAllColumns | 
		ImGuiSelectableFlags_DontClosePopups;

	auto& main_window = MainWindow::get();

	if (main_window.curr_window_sheet) {
		auto& ws = main_window.getCurrentWindowSheet();

		ImGui::TextUnformatted("Histories");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 45);
		if (ImGui::ImageButton(ICON_TRASH_CAN, { 20, 20 }))
			ws.clearCommand();

		if (ImGui::BeginTable("History##Table", 3, table_flags)) {
			int64_t next_command = ws.curr_command;

			ImGui::TableSetupColumn("id");
			ImGui::TableSetupColumn("description");
			ImGui::TableSetupColumn("saved");
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableHeadersRow();

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 0.5f));

			for (int64_t i = ws.command_stack.size() - 1; 0 <= i; --i) {
				auto& cmd = ws.command_stack[i];

				if (i == ws.curr_command)
					ImGui::PopStyleColor();

				ImGui::TableNextRow(ImGuiTableRowFlags_None);

				sprintf_s(buf, 10, "%lld", i);
				ImGui::TableSetColumnIndex(0);
				ImGui::PushID((ImGuiID)i);
				if (ImGui::Selectable(buf, i == ws.curr_command, item_flags)) {
					next_command = i;
				}
				ImGui::PopID();

				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(cmd->what().c_str());

				if (ws.isCommandInSavedRange(i)) {
					ImGui::TableSetColumnIndex(2);
					ImGui::Image(ICON_CHECK, vec2(20, 20));
				}
			}

			if (ws.curr_command == -1)
				ImGui::PopStyleColor();

			ImGui::TableNextRow(ImGuiTableRowFlags_None);

			ImGui::TableSetColumnIndex(0);
			if (ImGui::Selectable("", ws.curr_command == -1, item_flags))
				next_command = -1;

			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted("begin");

			if (ws.last_saved_command_min == -1) {
				ImGui::TableSetColumnIndex(2);
				ImGui::Image(ICON_CHECK, vec2(20, 20));
			}

			ImGui::EndTable();

			if (next_command != ws.curr_command)
				ws.setCurrCommandTo(next_command);
		}
	} else {
		static const char* msg = "there is no window sheet avaliable";

		auto size = ImGui::CalcTextSize(msg);
		auto pos  = ImVec2((content_rect.width - size.x) / 2.f, (content_rect.height + size.y) / 2.f);

		ImGui::SetCursorPos(pos);
		ImGui::TextUnformatted(msg);
	}

	DockingWindow::endDockWindow();
}
