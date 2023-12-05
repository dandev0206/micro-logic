#include "main_window.h"

#define ICON_TEXTURE_VAR textures[TEXTURE_ICONS_IDX]

#include <vk2d/graphics/texture_view.h>
#include <vk2d/system/clipboard.h>
#include <vk2d/system/file_dialog.h>
#include <imgui_internal.h>
#include <tinyxml2.h>
#include <filesystem>
#include <fstream>
#include <random>
#include <locale>
#include <iomanip>
#include "gui/platform/platform_impl.h"
#include "gui/platform/system_folder.h"
#include "gui/imgui_impl_vk2d.h"
#include "gui/imgui_custom.h"
#include "util/convert_string.h"
#include "dialogs.h"
#include "circuit_element_loader.h"
#include "base64.h"
#include "icons.h"
#include "micro_logic_config.h"

#ifdef _WIN32 
#include "gui/platform/platform_impl.h"
#endif

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)
#define IMGUI_NO_LABEL "##" STRINGIZE(__COUNTER__)

using vk2d::DialogResult;
namespace fs = std::filesystem;

MainWindow* MainWindow::main_window = nullptr;

char int_to_hex_char(int val) {
	if (0 <= val && val < 10)
		return val + '0';
	if (10 <= val && val < 16)
		return val - 10 + 'A';
	return ' ';
}

static std::string create_guid() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution dist(0, 15);

	std::string str;
	str.reserve(47);

	auto push_chars = [&](int count) {
		while (count-- > 0)
			str += int_to_hex_char(dist(gen));
	};

	str += "[";
	push_chars(8);
	str += '-';
	push_chars(4);
	str += '-';
	push_chars(4);
	str += '-';
	push_chars(4);
	str += '-';
	push_chars(12);
	str += "]";

	return str;
}

bool is_sub_path(const fs::path& base, const fs::path& destination)
{
	auto relative = fs::relative(destination, base).generic_string();

	if (relative.empty()) 
		return false;
	if (relative.size() == 1) 
		return true;
	if (relative[0] != '.' && relative[1] != '.') 
		return true;
	return false;
}

MainWindow::MainWindow() :
	initialized(false)
{
	assert(!main_window);
	main_window = this;

	std::setlocale(LC_ALL, "");

	window.create(1280, 800, "micro logic", vk2d::Window::Resizable);
	window.enableVSync(false);
	titlebar.bindWindow(window);

	{ // load font
		font.loadFromFile(FONT_PATH);
	}
	{ // load side menus
		side_menus.emplace_back(std::make_unique<Menu_Info>());
		side_menus.emplace_back(std::make_unique<Menu_Zoom>());
		side_menus.emplace_back(std::make_unique<Menu_Select>());
		side_menus.emplace_back(std::make_unique<Menu_Library>());
		side_menus.emplace_back(std::make_unique<Menu_Unit>());
		side_menus.emplace_back(std::make_unique<Menu_Shapes>());
		side_menus.emplace_back(std::make_unique<Menu_Label>());
		side_menus.emplace_back(std::make_unique<Menu_Copy>());
		side_menus.emplace_back(std::make_unique<Menu_Cut>());
		side_menus.emplace_back(std::make_unique<Menu_Delete>());
		side_menus.emplace_back(std::make_unique<Menu_Wire>());
		side_menus.emplace_back(std::make_unique<Menu_Net>());
		side_menus.emplace_back(std::make_unique<Menu_SplitWire>());
	}
	{ // load textures
		vk2d::Image icon_image;
		icon_image.loadFromFile(RESOURCE_DIR_NAME"icons.png");

		textures.emplace_back(icon_image);
		textures.emplace_back(font.getTexture(100));
	}
	{ // load circuit elements
		CircuitElementLoader loader;

		loader.load(RESOURCE_DIR_NAME"elements.xml", font);

		logic_gates.swap(loader.logic_gates);
		logic_units.swap(loader.logic_units);
		for (auto& data : loader.texture_datas) {
			textures.emplace_back(data.texture.release());
			textures.emplace_back(std::move(data.texture_mask));
		}
	}
	{
		window_library.bindMenuLibrary(dynamic_cast<Menu_Library&>(*side_menus[3]));
	}

	resize_tab_hovered = false;
	want_open_project  = false;
	want_close_window  = false;

	initializeProject();
	ImGui::LoadIniSettingsFromDisk(RESOURCE_DIR_NAME"default.ini");

	ResizingLoop::setFrameLimit(60);
	ResizingLoop::initResizingLoop(window);
	Dialog::pushImpls(3);
	window.setVisible(true);
}

MainWindow::~MainWindow()
{
	Dialog::destroyImpl();
	main_window = nullptr;
}

void MainWindow::show()
{
	while (true) {
		vk2d::Event e;
		while (window.pollEvent(e))
			eventProc(e);

		if (want_open_project) {

			if (new_project_path.empty()) {
				closeProjectImpl();
				initializeProject();
				ImGui::LoadIniSettingsFromDisk(RESOURCE_DIR_NAME"default.ini");
			} else
				openProjectImpl(new_project_path);

			want_open_project = false;
			new_project_path  = "";
		}

		if (want_close_window) {
			closeProjectImpl();
			window.close();
			break;
		}

		ImGui::VK2D::ProcessViewportEvent(window);

		loop();
	}
}

