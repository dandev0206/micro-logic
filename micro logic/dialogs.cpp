#define IMGUI_DEFINE_MATH_OPERATORS

#include "dialogs.h"

#define ICON_TEXTURE_VAR MainWindow::get().textures[TEXTURE_ICONS_IDX]

#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <filesystem>
#include "platform/system_dialog.h"
#include "gui/imgui_impl_vk2d.h"
#include "gui/imgui_custom.h"
#include "util/convert_string.h"
#include "main_window.h"
#include "icons.h"
#include "micro_logic_config.h"

#define TITLE_BAR_HEIGHT 45.f
#define WINDOW_PADDING_WIDTH 20.f
#define WINDOW_PADDING_HEIGHT 20.f
#define WINDOW_PADDING ImVec2(20.f, 20.f)
#define PADDING_WIDTH 10.f
#define PADDING_HEIGHT 10.f
#define PADDING_SIZE ImVec2(10.f, 10.f)
#define SPACING_WIDTH 20.f
#define SPACING_HEIGHT 20.f
#define SPACING_SIZE ImVec2(20.f, 20.f)
#define ICON_WIDTH 70.f
#define ICON_SIZE ImVec2(70.f, 70.f)
#define BUTTON_WIDTH 100.f
#define BUTTON_HEIGHT 40.f
#define BUTTON_SIZE ImVec2(0.f, 40.f)
#define MESSAGE_BOX_WIDTH 600.f

namespace fs = std::filesystem;

static const auto window_flags =
	ImGuiWindowFlags_NoResize |
	ImGuiWindowFlags_NoMove |
	ImGuiWindowFlags_NoCollapse |
	ImGuiWindowFlags_NoScrollWithMouse |
	ImGuiWindowFlags_NoDecoration;

static std::string get_untitled_name(const std::string& path)
{
	std::string result;
	int32_t     num = 0;

	do {
		result = "Untitled" + std::to_string(num++);
	} while (fs::exists(path + "\\" + result));

	return result;
}

static std::vector<std::string> split_str(const std::string& str, char ch)
{
	if (str.empty()) return {};

	std::vector<std::string> result;

	size_t first = 0;
	size_t last  = 0;

	while (true) {
		last = str.find(ch, first);

		if (last == std::string::npos) {
			result.push_back(str.substr(first, str.size() - first));
			break;
		} else {
			result.push_back(str.substr(first, last - first));
		}
		first = last + 1;
	}

	return result;
}

static ImVec2 calc_buttons_size(const char* button_names)
{
	const char* first = button_names;
	const char* last  = nullptr;

	auto buttons_width = -SPACING_WIDTH;
	do {
		last = strchr(first, ';');

		auto width = ImGui::CalcTextSize(first, last).x + 2.f * PADDING_WIDTH;
		buttons_width += std::max(BUTTON_WIDTH, width) + SPACING_WIDTH;

		first = last + 1;
	} while (last);

	return ImVec2(buttons_width, BUTTON_HEIGHT);
}

static ImVec2 calc_buttons_size(const std::vector<std::string>& button_names)
{
	auto buttons_width = -SPACING_WIDTH;
	for (const auto& name : button_names) {
		auto width = ImGui::CalcTextSize(name.c_str()).x + 2.f * PADDING_WIDTH;
		buttons_width += std::max(BUTTON_WIDTH, width) + SPACING_WIDTH;
	}

	return ImVec2(buttons_width, BUTTON_HEIGHT);
}

static bool DialogButton(const char* label)
{
	auto width = ImGui::CalcTextSize(label).x + 2.f * PADDING_WIDTH;
	auto size  = ImVec2(std::max(BUTTON_WIDTH, width), BUTTON_HEIGHT);

	return ImGui::Button(label, size);
}

MessageBox::MessageBox() :
	Dialog(false),
	buttons("Ok")
{}

