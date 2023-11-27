#define ICON_TEXTURE_VAR textures[TEXTURE_ICONS_IDX]

#include "main_window.h"

#include <vk2d/graphics/texture_view.h>
#include <vk2d/system/clipboard.h>
#include <vk2d/system/file_dialog.h>
#include <vk2d/system/message_box.h>
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

#ifdef _WIN32 
#include "gui/platform/windows_main_window.h"
#endif

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)
#define IMGUI_NO_LABEL "##" STRINGIZE(__COUNTER__)

using vk2d::MessageBox;
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
	wstr.resize(str.length() / 2);
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

	window.create(1280, 800, "micro logic", vk2d::Window::Style::None);
#ifdef _WIN32 
	InjectWndProc(this);
#endif

	{ // load font
		font.loadFromFile("C:/Windows/Fonts/consola.ttf");
		font.getGlyph('a', 100, true);
		font.getGlyph('b', 100, true);
		font.getGlyph('c', 100, true);
		font.getGlyph('d', 100, true);
		font.getGlyph('e', 100, true);
		font.getGlyph('f', 100, true);
		font.getGlyph('g', 100, true);
		font.getGlyph('h', 100, true);
		font.getGlyph('i', 100, true);
		font.getGlyph('j', 100, true);
		font.getGlyph('k', 100, true);
		font.getGlyph('l', 100, true);
		font.getGlyph('m', 100, true);
		font.getGlyph('n', 100, true);
		font.getGlyph('o', 100, true);
	}
	{ // load side menus
		side_menus.emplace_back(std::make_unique<Menu_Info>(0));
		side_menus.emplace_back(std::make_unique<Menu_Zoom>(1));
		side_menus.emplace_back(std::make_unique<Menu_Select>(2));
		side_menus.emplace_back(std::make_unique<Menu_Library>(3));
		side_menus.emplace_back(std::make_unique<Menu_Unit>(4));
		side_menus.emplace_back(std::make_unique<Menu_Shapes>(5));
		side_menus.emplace_back(std::make_unique<Menu_Label>(6));
		side_menus.emplace_back(std::make_unique<Menu_Copy>(7));
		side_menus.emplace_back(std::make_unique<Menu_Cut>(8));
		side_menus.emplace_back(std::make_unique<Menu_Delete>(9));
		side_menus.emplace_back(std::make_unique<Menu_Wire>(10));
		side_menus.emplace_back(std::make_unique<Menu_Net>(11));
		side_menus.emplace_back(std::make_unique<Menu_SplitWire>(12));
	}
	{ // load textures
		vk2d::Image icon_image;
		icon_image.loadFromFile("resources/icons.png");

		textures.emplace_back(icon_image);
		textures.emplace_back(font.getTexture(100));
	}
	{ // load circuit elements
		CircuitElementLoader loader;

		loader.load("resources/elements.xml", font);

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

	hovered_title_button = TitleButton::None;
	close_window         = false;

	initializeProject();

	{
		openProject("C:\\Users\\j0994\\OneDrive\\¹ÙÅÁ È­¸é\\Untitled0");
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
	curr_menu            = -1;
	curr_menu_hover      = -1;
	curr_window_sheet    = -1;
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
		style.Colors[ImGuiCol_HeaderActive]       = ImVec4(0.5f, 0.5f, 0.5f, 1.f);
		style.Colors[ImGuiCol_HeaderHovered]      = ImVec4(0.5f, 0.5f, 0.5f, 1.f);
		style.Colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.2f, 0.2f, 0.8f, 1.f);
		style.Colors[ImGuiCol_SliderGrab]         = ImVec4(1.f, 1.f, 1.f, 1.f);

		io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/Arial.ttf", FONT_SIZE);
		ImGui::VK2D::UpdateFontTexture(window);

		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigViewportsNoAutoMerge = true;
		io.ConfigDockingTransparentPayload = true;
	}

	initialized = true;
}

