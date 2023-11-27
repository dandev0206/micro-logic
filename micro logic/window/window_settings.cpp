#include "window_settings.h"

#include <imgui.h>
#include "../main_window.h"
#include "../gui/imgui_custom.h"

Window_Settings::Window_Settings() :
	show(false)
{}

void Window_Settings::showUI()
{
	if (show)
		ImGui::OpenPopup("Settings");

	auto flags = ImGuiWindowFlags_NoDocking;// | ImGuiWindowFlags_NoResize;
	if (ImGui::BeginPopupModal("Settings", &show, flags)) {
		auto& main_window = MainWindow::get();
		auto& settings    = main_window.settings;
		vec2 size(250, 60);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(15, 15));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.f));

		ImGui::RadioButton("General##Settings", &curr_setting, 0, size);
		ImGui::RadioButton("Grid##Settings", &curr_setting, 1, size);
		ImGui::RadioButton("Simulation##Settings", &curr_setting, 2, size);
		ImGui::RadioButton("Settings0##Settings", &curr_setting, 3, size);
		ImGui::RadioButton("Settings1##Settings", &curr_setting, 4, size);
		ImGui::RadioButton("Settings2##Settings", &curr_setting, 5, size);

		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		ImGui::SetCursorPos(ImVec2(270, 40));
		ImGui::BeginChild("##Settings Child", ImVec2(0, 0), ImGuiChildFlags_Border);
		
		if (curr_setting == 1) {
			bool update_grid = false;

			update_grid |= ImGui::Combo("style", (int*)&settings.view.grid_style, "line\0dot", 2);
			update_grid |= ImGui::Checkbox("show axis", &settings.view.grid_axis);
			update_grid |= ImGui::SliderFloat("width", &settings.view.grid_width, 1, 4, "%.1f");
			update_grid |= ImGui::ColorEdit4("grid color##grid_color", &settings.view.grid_color);
			update_grid |= ImGui::ColorEdit4("axis color##axis_color", &settings.view.grid_axis_color);

			if (update_grid) {
				for (auto& window_sheet : main_window.window_sheets)
					window_sheet.update_grid = true;
			}
		}

		ImGui::EndChild();
		ImGui::EndPopup();
	}
}