std::string MessageBox::showDialog() const
{
	std::string result = "Close";

	Dialog::switchContext();

	auto icon_size    = (icon.empty() ? ImVec2(0.f, 0.f) : ICON_SIZE) + SPACING_SIZE;
	auto wrap_width   = MESSAGE_BOX_WIDTH - 2.f * WINDOW_PADDING_WIDTH - icon_size.x;
	auto button_names = split_str(buttons, ';');
	auto buttons_size = calc_buttons_size(button_names);
	auto content_size = ImGui::CalcTextSize(content.c_str(), nullptr, false, wrap_width) + SPACING_SIZE;

	auto required_size = ImVec2(MESSAGE_BOX_WIDTH, 2 * WINDOW_PADDING_HEIGHT);
	required_size.y   += std::max(icon_size.y, content_size.y);
	required_size.y   += buttons_size.y;

	titlebar.setButtonStyle(false, false, true);
	Dialog::beginShowDialog(required_size);

	while (window.isVisible()) {
		if (!Dialog::updateDialog()) break;

		Dialog::beginDialogWindow("MessageBox", nullptr, window_flags);

		if (!icon.empty())
			ImGui::Image(icon, to_vec2(ICON_SIZE));

		ImGui::SameLine();
		ImGui::TextWrapped(content.c_str());

		ImGui::SetCursorPos(client_size - buttons_size - WINDOW_PADDING);
		for (const auto& name : button_names) {
			if (DialogButton(name.c_str())) {
				result = name;
				window.setVisible(false);
			}
			ImGui::SameLine();
		}

		Dialog::endDialogWindow();
		Dialog::renderDialog();
	}

	Dialog::endShowDialog();

	return result;
}

ListMessageBox::ListMessageBox() :
	Dialog(true)
{}

std::string ListMessageBox::showDialog() const
{
	std::string result = "Close";
	
	Dialog::switchContext();

	auto icon_size    = (icon.empty() ? ImVec2(0.f, 0.f) : ICON_SIZE) + SPACING_SIZE;
	auto wrap_width   = MESSAGE_BOX_WIDTH - 2.f * WINDOW_PADDING_WIDTH - icon_size.x;
	auto button_names = split_str(buttons, ';');
	auto buttons_size = calc_buttons_size(button_names);
	auto content_size = ImGui::CalcTextSize(content.c_str(), nullptr, false, wrap_width) + SPACING_SIZE;

	auto required_size = ImVec2(MESSAGE_BOX_WIDTH, 2 * WINDOW_PADDING_HEIGHT);
	required_size.y   += std::max(icon_size.y, content_size.y);
	required_size.y   += 2.f * ImGui::GetFrameHeight();
	required_size.y   += 2 * WINDOW_PADDING_HEIGHT + buttons_size.y;
	required_size.y   += std::clamp(list.size() * (FONT_SIZE + 2.f * PADDING_HEIGHT), 200.f, 400.f);

	titlebar.setButtonStyle(false, false, true);
	Dialog::beginShowDialog(required_size);

	Dialog::dialogLoop([&] {
		Dialog::beginDialogWindow("ListMessageBox", nullptr, window_flags);

		if (!icon.empty())
			ImGui::Image(icon, to_vec2(ICON_SIZE));
		
		ImGui::SameLine();
		ImGui::TextWrapped(content.c_str());

		ImGui::Separator();

		auto child_height = client_size.y;
		child_height -= ImGui::GetCursorPosY() + ImGui::GetFrameHeight();
		child_height -= BUTTON_HEIGHT + WINDOW_PADDING_HEIGHT + 10;
		
		// window.setMinSizeLimit({ 0, window_size.y - child_height - ImGui::GetFrameHeight() });

		ImGui::BeginChild("ListMessageBoxChild", ImVec2(0.f, child_height));
		for (const auto& names : list)
			ImGui::TextUnformatted(names.c_str());
		ImGui::EndChild();

		ImGui::Separator();

		ImGui::SetCursorPos(client_size - buttons_size - WINDOW_PADDING);
		for (const auto& name : button_names) {
			if (DialogButton(name.c_str())) {
				result = name;
				window.setVisible(false);
			}
			ImGui::SameLine();
		}

		Dialog::endDialogWindow();
		Dialog::renderDialog();
	});

	Dialog::endShowDialog();

	return result;
}

SettingsDialog::SettingsDialog() :
	Dialog(icon_to_texture_view(ICON_GEAR), "Settings", true)
{}

