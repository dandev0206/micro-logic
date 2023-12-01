#include "dialogs.h"

#include <vk2d/system/file_dialog.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <filesystem>
#include "gui/platform/system_folder.h"
#include "gui/imgui_impl_vk2d.h"
#include "util/convert_string.h"
#include "main_window.h"
#include "micro_logic_config.h"

#define BUTTON_WIDTH 100.f
#define BUTTON_HEIGHT 40.f
#define BUTTON_SIZE ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)

namespace fs = std::filesystem;

static const auto flags =
	ImGuiWindowFlags_NoResize |
	ImGuiWindowFlags_NoMove |
	ImGuiWindowFlags_NoCollapse |
	ImGuiWindowFlags_NoScrollWithMouse |
	ImGuiWindowFlags_NoDecoration;

static std::string get_untitled_name(const std::string& dir)
{
	std::string result;
	int32_t     num = 0;

	do {
		result = "Untitled" + std::to_string(num++);
	} while (fs::exists(dir + "\\" + result));

	return result;
}

static std::vector<std::string> split_str(const std::string& str, char ch)
{
	std::vector<std::string> result;

	size_t first = 0;
	size_t last = str.find(ch);

	while (last != std::string::npos) {
		result.push_back(str.substr(first, last - first));
		first = last + 1;
		last = str.find(ch, first);
	}

	result.push_back(str.substr(first, std::min(last, str.size()) - first + 1));

	return result;
}

static ImVec2 calc_buttons_size(const std::vector<std::string>& button_names)
{
	auto& style = ImGui::GetStyle();

	auto buttons_size = ImVec2(0.f, 0.f);
	for (const auto& name : button_names) {
		auto text_size = ImGui::CalcTextSize(name.c_str(), nullptr, true);
		auto size = ImVec2(text_size.x + 2.f * style.FramePadding.x, text_size.y + 2.f * style.FramePadding.y);

		size = ImGui::CalcItemSize(ImVec2(100, 40), size.x, size.y);

		buttons_size.x += size.x + style.ItemSpacing.x;
		buttons_size.y = size.y;
	}
	buttons_size.x -= style.ItemSpacing.x;

	return buttons_size;
}

MessageBox::MessageBox() :
	Dialog("", false)
{}