void MainWindow::loop() {
	ResizingLoop::loop();

	ImGui::VK2D::Update(window, delta_time);

	showMainMenus();
	showStatusBar();
	showUpperMenus();
	showSideMenus();

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::SetNextWindowBgAlpha(0.f);
	{ // setup docks
		auto flags = ImGuiDockNodeFlags_PassthruCentralNode;
		ImGui::DockSpaceOverViewport(nullptr, flags);
	}
	ImGui::PopStyleColor();

	auto& platform_io = ImGui::GetPlatformIO();

	if (curr_menu && curr_window_sheet)
		if (getCurrentWindowSheet().window)
			getCurrentSideMenu().loop();

	closeUnvisibleWindowSheet();
	for (auto& ws : window_sheets) {
		ws->showUI();
		ws->draw();
	}

	if (curr_window_sheet)
		getCurrentWindowSheet().clearHoverList();

	window_library.showUI();
	window_history.showUI();
	window_explorer.showUI();

	if (settings.debug.show_imgui_demo)
		ImGui::ShowDemoWindow(&settings.debug.show_imgui_demo);
	
	if (settings.debug.show_fps)
		showFPS();

	auto& io = ImGui::GetIO();

	ImGui::Render();
	ImGui::VK2D::Render(ImGui::GetDrawData());

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::VK2D::RenderViewports(window);
	}

	//vk2d::TextureView view(textures[3]);
	//view.setPosition(100, 100);
	//window.draw(view);

	ImGui::VK2D::DisplayViewports(window, true);

	if (io.WantSaveIniSettings && isProjectOpened())
		saveProject();
}

void MainWindow::eventProc(const vk2d::Event& e)
{
	ImGui::VK2D::ProcessEvent(window, e);

	using vk2d::Event;
	using vk2d::Key;

	switch (e.type) {
	case Event::KeyPressed: {
		bool ctrl = vk2d::Keyboard::isKeyPressed(Key::LControl);
		bool shft = vk2d::Keyboard::isKeyPressed(Key::LShift);
		bool alt  = vk2d::Keyboard::isKeyPressed(Key::LAlt);

		switch (e.keyboard.key) {
		case Key::X:
			if (ctrl && !shft && !alt && curr_window_sheet)
				curr_window_sheet->copySelectedToClipboard();
			break;
		case Key::C:
			if (ctrl && !shft && !alt && curr_window_sheet)
				curr_window_sheet->cutSelectedToClipboard();
			break;
		case Key::V:
			if (ctrl && !shft && !alt && curr_window_sheet)
				beginClipboardPaste();
			break;
		case Key::S:
			if (ctrl && shft && !alt && shft)
				saveAll();
			else if (ctrl && !shft && !alt && curr_window_sheet)
				saveSchematicSheet(*curr_window_sheet->sheet);
			break;
		case Key::Z:
			if (ctrl && !shft && !alt && isUndoable())
				undo();
			break;
		case Key::Y:
			if (ctrl && !shft && !alt && isRedoable())
				redo();
			break;
		}
	} break;
	case Event::Close:
		closeWindow();
		break;
	case Event::Resize:
		break;
	}

	if (curr_menu && curr_window_sheet)
		getCurrentSideMenu().eventProc(e, delta_time);
}

bool MainWindow::closeWindow()
{
	if (!closeProject()) 
		return false;

	want_close_window = true;

	return true;
}

void MainWindow::initializeProject()
{
	assert(!initialized);

	project_dir          = "";
	project_name         = "(empty project)";
	curr_menu            = nullptr;
	curr_menu_hover      = nullptr;
	curr_window_sheet    = nullptr;
	project_opened       = false;
	last_time            = clock_t::now();

	{ // init settings
		settings.view.grid               = true;
		settings.view.grid_axis          = true;
		settings.view.grid_style         = GridStyle::line;
		settings.view.grid_width         = 1;
		settings.view.grid_axis_width    = 4;
		settings.view.grid_color         = Color(1.f, 1.f, 1.f, 0.5f);
		settings.view.grid_axis_color    = Color(1.f, 1.f, 1.f, 0.7f);
		settings.view.library_window     = true;

		settings.rendering.enable_vsync = false;
		settings.rendering.frame_limit  = true;
		settings.rendering.max_fps      = 60;

		settings.debug.show_bvh        = false;
		settings.debug.show_chunks     = false;
		settings.debug.show_fps        = false;
		settings.debug.show_imgui_demo = false;

		settings.curr_settings_menu = 0;
	}
	{ // init ImGui
		ImGui::VK2D::Init(window, false);

		auto& io    = ImGui::GetIO();
		auto& style = ImGui::GetStyle();

		style.Colors[ImGuiCol_WindowBg]           = ImVec4(0.f, 0.f, 0.f, 1.f);
		style.Colors[ImGuiCol_PopupBg]            = ImVec4(0.08f, 0.08f, 0.08f, 1.f);
		style.Colors[ImGuiCol_Tab]                = ImVec4(0.20f, 0.20f, 0.20f, 1.f);
		style.Colors[ImGuiCol_TabHovered]         = ImVec4(0.31f, 0.31f, 0.31f, 1.f);
		style.Colors[ImGuiCol_TabActive]          = ImVec4(0.31f, 0.31f, 0.31f, 1.f);
		style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.06f, 0.1f, 0.15f, 1.f);
		style.Colors[ImGuiCol_MenuBarBg]          = ImVec4(0.14f, 0.14f, 0.14f, 1.f);
		style.Colors[ImGuiCol_Button]             = ImVec4(0.f, 0.f, 0.f, 0.f);
		style.Colors[ImGuiCol_ButtonHovered]      = ImVec4(0.5f, 0.5f, 0.5f, 1.f);
		style.Colors[ImGuiCol_ButtonActive]       = ImVec4(1.f, 1.f, 1.f, 1.f);
		style.Colors[ImGuiCol_HeaderActive]       = ImVec4(1.f, 1.f, 1.f, 0.3f);
		style.Colors[ImGuiCol_HeaderHovered]      = ImVec4(1.f, 1.f, 1.f, 0.2f);
		style.Colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.2f, 0.2f, 0.8f, 1.f);
		style.Colors[ImGuiCol_SliderGrab]         = ImVec4(1.f, 1.f, 1.f, 1.f);

		io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/Arial.ttf", FONT_SIZE);
		ImGui::VK2D::UpdateFontTexture(window);

		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		io.ConfigViewportsNoAutoMerge      = true;
		io.ConfigDockingTransparentPayload = true;
		io.IniFilename                     = nullptr;
	}

	initialized = true;
}

