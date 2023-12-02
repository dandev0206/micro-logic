#define IMGUI_DEFINE_MATH_OPERATORS

#include "dialogs.h"

#include <vk2d/system/file_dialog.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <filesystem>
#include "gui/platform/system_folder.h"
#include "gui/imgui_impl_vk2d.h"
#include "gui/imgui_custom.h"
#include "util/convert_string.h"
#include "micro_logic_config.h"

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
		ImGui::TextUnformatted(content.c_str());

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

ProjectSaveDialog::ProjectSaveDialog() :
	Dialog("", true),
	save_as(false)
{}

std::string ProjectSaveDialog::showDialog()
{
	Dialog::switchContext();
	
	std::string result = "Close";

	std::string project_location;
	bool        create_subdirectory = true;

	project_dir.resize(256);
	project_name.resize(256);
	project_location.resize(256);

	title               = save_as ? "Save project as..." : "Save project";
	project_location    = GetSystemFolderPath(SystemFolders::Desktop);
	project_name        = get_untitled_name(project_location);

	window.setTitle(title.c_str());
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
			vk2d::FolderOpenDialog dialog;

			auto default_dir  = to_wide(project_location);

			dialog.title        = L"Choose project directory";
			dialog.default_dir  = default_dir.c_str();
			dialog.owner        = &window;
			
			if (dialog.showDialog() == vk2d::DialogResult::OK)
				project_location = to_unicode(dialog.getResultDir());
			
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

	auto text_size    = ImGui::CalcTextSize(parent_dir.c_str());
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
		ImGui::Text("%s/", parent_dir.c_str());
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
