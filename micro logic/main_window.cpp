#define ICON_TEXTURE_VAR textures[TEXTURE_ICONS_IDX]

#include "main_window.h"

#include <vk2d/graphics/texture_view.h>
#include <vk2d/system/clipboard.h>
#include <vk2d/system/file_dialog.h>
#include <imgui_internal.h>
#include <tinyxml2.h>
#include <thread>
#include <filesystem>
#include <fstream>
#include <random>
#include <locale>
#include <iomanip>
#include "gui/imgui_impl_vk2d.h"
#include "gui/imgui_custom.h"
#include "circuit_element_loader.h"
#include "base64.h"
#include "icons.h"
#include "micro_logic_config.h"

#ifdef _WIN32 
#include "gui/platform/platform_windows_impl.h"
#endif

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)
#define IMGUI_NO_LABEL "##" STRINGIZE(__COUNTER__)

using vk2d::DialogResult;

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

static std::string to_unicode(const std::wstring& wstr) {
	std::string str;
	size_t size;
	str.resize(wstr.length() * 2);
	auto res = wcstombs_s(&size, &str[0], str.size(), wstr.c_str(), _TRUNCATE);
	assert(!res);
	str.resize(size - 1);

	return str;
}

static std::wstring to_wide(const std::string& str) {
	std::wstring wstr;
	size_t size;
	wstr.resize(str.length() + 1);
	auto res = mbstowcs_s(&size, &wstr[0], wstr.size(), str.c_str(), _TRUNCATE);
	assert(!res);
	wstr.resize(size - 1);
	return wstr;
}

MainWindow::MainWindow() :
	initialized(false)
{
	assert(!main_window);
	main_window = this;

	std::setlocale(LC_ALL, "");

	window.create(1280, 800, "micro logic", vk2d::Window::Resizable);
	msg_box.init(&window);

#ifdef _WIN32 
	InjectTitleBar(window, titlebar);
	InjectTitleBar(msg_box.getWindow(), msg_box.getTitlebar());
	InjectWndProc(this);
#endif

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

	{
		// openProject("C:\\Users\\j0994\\OneDrive\\¹ÙÅÁ È­¸é\\Untitled0");
		//sheets.resize(2);
		//sheets[0].name = "Sheet0";
		//sheets[0].guid = create_guid();
		//sheets[1].name = "Sheet1";
		//sheets[1].guid = create_guid();

		//window_sheets.resize(2);
		//window_sheets[0].bindSchematicSheet(sheets[0]);
		//window_sheets[0].show = true;
		//window_sheets[0].name = "Sheet0";
		//window_sheets[1].bindSchematicSheet(sheets[1]);
		//window_sheets[1].show = true;
		//window_sheets[1].name = "Sheet1";
	}

	window.setVisible(true);
}

MainWindow::~MainWindow()
{
	main_window = nullptr;
}