bool MainWindow::openProject()
{
	if (!closeProject()) return false;

	vk2d::FileOpenDialog dialog;

	auto default_dir = to_wide(GetSystemFolderPath(SystemFolders::Desktop));

	dialog.owner        = &window;
	dialog.title        = L"Open project";
	dialog.default_dir  = default_dir.c_str();
	dialog.default_name = L"";
	dialog.filters.emplace_back(L"Project file", L"mlp");

	if (dialog.showDialog() != vk2d::DialogResult::OK) return false;

	auto path = std::filesystem::path(dialog.getResultPaths().front());
	auto dir  = path.parent_path().generic_string();
	auto name = path.filename().generic_string();

	assert(path.extension() == PROJECT_EXTW);

	if (dir != project_dir || name != project_name) {
		want_open_project = true;
		new_project_path  = path.generic_string();
	}

	return true;
}

bool MainWindow::saveAll()
{
	if (!saveProject()) return false;

	for (auto& sheet : sheets)
		if (!sheet->is_up_to_date)
			if (!saveSchematicSheet(*sheet))
				return false;

	postInfoMessage("Item(s) saved");

	return true;
}

bool MainWindow::saveProjectAs()
{
	ProjectSaveDialog dialog;
	dialog.owner   = &window;
	dialog.save_as = true;

	if (dialog.showDialog() == "Save") {
		project_dir    = dialog.project_dir;
		project_name   = dialog.project_name;
		project_opened = true;
	} else {
		return false;
	}

	return saveAll();
}

bool MainWindow::saveProject()
{
	if (!isProjectOpened()) {
		ProjectSaveDialog dialog;
		dialog.owner   = &window;
		dialog.save_as = false;

		if (dialog.showDialog() == "Save") {
			project_dir    = dialog.project_dir;
			project_name   = dialog.project_name;
			project_opened = true;
		} else {
			return false;
		}
	}

	tinyxml2::XMLDocument doc;

	auto* root = doc.NewElement("Project");
	doc.InsertFirstChild(root);

	for (const auto& sheet : sheets) {
		auto* elem = root->InsertNewChildElement("SchematicSheet");

		elem->SetAttribute("name", sheet->name.c_str());
		elem->SetAttribute("path", sheet->path.c_str());
		elem->SetAttribute("guid", sheet->guid.c_str());
	}
	{
		auto* elem = root->InsertNewChildElement("ImGui");

		auto encoded = Base64::encode(ImGui::SaveIniSettingsToMemory());
		elem->SetText(encoded.c_str());
	}
	
	auto res = doc.SaveFile((project_dir + "\\" + project_name + PROJECT_EXT).c_str());

	if (res != tinyxml2::XMLError::XML_SUCCESS) {
		MessageBox msg_box;
		msg_box.owner   = &window;
		msg_box.title   = "Error";
		msg_box.content = "Error occured while saving project '" + project_name + PROJECT_EXT"'";
		msg_box.icon    = icon_to_texture_view(ICON_ERROR_BIG);

		msg_box.showDialog();
		return false;
	}

	return true;
}

bool MainWindow::closeProject()
{
	assert(initialized);

	if (isProjectEmpty()) {
		return true;
	} else if (hasUnsavedSchematicSheet()) {
		ListMessageBox dialog;
		dialog.owner   = &window;
		dialog.title   = "Closing project '" + project_name + "'";
		dialog.content = "Save changes to the following item(s)?";
		dialog.buttons = "Save;Don't Save;Cancel";

		for (auto& sheet : sheets)
			if (!sheet->is_up_to_date)
				dialog.list.emplace_back("*" + sheet->name + SCHEMATIC_SHEET_EXT);

		auto result = dialog.showDialog();
		if (result == "Save") {
			return saveAll();
		} else if (result == "Don't Save") {
			return true;
		}
	} else {
		return true;
	}

	return false;
}