void SettingsDialog::showDialog()
{
	auto& main_window     = MainWindow::get();
	std::string result    = "Close";
	auto settings         = main_window.settings;
	int curr_setting_menu = settings.curr_settings_menu;

	Dialog::switchContext();

	Dialog::beginShowDialog(ImVec2(1000, 800));

	vec2 menu_button_size(250, 60);
	auto buttons_size = calc_buttons_size("OK;Cancel");

	float preview_scale = 1.f;

	Dialog::dialogLoop([&] {
		Dialog::beginDialogWindow("Settings", nullptr, window_flags);

		ImGui::RadioButton("General##Settings", &curr_setting_menu, 0, menu_button_size);
		ImGui::RadioButton("Rendering##Settings", &curr_setting_menu, 1, menu_button_size);
		ImGui::RadioButton("Grid##Settings", &curr_setting_menu, 2, menu_button_size);
		ImGui::RadioButton("Simulation##Settings", &curr_setting_menu, 3, menu_button_size);
		ImGui::RadioButton("Settings0##Settings", &curr_setting_menu, 4, menu_button_size);
		ImGui::RadioButton("Settings1##Settings", &curr_setting_menu, 5, menu_button_size);
		ImGui::RadioButton("Settings2##Settings", &curr_setting_menu, 6, menu_button_size);

		auto child_height = client_size.y - buttons_size.y - 2 * WINDOW_PADDING_HEIGHT - SPACING_HEIGHT;
		ImGui::SetCursorPos(ImVec2(menu_button_size.x + 2.f * WINDOW_PADDING_WIDTH, WINDOW_PADDING_HEIGHT));
		ImGui::BeginChild("##Settings Child", ImVec2(0, child_height), ImGuiChildFlags_Border);

		if (curr_setting_menu == 0) {

		} else if (curr_setting_menu == 1) {
			ImGui::Checkbox("Enable vSync", &settings.rendering.enable_vsync);

			ImGui::BeginDisabled(settings.rendering.enable_vsync);
			ImGui::Checkbox("Frame Limit", &settings.rendering.frame_limit);

			ImGui::TextUnformatted("Max FPS");
			ImGui::BeginDisabled(!settings.rendering.frame_limit);
			ImGui::InputInt("##Max FPS", &settings.rendering.max_fps);
			
			ImGui::EndDisabled();
			ImGui::EndDisabled();
		} else if (curr_setting_menu == 2) {
			ImGui::TextUnformatted("preview");
			ImGui::BeginChild("preview", ImVec2(0, 200), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY);
			auto pos             = ImGui::GetWindowPos();
			auto size            = ImGui::GetWindowSize();
			auto grid_pixel_size = preview_scale * DEFAULT_GRID_SIZE;

			auto pos_min = ImVec2(pos.x, std::max(pos.y, TITLE_BAR_HEIGHT + WINDOW_PADDING_HEIGHT));
			auto pos_max = ImVec2(pos.x + size.x, std::max(pos.y + size.y, TITLE_BAR_HEIGHT + WINDOW_PADDING_HEIGHT));
			auto center  = pos + size / 2.f;

			auto* draw_list = ImGui::GetForegroundDrawList();

			draw_list->PushClipRect(pos_min, pos_max);
			draw_list->AddRectFilled(pos_min, pos_max, 0x000000ff);

			if (settings.view.grid_axis) {
				float width = settings.view.grid_axis_width;
				auto color  = settings.view.grid_axis_color;

				draw_list->AddLine(ImVec2(pos_min.x, center.y), ImVec2(pos_max.x, center.y), to_ImColor(color), width);
				draw_list->AddLine(ImVec2(center.x, pos_min.y), ImVec2(center.x, pos_max.y), to_ImColor(color), width);
			}

			if (settings.view.grid_style == GridStyle::dot) {
				auto delta     = grid_pixel_size;
				auto padding   = ImVec2(settings.view.grid_width, settings.view.grid_width) / 2.f;
				auto half_size = size / 2.f;
				auto color     = settings.view.grid_color;

				for (float x = 0; x < half_size.x; x += delta) {
					for (float y = 0; y <= half_size.y; y += delta) {
						draw_list->AddRectFilled(center + ImVec2(x, y) - padding, center + ImVec2(x, y) + padding, to_ImColor(color));
						draw_list->AddRectFilled(center + ImVec2(x, -y) - padding, center + ImVec2(x, -y) + padding, to_ImColor(color));
						draw_list->AddRectFilled(center + ImVec2(-x, y) - padding, center + ImVec2(-x, y) + padding, to_ImColor(color));
						draw_list->AddRectFilled(center + ImVec2(-x, -y) - padding, center + ImVec2(-x, -y) + padding, to_ImColor(color));
					}
				}
			} else {
				auto width = settings.view.grid_width;
				auto delta = grid_pixel_size;
				auto color = settings.view.grid_color;

				for (float y = center.y; y <= pos_max.y; y += delta) {
					draw_list->AddLine(ImVec2(pos_min.x, y), ImVec2(pos_max.x, y), to_ImColor(color), width);
					draw_list->AddLine(ImVec2(pos_min.x, 2.f * center.y - y), ImVec2(pos_max.x, 2.f * center.y - y), to_ImColor(color), width);
				}

				for (float x = center.x; x < pos_max.x; x += delta) {
					draw_list->AddLine(ImVec2(x, pos_min.y), ImVec2(x, pos_max.y), to_ImColor(color), width);
					draw_list->AddLine(ImVec2(2.f * center.x - x, pos_min.y), ImVec2(2.f * center.x - x, pos_max.y), to_ImColor(color), width);
				}
			}
			draw_list->PopClipRect();

			ImGui::EndChild();

			ImGui::SetCursorPosX(size.x - 390);
			if (ImGui::ImageButton(ICON_MAG_MINUS, { 17, 17 })) preview_scale /= 1.05f;
			ImGui::SameLine();
			ImGui::SetNextItemWidth(200);
			ImGui::ZoomSlider("##Zoom Slider", &preview_scale, MIN_SCALE, MAX_SCALE);
			ImGui::SameLine();
			if (ImGui::ImageButton(ICON_MAG_PLUS, { 17, 17 })) preview_scale *= 1.05f;
			ImGui::SameLine();
			ImGui::Text("%.1f%%", 100.f * preview_scale);

			ImGui::TextUnformatted("Style");
			ImGui::Combo("##ComboStyle", (int*)&settings.view.grid_style, "line\0dot", 2);
			ImGui::TextUnformatted("Grid Width");
			ImGui::SliderFloat("##SliderGridWidth", &settings.view.grid_width, 1, 4, "%.1f");
			ImGui::TextUnformatted("Grid Color");
			ImGui::ColorEdit4("##ColorEditGridcolor", &settings.view.grid_color);
			ImGui::Checkbox("ShowAxis", &settings.view.grid_axis);
			ImGui::BeginDisabled(!settings.view.grid_axis);
			ImGui::TextUnformatted("Axis Width");
			ImGui::SliderFloat("##SliderFloatAxisWidth", &settings.view.grid_axis_width, 1, 4, "%.1f");
			ImGui::TextUnformatted("Axis Color");
			ImGui::ColorEdit4("##ColorEditAxiscolor", &settings.view.grid_axis_color);
			ImGui::EndDisabled();
		} else if (curr_setting_menu == 3) {
		} else if (curr_setting_menu == 4) {
		} else if (curr_setting_menu == 5) {
		} else if (curr_setting_menu == 6) {
		}

		ImGui::EndChild();

		ImGui::SetCursorPos(client_size - buttons_size - WINDOW_PADDING);
		if (DialogButton("OK")) {
			result = "OK";
			window.setVisible(false);
		}
		ImGui::SameLine();
		if (DialogButton("Cancel")) {
			result = "Cancel";
			window.setVisible(false);
		}

		Dialog::endDialogWindow();
		Dialog::renderDialog();
	});

	Dialog::endShowDialog();

	if (result != "OK") {
		main_window.settings.curr_settings_menu = curr_setting_menu;
		return;
	} else {
		main_window.settings = settings;
		main_window.settings.curr_settings_menu = curr_setting_menu;
	}

	// update settings
	// general
	
	// rendering
	main_window.window.enableVSync(settings.rendering.enable_vsync);
	if (settings.rendering.frame_limit)
		main_window.setFrameLimit(settings.rendering.max_fps);
	else	
		main_window.setFrameLimit(0);

	// grid
	for (auto& ws : main_window.window_sheets)
		ws->update_grid = true;
}