void MainWindow::closeProject()
{
	assert(initialized);

	if (hasUnsaved()) {
		MessageBox msg_box;

		msg_box.title   = to_wide("closing project " + project_name);
		msg_box.msg     = to_wide("current project not saved, save?");
		msg_box.buttons = MessageBox::Yes_No_Cancel;
		
		auto result = msg_box.showDialog();
		
		if (result == DialogResult::Yes)
			saveProjectDialog();
		else if (result == DialogResult::No)
			close_window = true;
		else
			return;
	}

	ImGui::VK2D::ShutDown(window);

	project_dir  = "";
	project_name = "";
	sheets.clear();
	sheet_thumbnails.clear();
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

		if (close_window) {
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

void MainWindow::closeWindow()
{
	closeProject();

	if (initialized) return;

	close_window = true;
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
		auto flags = ImGuiDockNodeFlags_PassthruCentralNode;// ImGuiDockNodeFlags_AutoHideTabBar;
		ImGui::DockSpaceOverViewport(nullptr, flags);
	}
	ImGui::PopStyleColor();

	auto& platform_io = ImGui::GetPlatformIO();

	if (curr_menu != -1 && curr_window_sheet != -1)
		if (getCurrentWindowSheet().window)
			getCurrentSideMenu().loop();

	for (auto& ws : window_sheets) {
		ws.showUI();
		ws.draw();
	}

	getCurrentWindowSheet().clearHoverList();

	window_settings.showUI();
	window_library.showUI();
	window_history.showUI();

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
}

void MainWindow::eventProc(const vk2d::Event& e, float dt)
{
	ImGui::VK2D::ProcessEvent(window, e);

	using vk2d::Event;
	using vk2d::Key;

	switch (e.type) {
	case Event::Close:
		closeWindow();
		break;
	case Event::Resize:
		break;
	}

	if (curr_menu != -1)
		getCurrentSideMenu().eventProc(e, dt);
}

void MainWindow::openProjectDialog()
{
	namespace fs = std::filesystem;

	vk2d::FolderOpenDialog dialog;

	dialog.owner        = &window;
	dialog.title        = L"Open Project";
	dialog.default_dir  = L"%Desktop";
	dialog.default_name = L"";
	dialog.check_empty  = true;

	if (dialog.showDialog() != vk2d::DialogResult::OK) return;

	auto dir  = to_unicode(dialog.getResultDir());
	auto name = to_unicode(fs::path(dir).filename());
	
	if (dir == project_dir && name == project_name) {

	} else
		openProject(dir);
}

void MainWindow::openProject(const std::string& dir)
{
	namespace fs = std::filesystem;
	using vk2d::MessageBox;

	std::vector<SchematicSheet_t> new_sheets;
	std::vector<Window_Sheet>     new_window_sheets;

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
				vk2d::MessageBox msg_box;
				msg_box.owner = &window;
				msg_box.title = L"Error";
				msg_box.msg   = to_wide("cannot locate " + file_name);

				msg_box.showDialog();
				return;
			}

			auto& sheet = new_sheets.emplace_back(std::make_unique<SchematicSheet>());
			sheet->unserialize(file);

			auto& ws = new_window_sheets.emplace_back();
			ws.bindSchematicSheet(*sheet);
			ws.show               = true;
			ws.name               = sheet->name;
			ws.file_saved         = true;
			ws.last_saved_command = -1;

			if (elem->Attribute("guid") != sheet->guid) {
				vk2d::MessageBox msg_box;
				msg_box.owner = &window;
				msg_box.title = L"Error";
				msg_box.msg   = to_wide("guid of sheet " + file_name + " is different");

				msg_box.showDialog();
				return;
			}
		}
	}
	{
		auto elem = root->FirstChildElement("UISettings");
		ImGui::LoadIniSettingsFromMemory(elem->GetText());
	}

	if (initialized) 
		closeProject();
	initializeProject();

	project_dir  = dir;
	project_name = new_project_name;
	sheets.swap(new_sheets);
	window_sheets.swap(new_window_sheets);
	project_saved = true;
}

void MainWindow::saveProjectDialog()
{
	namespace fs = std::filesystem;

	vk2d::FolderOpenDialog dialog;

	dialog.owner        = &window;
	dialog.title        = L"Save Project";
	dialog.default_dir  = L"%Desktop";
	dialog.default_name = L"";
	dialog.check_empty  = true;
	
	if (dialog.showDialog() != vk2d::DialogResult::OK) return;

	project_dir  = to_unicode(dialog.getResultDir());
	project_name = to_unicode(fs::path(project_dir).filename());

	saveProject();
}