bool MainWindow::openProjectImpl(const std::string& project_path)
{
	std::string                      imgui_ini;
	std::vector<SchematicSheetPtr_t> new_sheets;

	auto path             = std::filesystem::path(project_path);
	auto new_project_dir  = path.parent_path().generic_string();
	auto new_project_name = path.filename().replace_extension().generic_string();

	tinyxml2::XMLDocument doc;
	doc.LoadFile(project_path.c_str());

	auto* root = doc.FirstChildElement("Project");

	{
		auto* elem = root->FirstChildElement("SchematicSheet");
		for (; elem; elem = elem->NextSiblingElement("SchematicSheet")) {
			std::string file_path = elem->Attribute("path");
			std::string full_path = new_project_dir + "\\" + file_path;	

			SchematicSheetPtr_t sheet;
			if (!openSchematicSheetImpl(sheet, full_path)) return false;

			if (elem->Attribute("guid") != sheet->guid) {
				MessageBox msg_box;
				msg_box.owner   = &window;
				msg_box.title   = "Error";
				msg_box.content = "guid of sheet '" + sheet->name + "' is different";
				msg_box.icon    = icon_to_texture_view(ICON_ERROR_BIG);

				msg_box.showDialog();
				return false;
			}

			new_sheets.emplace_back(std::move(sheet));
		}
	}
	{
		auto* elem = root->FirstChildElement("ImGui");
		
		if (elem->GetText() == nullptr) {
			MessageBox msg_box;
			msg_box.owner   = &window;
			msg_box.title   = "Error";
			msg_box.content = "cannot load imgui ini settings";
			msg_box.icon    = icon_to_texture_view(ICON_ERROR_BIG);

			msg_box.showDialog();
			return false;
		}

		imgui_ini = Base64::decode(elem->GetText());
	}

	closeProjectImpl();
	initializeProject();
	ImGui::LoadIniSettingsFromMemory(imgui_ini.c_str());

	project_dir    = new_project_dir;
	project_name   = new_project_name;
	project_opened = true;
	sheets.swap(new_sheets);

	postInfoMessage("Project " + project_name + " Opened");
	return true;
}

bool MainWindow::closeProjectImpl()
{
	assert(initialized);

	ImGui::VK2D::ShutDown(window);

	project_dir  = "";
	project_name = "";
	sheets.clear();
	window_sheets.clear();

	initialized = false;

	return true;
}

bool MainWindow::isProjectOpened() const
{
	return project_opened;
}

SideMenu& MainWindow::getCurrentSideMenu()
{
	return *curr_menu;
}

bool MainWindow::isProjectEmpty() const
{
	if (project_opened) return false;
	if (!sheets.empty()) return false;
	return true;
}

void MainWindow::setCurrentSideMenu(SideMenu* menu)
{
	if (curr_menu == menu) return;

	if (curr_window_sheet && curr_menu)
		curr_menu->onClose();

	curr_menu = menu;

	if (curr_window_sheet && curr_menu)
		curr_menu->onBegin();
}

bool MainWindow::addSchematicSheet()
{
	SchematicSheetNameDialog dialog;
	dialog.owner      = &window;
	dialog.title      = "Add schematic sheet";
	dialog.parent_dir = project_name;
	dialog.buttons    = "Add;Cancel";

	int32_t num = 0;
	do {
		dialog.sheet_name = "Sheet" + std::to_string(num++);
		dialog.sheet_path = dialog.sheet_name + SCHEMATIC_SHEET_EXT;

	} while (findSchematicSheetByPath(dialog.sheet_path));

	dialog.sheet_path = dialog.sheet_name;

	if (dialog.showDialog() != "Add") return false;

	auto sheet  = std::make_unique<SchematicSheet>();
	sheet->guid = create_guid();
	sheet->name = dialog.sheet_name;
	sheet->path = dialog.sheet_path;

	updateThumbnail(*sheet);

	if (isProjectOpened()) {
		if (!saveSchematicSheet(*sheet)) return false;
		if (!saveProject()) return false;
	}

	sheets.emplace_back(std::move(sheet));

	return true;
}

bool MainWindow::saveSchematicSheet(SchematicSheet& sheet)
{
	if (!isProjectOpened())
		if (!saveProject())
			return false;

	for (auto& ws : window_sheets) {
		if (ws->sheet == &sheet) {
			ws->sheetSaved();
			break;
		}
	}

	if (!saveSchematicSheetImpl(sheet, project_dir + "\\" + sheet.path)) return false;

	sheet.file_saved    = true;
	sheet.is_up_to_date = true;

	postInfoMessage("Sheet '" + sheet.name + "' Saved");

	return true;
}

bool MainWindow::importSchematicSheet()
{
	auto default_dir = to_wide(project_dir);

	vk2d::FileOpenDialog dialog;
	dialog.owner            = &window;
	dialog.title            = L"Import schematic sheet";
	dialog.default_dir      = default_dir.c_str();
	dialog.default_name     = L"";
	dialog.multi_selectable = true;
	dialog.filters.emplace_back(L"Schematic sheet file", L"mls");

	if (dialog.showDialog() != vk2d::DialogResult::OK) return false;

	auto paths = dialog.getResultPaths();

	if (isProjectOpened()) {
		std::vector<std::string> out_of;

		for (const auto* path_str : paths) {
			if (!is_sub_path(project_dir, path_str)) {
				out_of.emplace_back(to_unicode(path_str));
			}
		}

		if (!out_of.empty()) {
			ListMessageBox msg_box;
			msg_box.owner   = &window; 
			msg_box.title   = "Warning";
			msg_box.content = "Following items are out of project sub directory.";
			msg_box.buttons = "Ok;Cancel";
			msg_box.icon    = icon_to_texture_view(ICON_WARNING_BIG);
			msg_box.list.swap(out_of);

			if (msg_box.showDialog() != "Ok") return false;
		}
	}

	for (const auto* path_str : paths) {
		SchematicSheetPtr_t sheet;
		
		auto path = to_unicode(path_str);

		if (!openSchematicSheetImpl(sheet, path)) return false;

		sheet->file_saved    = is_sub_path(project_dir, path_str);
		sheet->is_up_to_date = sheet->file_saved;

		if (findSchematicSheetByPath(sheet->path)) {
			SchematicSheetNameDialog dialog;
			dialog.owner        = &window;
			dialog.title        = "Rename schematic sheet";
			dialog.content      = "'" + sheet->path + "' already exists.";
			dialog.sheet_path   = sheet->path;
		}

		sheets.emplace_back(std::move(sheet));
	}

	return true;
}