ProjectSaveDialog::ProjectSaveDialog() :
	Dialog(true),
	save_as(false)
{}

std::string ProjectSaveDialog::showDialog()
{
	std::string result = "Close";
	
	Dialog::switchContext();

	std::string project_location;
	bool        create_subdirectory = true;

	project_dir.resize(256);
	project_name.resize(256);
	project_location.resize(256);

	title               = save_as ? "Save project as..." : "Save project";
	project_location    = GetSystemFolderPath(SystemFolders::Desktop);
	project_name        = get_untitled_name(project_location);

	titlebar.setButtonStyle(false, false, true);
	Dialog::beginShowDialog(ImVec2(1000, 280));

	auto update_project_dir = [&] {
		if (create_subdirectory)
			project_dir = project_location + "\\" + project_name;
		else
			project_dir = project_location;
	};

	update_project_dir();

	Dialog::dialogLoop([&] {
		static auto color_warning = ImVec4(0.8f, 0.73f, 0.16f, 1.f);
		static auto color_error   = ImVec4(0.96f, 0.34f, 0.38f, 1.f);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 20));
		Dialog::beginDialogWindow("Save Project", nullptr, window_flags);

		ImGui::TextUnformatted("Project name:    ");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(client_size.x - ImGui::GetCursorPosX() - 20);
		if (ImGui::InputText("##ProjectName", &project_name))
			update_project_dir();

		ImGui::TextUnformatted("Project location:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(client_size.x - ImGui::GetCursorPosX() - 78);
		if (ImGui::InputText("##ProjectDir", &project_location))
			update_project_dir();
		ImGui::SameLine();
		if (ImGui::Button("...")) {
			FolderOpenDialog dialog;

			dialog.title        = "Choose project directory";
			dialog.default_dir  = project_location;
			dialog.owner        = &window;
			
			if (dialog.showDialog() == DialogResult::OK)
				project_location = dialog.path;
			
			update_project_dir();
		}
		
		if (ImGui::Checkbox("Create project subdirectory", &create_subdirectory))
			update_project_dir();

		ImGui::Text("project will be created at: \n%s", project_dir.c_str());

		if (fs::exists(project_dir))
			ImGui::TextColored(color_warning, "warning: directory is not empty");

		ImGui::SetCursorPosX(client_size.x - 180);
		ImGui::SetCursorPosY(client_size.y - BUTTON_HEIGHT - 20);
		if (ImGui::Button("Save", BUTTON_SIZE)) {
			if (create_subdirectory)
				fs::create_directory(project_dir);

			result = "Save";
			window.setVisible(false);
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", BUTTON_SIZE)) {
			result = "Cancel";
			window.setVisible(false);
		}

		Dialog::endDialogWindow();
		ImGui::PopStyleVar(2);
		Dialog::renderDialog();
	});

	Dialog::endShowDialog();

	return result;
}