void MainWindow::saveProject()
{
	tinyxml2::XMLDocument doc;

	auto* root = doc.NewElement("Project");
	doc.InsertFirstChild(root);

	for (const auto& sheet : sheets) {
		auto* elem = root->InsertNewChildElement("SchematicSheet");

		std::string file_name = sheet->name + ".mls";

		elem->SetAttribute("name", sheet->name.c_str());
		elem->SetAttribute("path", file_name.c_str());
		elem->SetAttribute("guid", sheet->guid.c_str());
	}
	for (auto& ws : window_sheets) {
		ws.saveSheet(project_dir);
	}
	{
		auto* elem = root->InsertNewChildElement("UISettings");
		elem->SetText(ImGui::SaveIniSettingsToMemory());
	}
	doc.SaveFile((project_dir + "\\" + project_name + ".mlp").c_str());
}

bool MainWindow::hasUnsaved() const
{
	for (auto& ws : window_sheets)
		if (!ws.file_saved) return true;
	return false;
}

SideMenu& MainWindow::getCurrentSideMenu()
{
	return *side_menus[curr_menu];
}

void MainWindow::setCurrentSideMenu(int32_t menu_idx)
{
	auto last_menu = curr_menu;
	curr_menu      = menu_idx;

	if (last_menu != curr_menu) {
		if (last_menu != -1)
			side_menus[last_menu]->onClose();
		if (curr_menu != -1)
			side_menus[curr_menu]->onBegin();
	}
}

void MainWindow::setCurrentSideMenu(const SideMenu& menu)
{
	setCurrentSideMenu(menu.menu_idx);
}

Window_Sheet& MainWindow::getCurrentWindowSheet()
{
	return window_sheets[curr_window_sheet];
}

void MainWindow::setCurrentWindowSheet(const Window_Sheet& window_sheet)
{
	for (size_t i = 0; i < window_sheets.size(); ++i) {
		if (&window_sheets[i] == &window_sheet) {
			curr_window_sheet = i;
			break;
		}
	}
}

void MainWindow::setStatusMessage(const std::string& str)
{
	status_message = str;
}

void MainWindow::setInfoMessage(const std::string& str)
{
	info_message = str;
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
	if (curr_window_sheet == -1) return false;
	return getCurrentWindowSheet().isRedoable();
}

bool MainWindow::isUndoable()
{
	if (curr_window_sheet == -1) return false;
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
		auto& menu = dynamic_cast<Menu_Copy&>(*side_menus[7]);
		menu.beginClipboardPaste(ss);
	} else if (guid == CLIPBOARD_CUT_IDENTIFICATION) {
		auto& menu = dynamic_cast<Menu_Cut&>(*side_menus[8]);
		menu.beginClipboardPaste(ss);
	}
}