bool MainWindow::exportSchematicSheet(const SchematicSheet& sheet)
{
	auto default_dir = to_wide(project_dir);

	vk2d::FileSaveDialog dialog;
	dialog.owner        = &window;
	dialog.title        = L"Export schematic sheet";
	dialog.default_dir  = default_dir.c_str();
	dialog.default_name = L"";
	dialog.filters.emplace_back(L"Schematic sheet file", L"mls");

	if (dialog.showDialog() != vk2d::DialogResult::OK) return false;

	auto path = std::filesystem::path(dialog.getResultPath())
		.replace_extension(SCHEMATIC_SHEET_EXTW);

	return saveSchematicSheetImpl(sheet, path.generic_string());
}

bool MainWindow::openSchematicSheetImpl(SchematicSheetPtr_t& sheet, const std::string& path)
{
	std::ifstream file(path);

	if (!file.is_open()) {
		MessageBox msg_box;
		msg_box.owner   = &window;
		msg_box.title   = "Error";
		msg_box.content = "cannot locate '" + path + "'.";
		msg_box.icon    = icon_to_texture_view(ICON_ERROR_BIG);

		msg_box.showDialog();
		return false;
	}

	sheet = std::make_unique<SchematicSheet>();
	sheet->unserialize(file);
	sheet->path          = path;
	sheet->file_saved    = true;
	sheet->is_up_to_date = true;
	updateThumbnail(*sheet);

	return true;
}

bool MainWindow::saveSchematicSheetImpl(const SchematicSheet& sheet, const std::string& path)
{
	std::ofstream of(path);

	if (!of.is_open()) {
		MessageBox msg_box;
		msg_box.owner   = &window;
		msg_box.title   = "Error";
		msg_box.content = "Failed to save schematic sheet";
		msg_box.icon    = icon_to_texture_view(ICON_ERROR_BIG);
		return false;
	}

	sheet.serialize(of);

	return true;
}

bool MainWindow::deleteSchematicSheet(SchematicSheet& sheet)
{
	std::string result;

	if (sheet.file_saved) {
		MessageBox dialog;
		dialog.owner    = &window;
		dialog.title    = "Deleting schematic sheet";
		dialog.buttons  = "Remove;Delete;Cancel";
		dialog.content += "Choose Remove to remove '" + sheet.name + SCHEMATIC_SHEET_EXT"' from '" + project_name + "'.\n";
		dialog.content += "Choose Delete to permanently delete '" + sheet.name + SCHEMATIC_SHEET_EXT"'.";

		result = dialog.showDialog();

		if (result != "Remove" || result != "Delete") return false;
	}
	else if (!sheet.empty()) {
		MessageBox dialog;
		dialog.owner   = &window;
		dialog.title   = "Deleting schematic sheet";
		dialog.buttons = "Remove;Cancel";
		dialog.content = "Do you want to remove'" + sheet.name + "' from '" + project_name + "'?";

		result = dialog.showDialog();

		if (result != "Remove") return false;
	}

	if (result == "Delete")
		std::filesystem::remove(sheet.path);

	for (auto iter = sheets.begin(); iter != sheets.end(); ++iter) {
		if (iter->get() == &sheet) {
			sheets.erase(iter);
			break;
		}
	}

	return true;
}

bool MainWindow::hasUnsavedSchematicSheet() const
{
	for (auto& sheet : sheets)
		if (!sheet->is_up_to_date)
			return true;
	return false;
}

SchematicSheet* MainWindow::findSchematicSheetByPath(const std::string& path)
{
	for (auto& sheet : sheets)
		if (sheet->path == path)
			return sheet.get();
	return nullptr;
}

void MainWindow::updateThumbnail(SchematicSheet& sheet)
{
	vk2d::DrawList draw_list;

	vk2d::RenderTexture texture(720, 480);
	
	if (!sheet.bvh.empty()) {
		AABB aabb = sheet.bvh.begin()->second->getAABB();

		for (auto& [elem_aabb, elem] : sheet.bvh)
			aabb = aabb.union_of(elem_aabb);

		auto scale_x = texture.size().x / aabb.width();
		auto scale_y = texture.size().y / aabb.height();

		auto position        = aabb.center();
		auto grid_pixel_size = 0.9f * std::min(scale_x, scale_y);
		auto transform       = vk2d::Transform().
			translate(texture.size() / 2.f - position * grid_pixel_size).
			scale(grid_pixel_size);

		size_t cmd_size = textures.size() + 3;
		for (size_t i = 0; i < cmd_size; ++i) {
			auto& cmd = draw_list.commands.emplace_back();

			cmd.options.transform = transform;

			if (2 <= i && i < 2 + textures.size())
				cmd.options.texture = &textures[i - 2];
		}

		for (const auto& [aabb, elem] : sheet.bvh)
			elem->draw(draw_list);

		texture.draw(draw_list);
	}

	texture.display();

	sheet.thumbnail = texture.release();
}