void MainWindow::initializeProject()
{
	assert(!initialized);

	project_dir          = "";
	project_name         = "";
	project_ini_name     = "";
	curr_menu            = nullptr;
	curr_menu_hover      = nullptr;
	curr_window_sheet    = nullptr;
	project_saved        = false;
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

		settings.debug.show_bvh        = false;
		settings.debug.show_chunks     = false;
		settings.debug.show_fps        = false;
		settings.debug.show_imgui_demo = false;
	}
	{ // init ImGui
		ImGui::VK2D::Init(window, false);

		auto& io    = ImGui::GetIO();
		auto& style = ImGui::GetStyle();

		style.Colors[ImGuiCol_WindowBg]           = ImVec4(0.f, 0.f, 0.f, 1.f);
		//style.Colors[ImGuiCol_TitleBgActive]      = ImVec4(0.5f, 0.5f, 0.5f, 1.f);
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

bool MainWindow::closeProjectDialog()
{
	msg_box.title   = "closing project " + project_name;
	msg_box.content = "current project is not saved, save?";
	msg_box.buttons = "Yes;No;Cancel";

	auto result = msg_box.showDialog();
	if (result == "Yes") {
		if (!isProjectOpened())
			saveProjectDialog();
		saveProject();
	}else if (result == "Cancel" || result == "Close")
		return false;

	return true;
}

void MainWindow::closeProject()
{
	assert(initialized);

	if (!project_ini_name.empty())
		saveProjectIni();
	ImGui::VK2D::ShutDown(window);

	project_dir = "";
	project_name = "";
	sheets.clear();
	window_sheets.clear();

	initialized = false;
}

void MainWindow::show()
{
	while (true) {
		float dt = getDeltaTime();

		vk2d::Event e;
		while (window.pollEvent(e))
			eventProc(e, dt);

		if (want_open_project) {
			do {
				if (!isProjectEmpty() && !isProjectAllSaved())
					if (!closeProjectDialog())
						break;

				if (!new_project_dir.empty())
					openProject(new_project_dir);
				else
					initializeProject();
			} while (false);

			want_open_project = false;
			new_project_dir   = "";
		}

		if (want_close_window) {
			closeProject();
			window.close();
			break;
		}

		ImGui::VK2D::ProcessViewportEvent(window);

		loop(dt);

		float delay = 1.f / FRAME_LIMIT - dt;
		if (delay > 0) {
			std::this_thread::sleep_for(std::chrono::microseconds((int)(1e6 * delay)));
		}
	}
}

void MainWindow::loop(float dt)
{
	ImGui::VK2D::Update(window, dt);

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

	window_settings.showUI();
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

	if (io.WantSaveIniSettings && !project_ini_name.empty())
		saveProjectIni();
}

void MainWindow::eventProc(const vk2d::Event& e, float dt)
{
	ImGui::VK2D::ProcessEvent(window, e);

	using vk2d::Event;
	using vk2d::Key;

	switch (e.type) {
	case Event::Close:
		closeWindowDialog();
		break;
	case Event::Resize:
		break;
	}

	if (curr_menu && curr_window_sheet)
		getCurrentSideMenu().eventProc(e, dt);
}

void MainWindow::closeWindowDialog()
{
	if (!isProjectEmpty() && !isProjectAllSaved())
		if (!closeProjectDialog()) return;

	want_close_window = true;
}

void MainWindow::openProjectDialog()
{
	vk2d::FileOpenDialog dialog;

	dialog.owner        = &window;
	dialog.title        = L"Open Project";
	dialog.default_dir  = L"%Desktop";
	dialog.default_name = L"";
	dialog.filters.emplace_back(L"Project file", L"mlp");

	if (dialog.showDialog() != vk2d::DialogResult::OK) return;

	auto dir  = to_unicode(dialog.getResultPaths().front());
	auto path = std::filesystem::path(dir);
	auto name = to_unicode(path.filename());

	assert(path.extension() == L"mls");
	
	if (dir == project_dir && name == project_name) {

	} else {
		want_open_project = true;
		new_project_dir   = dir;
	}
}

void MainWindow::openProject(const std::string& dir)
{
	namespace fs = std::filesystem;

	std::vector<SchematicSheetPtr_t> new_sheets;

	auto new_project_name  = to_unicode(fs::path(dir).filename());
	auto project_file_name = dir + "\\" + new_project_name + ".mlp";

	tinyxml2::XMLDocument doc;
	doc.LoadFile(project_file_name.c_str());

	auto* root = doc.FirstChildElement("Project");

	{
		auto* elem = root->FirstChildElement("SchematicSheet");
		for (; elem; elem = elem->NextSiblingElement("SchematicSheet")) {
			std::string file_name = elem->Attribute("path");

			std::ifstream file(dir + "\\" + file_name);

			if (!file.is_open()) {
				msg_box.title   = "Error";
				msg_box.content = "cannot locate " + file_name;
				msg_box.buttons = "OK";

				msg_box.showDialog();
				return;
			}

			auto& sheet = new_sheets.emplace_back(std::make_unique<SchematicSheet>());
			sheet->unserialize(file);
			updateThumbnail(*sheet);

			if (elem->Attribute("guid") != sheet->guid) {
				msg_box.title   = "Error";
				msg_box.content = "guid of sheet " + file_name + " is different";
				msg_box.buttons = "OK";

				msg_box.showDialog();
				return;
			}
		}
	}

	if (initialized) 
		closeProject();
	initializeProject();

	project_dir      = dir;
	project_name     = new_project_name;
	project_ini_name = project_dir + "\\" + project_name + ".ini";
	sheets.swap(new_sheets);
	project_saved = true;

	loadProjectIni();

	postInfoMessage("Project " + project_name + " Opened");
}

bool MainWindow::saveProjectDialog()
{
	namespace fs = std::filesystem;

	vk2d::FolderOpenDialog dialog;

	dialog.owner        = &window;
	dialog.title        = L"Save Project";
	dialog.default_dir  = L"%Desktop";
	dialog.default_name = L"";
	dialog.check_empty  = true;
	
	if (dialog.showDialog() != vk2d::DialogResult::OK) return false;

	project_dir      = to_unicode(dialog.getResultDir());
	project_name     = to_unicode(fs::path(project_dir).filename());
	project_ini_name = project_dir + "\\" + project_name + ".ini";

	return true;
}

void MainWindow::saveProject()
{
	assert(isProjectOpened());

	tinyxml2::XMLDocument doc;

	auto* root = doc.NewElement("Project");
	doc.InsertFirstChild(root);

	for (const auto& sheet : sheets) {
		auto* elem = root->InsertNewChildElement("SchematicSheet");

		std::string file_name = sheet->name + ".mls";

		elem->SetAttribute("name", sheet->name.c_str());
		elem->SetAttribute("path", file_name.c_str());
		elem->SetAttribute("guid", sheet->guid.c_str());

		if (!sheet->file_saved)
			saveSchematicSheet(*sheet);
	}
	doc.SaveFile((project_dir + "\\" + project_name + ".mlp").c_str());

	saveProjectIni();

	project_saved = true;

	postInfoMessage("Project Saved");
}

void MainWindow::loadProjectIni()
{
	assert(!project_ini_name.empty());

	tinyxml2::XMLDocument doc;

	doc.LoadFile(project_ini_name.c_str());
	auto* root = doc.FirstChildElement("ProjectInitialization");

	{
		auto* elem = root->FirstChildElement("ImGui");
		ImGui::LoadIniSettingsFromMemory(elem->GetText());
	}
}

void MainWindow::saveProjectIni()
{
	assert(!project_ini_name.empty());

	tinyxml2::XMLDocument doc;

	auto* root = doc.NewElement("ProjectInitialization");
	doc.InsertFirstChild(root);
	
	{
		auto* elem = root->InsertNewChildElement("ImGui");
		elem->SetText(ImGui::SaveIniSettingsToMemory());
	};

	doc.SaveFile(project_ini_name.c_str());
}

bool MainWindow::trySaveProject()
{
	if (!isProjectOpened())
		if (!saveProjectDialog())
			return false;

	saveProject();
	return true;
}

bool MainWindow::isProjectOpened() const
{
	return !project_dir.empty();
}

SideMenu& MainWindow::getCurrentSideMenu()
{
	return *curr_menu;
}

bool MainWindow::isProjectAllSaved() const
{
	return !hasUnsavedSchematicSheet() && project_saved;
}

bool MainWindow::isProjectEmpty() const
{
	if (!project_name.empty()) return false;
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

void MainWindow::importSchematicSheetDialog()
{
	vk2d::FileOpenDialog dialog;

	dialog.owner        = &window;
	dialog.title        = L"Import";
	dialog.default_dir  = L"%Desktop";
	dialog.default_name = L"";
	dialog.filters.emplace_back(L"Schematic sheet file", L"mls");

	if (dialog.showDialog() != vk2d::DialogResult::OK) return;


}

void MainWindow::exportSchematicSheetDialog()
{
}

void MainWindow::addSchematicSheet()
{
	bool        found;
	int32_t     num = -1;
	std::string name;

	do {
		found = false;
		name  = "Sheet" + std::to_string(++num);

		for (auto& sheet : sheets) {
			if (sheet->name == name) {
				found = true;
				break;
			}
		}
	} while (found);

	auto& sheet = sheets.emplace_back(std::make_unique<SchematicSheet>());

	sheet->guid      = create_guid();
	sheet->name      = name;
	updateThumbnail(*sheet);

	project_saved = false;
}

bool MainWindow::saveSchematicSheet(SchematicSheet& sheet)
{
	if (!isProjectOpened())
		if (!saveProjectDialog()) 
			return false;

	for (auto& ws : window_sheets) {
		if (ws->sheet == &sheet) {
			ws->sheetSaved();
			break;
		}
	}

	std::ofstream of(project_dir + "\\" + sheet.name + ".mls");
	sheet.serialize(of);
	sheet.file_saved = true;

	postInfoMessage("Sheet " + sheet.name + " Saved");
	return true;
}

void MainWindow::deleteSchematicSheet(SchematicSheet& sheet)
{
}

bool MainWindow::hasUnsavedSchematicSheet() const
{
	for (auto& sheet : sheets)
		if (!sheet->file_saved)
			return true;
	return false;
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
			auto* window = ImGui::FindWindowByName(ws->name.c_str());
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
			ImGui::DockBuilderDockWindow(ws->name.c_str(), id);
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

float MainWindow::getDeltaTime()
{
	using microseconds = std::chrono::microseconds;

	auto now = clock_t::now();
	
	float dt = (float)std::chrono::duration_cast<microseconds>(now - last_time).count() / 1e6f;
	
	last_time = now;

	return dt;
}

void MainWindow::showMainMenus()
{
	auto spacing = ImGui::GetFrameHeight();
	vec2 icon_size(spacing, spacing);
	spacing *= 1.5;

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			ImGui::SetCursorPosX(spacing);
			if (ImGui::BeginMenu("Recent...")) {
				ImGui::EndMenu();
			}

			ImGui::Image(ICON_FOLDER, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Open")) {
				if (!isProjectEmpty() || isProjectAllSaved())
					closeProjectDialog();
				openProjectDialog();
			}

			ImGui::Image(ICON_WINDOW, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("New Window")) {}

			ImGui::Separator();

			if (curr_window_sheet && !curr_window_sheet->sheet->file_saved) {
				std::string save_str = "Save ";
				save_str += curr_window_sheet->sheet->name;
				save_str += "###MenuItemSave";

				ImGui::Image(ICON_SAVE, icon_size);
				ImGui::SameLine();
				if (ImGui::MenuItem(save_str.c_str(), "Ctrl+S"))
					trySaveProject();
			} else {
				ImGui::Image(ICON_SAVE, icon_size, vk2d::Colors::Gray);
				ImGui::SameLine();
				ImGui::MenuItem("Save###MenuItemSave", "Ctrl+S", false, false);
			}

			ImGui::SetCursorPosX(spacing);
			if (ImGui::MenuItem("Save Project"))
				trySaveProject();

			ImGui::SetCursorPosX(spacing);
			if (ImGui::MenuItem("Save Project As..."))
				if (saveProjectDialog())
					saveProject();

			ImGui::Image(ICON_SAVE_ALL, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Save All", "Ctrl+Shift+S"))
				trySaveProject();

			ImGui::Separator();

			ImGui::Image(ICON_IMPORT, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Import Sheet")) {
				trySaveProject();
			}

			ImGui::Image(ICON_EXPORT, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Export Sheet")) {
				trySaveProject();
			}
			
			ImGui::Separator();

			ImGui::Image(ICON_EXIT, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Exit", "Alt+F4")) {
				closeWindowDialog();
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
			ImGui::MenuItem("Grid", nullptr, &settings.view.grid);

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
				window_settings.show = true;
			}
			ImGui::EndMenu();
		}

		ImGui::Separator();
		if (project_saved)
			ImGui::TextUnformatted(project_name.c_str());
		else if (!project_name.empty())
			ImGui::Text("*%s", project_name.c_str());
		else if (isProjectEmpty())
			ImGui::TextUnformatted("(empty project)");
		else
			ImGui::TextUnformatted("*(empty project)");

		auto cursor_x = ImGui::GetCursorPosX();
		auto width    = ImGui::GetWindowWidth();
		auto height    = ImGui::GetWindowHeight();

		titlebar.setCaptionRect({ cursor_x, 0.f, width - cursor_x, height});
		titlebar.showButtons();
		ImGui::EndMainMenuBar();
	}
	ImGui::PopStyleVar();
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
				trySaveProject();

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

void MainWindow::showSheets()
{
	//{
	//	auto* dc = &ImGui::GetCurrentContext()->DockContext;

	//	for (int n = 0; n < dc->Nodes.Data.Size; n++) {
	//		ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p;

	//		if (node && node->IsEmpty()) {
	//			auto* draw_list = ImGui::GetForegroundDrawList();

	//			ImVec2 max(node->Pos.x + node->Size.x, node->Pos.y + node->Size.y);

	//			draw_list->AddRect(node->Pos, max, 0xffffffff, 0, 0, 5);
	//			break;
	//		}
	//	}
	//}

	auto flags = ImGuiWindowFlags_NoDecoration;
	ImGui::Begin("##Sheets", nullptr, flags);

	if (ImGui::BeginTabBar("##Sheets Tabs")) {

		ImGui::EndTabBar();
	}

	ImGui::End();
}

void MainWindow::showFPS()
{
	auto& io = ImGui::GetIO();
	ImGui::Begin("fps", &settings.debug.show_fps, ImGuiWindowFlags_NoDocking);
	ImGui::Text("dt : %.2fms", 1000.f * io.DeltaTime);
	ImGui::Text("fps: %d", (int)(1.f / io.DeltaTime));
	ImGui::End();
}