std::string MessageBox::showDialog() const
{
	Dialog::switchContext();

	std::string result = "Error";

	auto button_names  = split_str(buttons, ';');
	auto buttons_size  = calc_buttons_size(button_names);

	ImVec2 text_size = ImGui::CalcTextSize(content.c_str(), nullptr, false, 0.f);
	ImVec2 required_size;
	required_size.x = 500.f;
	required_size.x = std::max(required_size.x, 100.f + buttons_size.x);
	required_size.x = std::max(required_size.x, text_size.x + 80.f);
	required_size.y = std::max(150.f, text_size.y + buttons_size.y + 90.f);

	titlebar.setButtonStyle(false, false, true);
	Dialog::beginShowDialog(required_size);

	while (window.isVisible()) {
		if (!Dialog::updateDialog()) {
			result = "Close";
			break;
		}

		Dialog::beginDialogWindow("MessageBox", nullptr, flags);

		ImGui::SetCursorPos(ImVec2(20, 20));
		ImGui::TextUnformatted(content.c_str());

		ImGui::SetCursorPosX(client_size.x - buttons_size.x - 10);
		ImGui::SetCursorPosY(client_size.y - buttons_size.y - 10);
		for (const auto& name : button_names) {
			if (ImGui::Button(name.c_str(), BUTTON_SIZE)) {
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

ProjectCloseDialog::ProjectCloseDialog() :
	Dialog("Closing project", false)
{}

std::string ProjectCloseDialog::showDialog() const
{
	Dialog::switchContext();

	std::vector<std::string> unsaved;
	std::string result = "Error";

	auto& main_window = MainWindow::get();

	if (!main_window.project_saved) {
		if (main_window.project_name.empty())
			unsaved.emplace_back("*(project file name)" PROJECT_EXT);
		else	
			unsaved.emplace_back("*" + main_window.project_name + PROJECT_EXT);
	}

	for (auto& sheet : main_window.sheets)
		if (!sheet->file_saved)
			unsaved.emplace_back("*" + sheet->name + SCHEMATIC_SHEET_EXT);

	auto text_height   = unsaved.size() * ImGui::GetFrameHeight();
	auto child_size    = ImVec2(550, std::clamp(text_height + 20.f, 200.f, 400.f));
	auto required_size = child_size;
	required_size.x   += 50.f;
	required_size.y   += 180.f;

	titlebar.setButtonStyle(false, false, true);
	Dialog::beginShowDialog(required_size);

	while (window.isVisible()) {
		if (!Dialog::updateDialog()) {
			result = "Close";
			break;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 20));
		Dialog::beginDialogWindow("ProjectClose", nullptr, flags);

		ImGui::TextUnformatted("Save changes to the following item(s)?");
		
		ImGui::Separator();
		
		ImGui::BeginChild("ProjectCloseChild", child_size);
		for (const auto& names : unsaved)
			ImGui::TextUnformatted(names.c_str());
		ImGui::EndChild();

		ImGui::Separator();

		ImGui::SetCursorPosX(client_size.x - 3 * (BUTTON_WIDTH + 20) - 26);
		ImGui::SetCursorPosY(client_size.y - BUTTON_HEIGHT - 15);
		if (ImGui::Button("Save", BUTTON_SIZE)) {
			result = "Save";
			window.setVisible(false);
		}
		ImGui::SameLine();
		if (ImGui::Button("Don't Save", ImVec2(0.f, BUTTON_HEIGHT))) {
			result = "Don't Save";
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
	}

	Dialog::endShowDialog();
	return result;
}

ProjectSaveDialog::ProjectSaveDialog() :
	Dialog("", true),
	create_subdirectory(true),
	save_as(false)
{}

std::string ProjectSaveDialog::showDialog() const
{
	Dialog::switchContext();


	project_dir.resize(256);
	project_name.resize(256);
	project_location.resize(256);

	project_location    = GetSystemFolderPath(SystemFolders::Desktop);
	project_name        = get_untitled_name(project_location);
	result              = "Error";
	create_subdirectory = true;

	updateProjectDir();

	titlebar.setButtonStyle(false, false, true);
	window.setTitle(save_as ? "Save project as..." : "Save project");
	Dialog::beginShowDialog(ImVec2(1000, 400));

	while (window.isVisible()) {
		dialogLoop();
	}

	Dialog::endShowDialog();

	return result;
}

void ProjectSaveDialog::dialogLoop() const
{
	static auto color_warning = ImVec4(0.8f, 0.73f, 0.16f, 1.f);
	static auto color_error   = ImVec4(0.96f, 0.34f, 0.38f, 1.f);

	if (!Dialog::updateDialog()) {
		result = "Close";
		return;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 20));
	Dialog::beginDialogWindow("Save Project", nullptr, flags);

	ImGui::TextUnformatted("Project name:    ");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(client_size.x - ImGui::GetCursorPosX() - 40);
	if (ImGui::InputText("##ProjectName", &project_name))
		updateProjectDir();

	ImGui::TextUnformatted("Project location:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(client_size.x - ImGui::GetCursorPosX() - 98);
	if (ImGui::InputText("##ProjectDir", &project_location))
		updateProjectDir();
	ImGui::SameLine();
	if (ImGui::Button("...")) {
		vk2d::FolderOpenDialog dialog;

		auto default_dir  = to_wide(project_location);

		dialog.title        = L"Choose project directory";
		dialog.default_dir  = default_dir.c_str();
		dialog.owner        = &window;
		
		if (dialog.showDialog() == vk2d::DialogResult::OK)
			project_location = to_unicode(dialog.getResultDir());
		
		updateProjectDir();
	}
	
	if (ImGui::Checkbox("Create project subdirectory", &create_subdirectory))
		updateProjectDir();

	ImGui::Text("project will be created at: \n%s", project_dir.c_str());

	if (fs::exists(project_dir))
		ImGui::TextColored(color_warning, "warning: directory is not empty");

	ImGui::SetCursorPosX(client_size.x - 2 * (BUTTON_WIDTH + 20) + 10);
	ImGui::SetCursorPosY(client_size.y - BUTTON_HEIGHT - 10);
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
}

void ProjectSaveDialog::updateProjectDir() const
{
	if (create_subdirectory)
		project_dir = project_location + "\\" + project_name;
	else
		project_dir = project_location;
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