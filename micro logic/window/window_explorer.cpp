#include "window_explorer.h"

#define ICON_TEXTURE_VAR main_window.textures[TEXTURE_ICONS_IDX]
#include <imgui.h>
#include "../main_window.h"
#include "../gui/imgui_custom.h"
#include "../micro_logic_config.h"
#include "../icons.h"

Window_Explorer::Window_Explorer()
{}

void Window_Explorer::showUI()
{
	if (!show) return;

	DockingWindow::beginDockWindow("Project Explorer", &show);

	if (visible) {
		auto& main_window = MainWindow::get();

		auto child_flags  = ImGuiChildFlags_Border;
		auto window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

		auto child_width = 0.95f * content_rect.width;
		auto button_size = ImVec2(child_width - 20, (child_width - 20) * 0.75f);
		auto child_size  = ImVec2(child_width, child_width * 0.75f + 35);

		SchematicSheet* delete_sheet = nullptr;

		for (size_t i = 0; i < main_window.sheets.size(); ++i) {
			auto& sheet     = main_window.sheets[i];
			auto& thumbnail = sheet->thumbnail;
			Rect rect(vec2(0.f, 0.f), thumbnail.size());

			ImGui::SetCursorPosX((content_rect.width - child_width) / 2 + 5);
			ImGui::BeginChild((ImGuiID)(i + 10), child_size, child_flags, window_flags);

			if (sheet->file_saved)
				ImGui::Text("#%d - %s", i, sheet->name.c_str());
			else
				ImGui::Text("#%d - *%s", i, sheet->name.c_str());

			ImGui::SameLine();
			ImGui::SetCursorPosX(child_width - 80);
			if (ImGui::ImageButton(ICON_SAVE, vec2(20, 20)))
				main_window.saveSchematicSheet(*sheet);

			ImGui::SameLine();
			if (ImGui::ImageButton(ICON_TRASH_CAN, vec2(20, 20)))
				delete_sheet = sheet.get();

			auto prev_cursor_pos = ImGui::GetCursorPos();
			ImGui::Image(thumbnail, rect, to_vec2(button_size));

			ImGui::SetCursorPos(prev_cursor_pos);
			ImGui::PushID((ImGuiID)(i + 10));
			if (ImGui::Selectable("##Selectable", false, ImGuiSelectableFlags_AllowDoubleClick, button_size)) {
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					main_window.openWindowSheet(*main_window.sheets[i]);
			}
			ImGui::PopID();

			ImGui::EndChild();
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 40);
		}

		ImGui::SetCursorPosX(content_rect.width / 2 - 90);
		if (ImGui::ImageButton(ICON_PLUS, vec2(180, 60)))
			main_window.addSchematicSheet();

		main_window.deleteSchematicSheet(*delete_sheet);

		ImGui::TextUnformatted("");
	}

	DockingWindow::endDockWindow();
}
