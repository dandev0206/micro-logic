#pragma once

#include <vk2d/system/window.h>
#include <vk2d/graphics/texture.h>
#include <vk2d/system/font.h>
#include <chrono>
#include "window/window_settings.h"
#include "window/window_sheet.h"
#include "window/window_library.h"
#include "window/window_history.h"
#include "window/window_explorer.h"
#include "side_menu.h"

#define FRAME_LIMIT 60
#define FONT_SIZE 25

#define TEXTURE_ICONS_IDX 0

enum class TitleButton {
	None     = 0,
	Minimize = 1,
	Maximize = 2,
	Close    = 3
};

class MainWindow {
private:
	static MainWindow* main_window;

public:
	static inline MainWindow& get();

public:
	using SideMenuPtr_t       = std::unique_ptr<SideMenu>;
	using SchematicSheetPtr_t = std::unique_ptr<SchematicSheet>;
	using Window_SheetPtr_t   = std::unique_ptr<Window_Sheet>;
	
	using clock_t          = std::chrono::system_clock;
	using timepoint_t      = clock_t::time_point;

	MainWindow();
	~MainWindow();

	void show();

	void loop(float dt);
	void eventProc(const vk2d::Event& e, float dt);
	void closeWindowDialog();

	void initializeProject();
	void closeProject();
	void openProjectDialog();
	void openProject(const std::string& dir);
	bool saveProjectDialog();
	void saveProject();
	void loadProjectIni();
	void saveProjectIni();
	bool trySaveProject();
	bool isProjectOpened() const;

	template <class Menu_T>
	Menu_T* findSideMenu();
	SideMenu& getCurrentSideMenu();
	void setCurrentSideMenu(SideMenu* menu);

	void addSchematicSheet();
	bool saveSchematicSheet(SchematicSheet& sheet);
	void deleteSchematicSheet(SchematicSheet& sheet);
	bool hasUnsavedSchematicSheet() const;

	void updateThumbnail(SchematicSheet& sheet);

	bool openWindowSheet(SchematicSheet& sheet);
	void closeUnvisibleWindowSheet();
	Window_Sheet& getCurrentWindowSheet();
	void setCurrentWindowSheet(Window_Sheet* window_sheet);

	void postStatusMessage(const std::string& str);
	void postInfoMessage(const std::string& str, bool force = false);
	bool infoMessageTimeout() const;

	void redo();
	void undo();
	bool isRedoable();
	bool isUndoable();

	void beginClipboardPaste();

	float getDeltaTime();

public:
	void showMainMenus();
	void showUpperMenus();
	void showStatusBar();
	void showSideMenus();
	void showSheets();
	void showFPS();
	void showTitleButtons();

public: // settings
	struct {
		struct {
			bool      grid;
			bool      grid_axis;
			GridStyle grid_style;
			float     grid_width;
			float     grid_axis_width;
			Color     grid_color;
			Color     grid_axis_color;

			bool      library_window;
		} view;

		struct {
			bool show_bvh;
			bool show_chunks;
			bool show_fps;
			bool show_imgui_demo;
		} debug;
	} settings;

public:
	vk2d::Window window;
	vk2d::Font   font;

	std::vector<SideMenuPtr_t> side_menus;
	std::vector<vk2d::Texture> textures;
	std::vector<LogicGate>     logic_gates;
	std::vector<LogicUnit>     logic_units;

	std::string status_message;
	std::string info_message;
	timepoint_t info_message_last_time;

public: // project
	std::string project_dir;
	std::string project_name;
	std::string project_ini_name;

	std::vector<SchematicSheetPtr_t> sheets;
	std::vector<Window_SheetPtr_t>   window_sheets;

	SideMenu*     curr_menu;
	SideMenu*     curr_menu_hover;
	Window_Sheet* curr_window_sheet;

	timepoint_t last_time;
	
	bool project_saved;

public: // windows
	Window_Settings window_settings;
	Window_Library  window_library;
	Window_History  window_history;
	Window_Explorer window_explorer;

public:
	TitleButton hovered_title_button;

	bool close_window;
	bool initialized;
};

inline MainWindow& MainWindow::get()
{
	return *main_window;
}

template <class Menu_T>
Menu_T* MainWindow::findSideMenu()
{
	for (auto& menu : side_menus)
		if (auto* ptr = dynamic_cast<Menu_T*>(menu.get()))
			return ptr;

	return nullptr;
}