bool MainWindow::openWindowSheet(SchematicSheet& sheet)
{
	for (auto& ws : window_sheets) {
		if (ws->sheet == &sheet) {
			auto* window = ImGui::FindWindowByName(ws->window_name.c_str());
			window->DockNode->TabBar->NextSelectedTabId = window->ID;;
			return true;
		}
	}

	auto& ws = window_sheets.emplace_back(std::make_unique<Window_Sheet>());
	ws->bindSchematicSheet(sheet);

	auto& dc = ImGui::GetCurrentContext()->DockContext;
	for (int i = 0; i < dc.Nodes.Data.Size; ++i) {
		if (auto* node = (ImGuiDockNode*)dc.Nodes.Data[i].val_p) {
			ImGuiID id = ImGui::DockBuilderGetCentralNode(node->ID)->ID;
			ImGui::DockBuilderDockWindow(ws->window_name.c_str(), id);
			return true;
		}
	}

	return false;
}

void MainWindow::closeUnvisibleWindowSheet()
{
	auto iter = window_sheets.begin();
	for (; iter != window_sheets.end(); ++iter) {
		if (!(*iter)->show) {
			if (curr_window_sheet == iter->get())
				curr_window_sheet = nullptr;

			window_sheets.erase(iter);
			
			return;
		}
	}
}

Window_Sheet& MainWindow::getCurrentWindowSheet()
{
	return *curr_window_sheet;
}

void MainWindow::setCurrentWindowSheet(Window_Sheet* window_sheet)
{
	if (curr_window_sheet == window_sheet) return;

	if (curr_window_sheet && curr_menu)
		curr_menu->onClose();

	curr_window_sheet = window_sheet;

	if (curr_window_sheet && curr_menu)
		curr_menu->onBegin();
}

Window_Sheet* MainWindow::findWindowSheet(const SchematicSheet& sheet)
{
	for (const auto& ws : window_sheets)
		if (ws->sheet == &sheet)
			return ws.get();
	return nullptr;
}

void MainWindow::postStatusMessage(const std::string& str)
{
	status_message = str;
}

void MainWindow::postInfoMessage(const std::string& str, bool force)
{
	if (!force && !infoMessageTimeout()) return;

	info_message_last_time = clock_t::now();
	info_message           = str;
}

bool MainWindow::infoMessageTimeout() const
{
	using namespace std::chrono;

	auto time = duration_cast<milliseconds>(clock_t::now() - info_message_last_time).count();
	return time > 3000;
}

void MainWindow::redo()
{
	getCurrentWindowSheet().redo();
}

void MainWindow::undo()
{
	getCurrentWindowSheet().undo();
}

bool MainWindow::isRedoable()
{
	if (!curr_window_sheet) return false;
	return getCurrentWindowSheet().isRedoable();
}

bool MainWindow::isUndoable()
{
	if (!curr_window_sheet) return false;
	return getCurrentWindowSheet().isUndoable();
}

void MainWindow::beginClipboardPaste()
{
	if (!vk2d::Clipboard::available()) return;

	std::stringstream ss(Base64::decode(vk2d::Clipboard::getString()));

	if (ss.str().size() < 38) return;

	std::string guid;
	guid.resize(39);

	ss.read(guid.data(), 38);

	if (guid == CLIPBOARD_COPY_IDENTIFICATION) {
		auto* menu = findSideMenu<Menu_Copy>();
		menu->beginClipboardPaste(ss);
	} else if (guid == CLIPBOARD_CUT_IDENTIFICATION) {
		auto* menu = findSideMenu<Menu_Cut>();
		menu->beginClipboardPaste(ss);
	}
}