/* template for resizable dialogs
std::string ExampleDialog::showDialog() const
{
	Dialog::switchContext();

	std::string result = "Error";

	titlebar.setButtonStyle(false, false, true);
	Dialog::beginShowDialog(ImVec2(1000, 500));

	while (window.isVisible()) {
		if (!Dialog::updateDialog())
			result = "close";

		Dialog::beginDialogWindow("Example Dialog", nullptr, flags);



		Dialog::endDialogWindow();
		Dialog::renderDialog();
	}

	Dialog::endShowDialog();

	return result;
}
*/

SchematicSheetNameDialog::SchematicSheetNameDialog() :
	Dialog("", false)
{}

std::string SchematicSheetNameDialog::showDialog()
{
	Dialog::switchContext();

	std::string result = "Error";

	auto text_size    = ImGui::CalcTextSize(project_dir.c_str());
	auto ext_size     = ImGui::CalcTextSize(SCHEMATIC_SHEET_EXT);
	auto button_names = split_str(buttons, ';');
	auto buttons_size = calc_buttons_size(button_names);

	auto required_size = ImVec2(400, 70);
	required_size.x += text_size.x + ext_size.x;
	required_size.y += text_size.y + buttons_size.y;

	titlebar.setButtonStyle(false, false, true);
	Dialog::beginShowDialog(required_size);

	while (window.isVisible()) {
		if (!Dialog::updateDialog()) {
			result = "Close";
			break;
		}

		Dialog::beginDialogWindow("ProjectClose", nullptr, window_flags);

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3, 10));
		ImGui::Text("%s/", project_dir.c_str());
		ImGui::SameLine();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
		ImGui::SetNextItemWidth(330);
		ImGui::InputText("##InputText", &sheet_path);
		ImGui::SameLine();
		ImGui::TextUnformatted(SCHEMATIC_SHEET_EXT);
		ImGui::PopStyleVar();

		ImGui::SetCursorPos(client_size - buttons_size - WINDOW_PADDING);
		for (const auto& name : button_names) {
			if (DialogButton(name.c_str())) {
				result = name;
				window.setVisible(false);
			}
			ImGui::SameLine();
		}

		Dialog::endDialogWindow();
		Dialog::renderDialog();
	}

	Dialog::endShowDialog();

	sheet_name  = fs::path(sheet_path).filename().generic_string();
	sheet_path += SCHEMATIC_SHEET_EXT;

	return result;
}