vk2d::Texture MainWindow::createThumbnail(const SchematicSheet& sheet) const
{
	vk2d::RenderTexture texture(720, 480);

	texture.setClearColor(vk2d::Colors::Black);
	
	if (!sheet.bvh.empty()) {
		vk2d::DrawList draw_list;
		AABB aabb = sheet.bvh.begin()->second->getAABB();

		for (auto& [elem_aabb, elem] : sheet.bvh)
			aabb = aabb.union_of(elem_aabb);

		auto position        = aabb.center();
		auto grid_pixel_size = texture.size().x / aabb.width();
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
	return texture.release();
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
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			ImGui::SetCursorPosX(spacing);
			if (ImGui::BeginMenu("New...")) {
				if (ImGui::MenuItem("Project")) {}
				if (ImGui::MenuItem("Sheet")) {}
				ImGui::EndMenu();
			}

			ImGui::Image(ICON_WINDOW, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("New Window")) {}

			ImGui::Image(ICON_FOLDER, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Open")) {}

			ImGui::SetCursorPosX(spacing);
			if (ImGui::BeginMenu("Open Recent...")) {
				ImGui::EndMenu();
			}

			ImGui::Separator();

			ImGui::Image(ICON_SAVE, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Save")) {
				if (project_dir.empty())
					saveProjectDialog();
				else
					saveProject();
			}

			ImGui::SetCursorPosX(spacing);
			if (ImGui::MenuItem("Save As..."))
				saveProjectDialog();

			ImGui::Separator();
			
			ImGui::Image(ICON_EXIT, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Exit")) {
				closeWindow();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			ImGui::Image(ICON_UNDO, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Undo", "CTRL+Z", false, isUndoable())) undo();

			ImGui::Image(ICON_REDO, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Redo", "CTRL+Y", false, isRedoable())) redo();

			ImGui::Separator();

			ImGui::Image(ICON_COPY, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Copy", "CTRL+C"))
				getCurrentWindowSheet().copySelectedToClipboard();

			ImGui::Image(ICON_CUT, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Cut", "CTRL+X"))
				getCurrentWindowSheet().cutSelectedToClipboard();

			ImGui::Image(ICON_PASTE, icon_size);
			ImGui::SameLine();
			if (ImGui::MenuItem("Paste", "CTRL+V"))
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

			ImGui::Separator();

			ImGui::SetCursorPosX(spacing);
			ImGui::MenuItem("Grid", nullptr, &settings.view.grid);

			ImGui::SetCursorPosX(spacing);
			if (ImGui::MenuItem("reset view", nullptr, nullptr, curr_window_sheet != -1)) {
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
		ImGui::TextUnformatted(project_name.c_str());

		showTitleButtons();
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

			if (ImGui::ImageButton(ICON_SAVE, icon_size)) {
				if (project_dir.empty())
					saveProjectDialog();
				else
					saveProject();
			}

			ImGui::PopStyleVar();

			ImGui::Separator();

			if (curr_menu != -1)
				side_menus[curr_menu]->upperMenu();

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
		ImGuiWindowFlags_MenuBar;

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
	float height = ImGui::GetFrameHeight();
	if (ImGui::BeginViewportSideBar("##MainStatusBar", nullptr, ImGuiDir_Down, height, window_flags)) {
		if (ImGui::BeginMenuBar()) {
			ImGui::SetCursorPosX(height * 0.25f);
			ImGui::ImageButton(ICON_MESSAGE, { 23, 23 });

			if (!status_message.empty()) {
				ImGui::TextUnformatted(info_message.c_str());
				info_message.clear();
			} else if (curr_menu_hover != -1) {
				ImGui::TextUnformatted(side_menus[curr_menu_hover]->menu_name);
			} else if (curr_window_sheet != -1) {
				auto& ws = getCurrentWindowSheet();

				if (ws.capturing_mouse) {
					auto pos = ws.getCursorPlanePos();
					ImGui::Text("(%.1f, %.1f)", pos.x, pos.y);
				}
			}

			if (curr_window_sheet != -1) {
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
			ImGui::EndMenuBar();
		}
		ImGui::End();
	}
	ImGui::PopStyleVar();
}

void MainWindow::showSideMenus()
{
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
	if (ImGui::BeginViewportSideBar("##SideBar", nullptr, ImGuiDir_Left, 60, window_flags)) {
		curr_menu_hover = -1;

		for (int i = 0; i < side_menus.size(); ++i) {
			side_menus[i]->menuButton();

			if (ImGui::IsItemHovered())
				curr_menu_hover = i;
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

void MainWindow::showTitleButtons()
{
	static const int title_height = 23;

	vec2 button_size(title_height * 1.52, title_height);

	hovered_title_button = TitleButton::None;
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	ImGui::SetCursorPosX(ImGui::GetWindowWidth() - button_size.x * 4.8f);
	ImGui::ImageButton(ICON_MINIMIZE, button_size);
	if (ImGui::IsItemHovered()) 
		hovered_title_button = TitleButton::Minimize;

	ImGui::SameLine();
	ImGui::ImageButton(ICON_MAXIMIZE, button_size);
	if (ImGui::IsItemHovered()) 
		hovered_title_button = TitleButton::Maximize;

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.83f, 0.13f, 0.2f, 1.f));
	ImGui::ImageButton(ICON_CLOSE, button_size);
	if (ImGui::IsItemHovered()) 
		hovered_title_button = TitleButton::Close;
	ImGui::PopStyleColor();

	ImGui::PopStyleVar();
}