void MainWindow::showMainMenus()
{
	auto spacing = ImGui::GetFrameHeight();
	
	if (titlebar.beginTitleBar()) {
		vec2 icon_size(spacing, spacing);
		spacing *= 1.5;

		if (ImGui::BeginMenu("File")) {
			ImGui::Image(ICON_WINDOW, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("New Window"))
				CreateNewProcess();

			ImGui::SetCursorPosX(spacing);
			if (ImGui::MenuItem("New Project")) {
				if (closeProject()) {
					want_open_project = true;
					new_project_path  = "";
				}
			}

			ImGui::SetCursorPosX(spacing);
			if (ImGui::BeginMenu("Recent...")) {
				ImGui::EndMenu();
			}

			ImGui::Image(ICON_FOLDER, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Open"))
				openProject();

			ImGui::Separator();

			ImGui::Image(ICON_SAVE, icon_size);
			ImGui::SameLine();
			if (project_opened) {
				if (ImGui::MenuItem("Save Project As...###MenuItemSaveProject"))
					saveProjectAs();
			} else {
				if (ImGui::MenuItem("Save Project###MenuItemSaveProject"))
					saveProject();
			}

			if (curr_window_sheet) {
				auto& ws = *curr_window_sheet;

				std::string save_str = "Save '";
				save_str += ws.sheet->name;
				save_str += "'###MenuItemSave";

				ImGui::Image(ICON_SAVE, icon_size);
				ImGui::SameLine();
				if (ImGui::MenuItem(save_str.c_str(), "Ctrl+S", false, !ws.sheet->is_up_to_date))
					saveSchematicSheet(*ws.sheet);
			} else {
				ImGui::Image(ICON_SAVE, icon_size, vk2d::Colors::Gray);
				ImGui::SameLine();
				ImGui::MenuItem("Save###MenuItemSave", "Ctrl+S", false, false);
			}

			ImGui::Image(ICON_SAVE_ALL, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Save All", "Ctrl+Shift+S"))
				saveAll();

			ImGui::Separator();

			ImGui::Image(ICON_IMPORT, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Import Sheet"))
				importSchematicSheet();

			ImGui::Image(ICON_EXPORT, icon_size);
			ImGui::SameLine();
			if (curr_window_sheet) {
				auto& sheet     = *curr_window_sheet->sheet;
				auto export_str = "Export '" + sheet.name + "'###MenuItemExport";

				if (ImGui::MenuItem(export_str.c_str()))
					exportSchematicSheet(sheet);
			} else {
				ImGui::MenuItem("Export###MenuItemExport", nullptr, nullptr, false);
			}

			ImGui::Separator();

			ImGui::Image(ICON_EXIT, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Exit", "Alt+F4")) {
				closeWindow();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit")) {
			auto undo_tint = isUndoable() ? vk2d::Colors::White : vk2d::Colors::Gray;
			ImGui::Image(ICON_UNDO, icon_size, undo_tint);
			ImGui::SameLine();
			if (ImGui::MenuItem("Undo", "Ctrl+Z", false, isUndoable())) undo();

			auto redo_tint = isRedoable() ? vk2d::Colors::White : vk2d::Colors::Gray;
			ImGui::Image(ICON_REDO, icon_size, redo_tint);
			ImGui::SameLine();
			if (ImGui::MenuItem("Redo", "Ctrl+Y", false, isRedoable())) redo();

			ImGui::Separator();

			bool has_ws    = curr_window_sheet;
			auto edit_tint = has_ws ? vk2d::Colors::White : vk2d::Colors::Gray;
			ImGui::Image(ICON_COPY, icon_size, edit_tint);
			ImGui::SameLine();
			if (ImGui::MenuItem("Copy", "Ctrl+C", false, has_ws))
				getCurrentWindowSheet().copySelectedToClipboard();

			ImGui::Image(ICON_CUT, icon_size, edit_tint);
			ImGui::SameLine();
			if (ImGui::MenuItem("Cut", "Ctrl+X", false, has_ws))
				getCurrentWindowSheet().cutSelectedToClipboard();

			ImGui::Image(ICON_PASTE, icon_size, edit_tint);
			ImGui::SameLine();
			if (ImGui::MenuItem("Paste", "Ctrl+V", false, has_ws))
				beginClipboardPaste();

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View")) {
			ImGui::Image(ICON_LIBRARY, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Library")) {
				window_library.show = true;
			}

			ImGui::Image(ICON_HISTORY, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("History")) {
				window_history.show = true;
			}

			ImGui::SetCursorPosX(spacing);
			if (ImGui::MenuItem("Browse")) {
				window_explorer.show = true;
			}

			ImGui::Separator();

			ImGui::SetCursorPosX(spacing);
			if (ImGui::MenuItem("Grid", nullptr, &settings.view.grid)) {
				for (auto& ws : window_sheets)
					ws->update_grid = true;
			}

			ImGui::Separator();
			
			ImGui::SetCursorPosX(spacing);
			if (ImGui::MenuItem("reset view", nullptr, nullptr, curr_window_sheet)) {
				auto& window_sheet = getCurrentWindowSheet();
				window_sheet.setPosition({ 0.f, 0.f });
				window_sheet.setScale(1.f);
			}

			ImGui::Separator();

			ImGui::SetCursorPosX(spacing);
			if (ImGui::BeginMenu("Debug")) {
				ImGui::SetCursorPosX(spacing);
				ImGui::MenuItem("BVH Hierarchy", nullptr, &settings.debug.show_bvh);
				ImGui::SetCursorPosX(spacing);
				ImGui::MenuItem("Chunks", nullptr, &settings.debug.show_chunks);
				ImGui::SetCursorPosX(spacing);
				ImGui::MenuItem("FPS", nullptr, &settings.debug.show_fps);
				ImGui::SetCursorPosX(spacing);
				ImGui::MenuItem("ImGui Demo", nullptr, &settings.debug.show_imgui_demo);
				
				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Simulation")) {
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Tools")) {
			ImGui::Image(ICON_GEAR, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Settings...")) {
				SettingsDialog dialog;
				dialog.owner = &window;

				dialog.showDialog();
			}
			ImGui::EndMenu();
		}

		ImGui::Separator();

		auto cursor_x = ImGui::GetCursorPosX();
		ImGui::TextUnformatted(project_name.c_str());

		Rect caption_rect;
		caption_rect.setPosition({ cursor_x, 0.f });
		caption_rect.setSize(to_vec2(ImGui::GetWindowSize()));
		caption_rect.width -= cursor_x + 10;

		titlebar.setCaptionRect(caption_rect);
		window.setMinSizeLimit({ ImGui::GetCursorPosX() + 220, 125 });
		titlebar.endTitleBar();
	}
}

void MainWindow::showUpperMenus()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
	
	ImGuiWindowFlags window_flags = 
		ImGuiWindowFlags_NoScrollbar | 
		ImGuiWindowFlags_NoSavedSettings | 
		ImGuiWindowFlags_NoNav | 
		ImGuiWindowFlags_MenuBar;
	
	float height = ImGui::GetFrameHeight();
	if (ImGui::BeginViewportSideBar("##UpperMenus", nullptr, ImGuiDir_Up, height, window_flags)) {
		if (ImGui::BeginMenuBar()) {
			vec2 icon_size(30, 30);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f ,0.f });

			auto undo_tint = isUndoable() ? vk2d::Colors::White : vk2d::Colors::Gray;
			if (ImGui::ImageButton(ICON_UNDO, icon_size, vk2d::Colors::Transparent, undo_tint)) undo();

			auto redo_tint = isRedoable() ? vk2d::Colors::White : vk2d::Colors::Gray;
			if (ImGui::ImageButton(ICON_REDO, icon_size, vk2d::Colors::Transparent, redo_tint)) redo();

			auto save_tint = curr_window_sheet ? vk2d::Colors::White : vk2d::Colors::Gray;
			if (ImGui::ImageButton(ICON_SAVE, icon_size, vk2d::Colors::Transparent, save_tint))
				if (curr_window_sheet)
					saveSchematicSheet(*curr_window_sheet->sheet);
			
			if (ImGui::ImageButton(ICON_SAVE_ALL, icon_size))
				saveAll();

			ImGui::PopStyleVar();

			ImGui::Separator();

			if (curr_window_sheet && curr_menu) 
				curr_menu->upperMenu();

			ImGui::EndMenuBar();
		}
		ImGui::End();
	}
	ImGui::PopStyleVar();
}

void MainWindow::showStatusBar()
{
	auto& io = ImGui::GetIO();

	ImGuiWindowFlags window_flags = 
		ImGuiWindowFlags_NoScrollbar | 
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_MenuBar;

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
	float height = ImGui::GetFrameHeight();
	if (ImGui::BeginViewportSideBar("##MainStatusBar", nullptr, ImGuiDir_Down, height, window_flags)) {
		if (ImGui::BeginMenuBar()) {
			ImGui::SetCursorPosX(height * 0.25f);
			ImGui::ImageButton(ICON_MESSAGE, { 23, 23 });

			if (!info_message.empty()) {
				ImGui::TextUnformatted(info_message.c_str());				
				if (infoMessageTimeout())
					info_message.clear();
			} else if (!status_message.empty()) {
				ImGui::TextUnformatted(status_message.c_str());
			} else if (curr_menu_hover) {
				ImGui::TextUnformatted(curr_menu_hover->menu_name);
			} else if (curr_window_sheet) {
				auto& ws = getCurrentWindowSheet();

				if (ws.capturing_mouse) {
					auto pos = ws.getCursorPlanePos();
					ImGui::Text("(%.1f, %.1f)", pos.x, pos.y);
				}
			}

			if (curr_window_sheet) {
				auto& ws = getCurrentWindowSheet();

				ImGui::SetCursorPosX(ImGui::GetWindowWidth() -  400);

				if (ImGui::ImageButton(ICON_MAG_MINUS, { 17, 17 }))
					ws.setScale(ws.sheet->scale / 1.05f);

				auto scale = ws.sheet->scale;
				ImGui::SetNextItemWidth(200);
				if (ImGui::ZoomSlider("##Zoom Slider", &scale, MIN_SCALE, MAX_SCALE))
					ws.setScale(scale);

				if (ImGui::ImageButton(ICON_MAG_PLUS, { 17, 17 }))
					ws.setScale(ws.sheet->scale * 1.05f);

				ImGui::Text(" -  %.1f%%", 100.f * scale);
			}

			if (!window.isMaximized()) {
				ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 35);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.14f, 0.14f, 1.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.14f, 0.14f, 0.14f, 1.f));
				ImGui::ImageButton(ICON_RESIZE_TAB, { 23, 23 });
				ImGui::PopStyleColor(2);
				resize_tab_hovered = ImGui::IsItemHovered();
			} else
				resize_tab_hovered = false;

			ImGui::EndMenuBar();
		}
		ImGui::End();
	}
	ImGui::PopStyleVar();
}

void MainWindow::showSideMenus()
{
	ImGuiWindowFlags window_flags =
		ImGuiWindowFlags_NoScrollbar | 
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoNav;

	if (ImGui::BeginViewportSideBar("##SideBar", nullptr, ImGuiDir_Left, 60, window_flags)) {
		curr_menu_hover = nullptr;

		for (auto& menu : side_menus) {
			menu->menuButton();

			if (ImGui::IsItemHovered())
				curr_menu_hover = menu.get();
		}

		ImGui::End();
	}
}

void MainWindow::showFPS()
{
	auto& io = ImGui::GetIO();
	ImGui::Begin("fps", &settings.debug.show_fps, ImGuiWindowFlags_NoDocking);
	ImGui::Text("dt : %.2fms", 1000.f * io.DeltaTime);
	ImGui::Text("fps: %d", (int)(1.f / io.DeltaTime));
	ImGui::End();
}
