#pragma once

#include <vk2d/system/window.h>
#include <vk2d/system/dialog.h>
#include <vk2d/graphics/texture.h>
#include <vk2d/system/font.h>
#include <chrono>
#include "gui/custom_titlebar.h"
#include "gui/resizing_loop.h"
#include "window/window_settings.h"
#include "window/window_sheet.h"
#include "window/window_library.h"
#include "window/window_history.h"
#include "window/window_explorer.h"
#include "side_menu.h"

class MainWindow : public ResizingLoop {
private:
	static MainWindow* main_window;

public:
	static inline MainWindow& get();

public:
	using SideMenuPtr_t       = std::unique_ptr<SideMenu>;
	using SchematicSheetPtr_t = std::unique_ptr<SchematicSheet>;
	using Window_SheetPtr_t   = std::unique_ptr<Window_Sheet>;
	
	using clock_t     = std::chrono::system_clock;
	using timepoint_t = clock_t::time_point;

	MainWindow();
	~MainWindow();

	void show();

	void loop() override;
	void eventProc(const vk2d::Event& e) override;

	bool closeWindow();

	void initializeProject();
	bool openProject();
	bool saveAll();
	bool saveProjectAs();
	bool saveProject();
	bool closeProject();
	bool openProjectImpl(const std::string& project_path);
	bool closeProjectImpl();
	bool isProjectOpened() const;
	bool isProjectEmpty() const;

	template <class Menu_T>
	Menu_T* findSideMenu();
	SideMenu& getCurrentSideMenu();
	void setCurrentSideMenu(SideMenu* menu);

	bool addSchematicSheet();
	bool saveSchematicSheet(SchematicSheet& sheet);
	bool importSchematicSheet();
	bool exportSchematicSheet(const SchematicSheet& sheet);
	bool openSchematicSheetImpl(SchematicSheetPtr_t& sheet, const std::string& path);
	bool saveSchematicSheetImpl(const SchematicSheet& sheet, const std::string& path);
	bool deleteSchematicSheet(SchematicSheet& sheet);
	bool hasUnsavedSchematicSheet() const;
	SchematicSheet* findSchematicSheetByPath(const std::string& path);

	void updateThumbnail(SchematicSheet& sheet);

	bool openWindowSheet(SchematicSheet& sheet);
	void closeUnvisibleWindowSheet();
	Window_Sheet& getCurrentWindowSheet();
	void setCurrentWindowSheet(Window_Sheet* window_sheet);
	Window_Sheet* findWindowSheet(const SchematicSheet& sheet);

	void postStatusMessage(const std::string& str);
	void postInfoMessage(const std::string& str, bool force = false);
	bool infoMessageTimeout() const;

	void redo();
	void undo();
	bool isRedoable();
	bool isUndoable();

	void beginClipboardPaste();

public:
	void showMainMenus();
	void showUpperMenus();
	void showStatusBar();
	void showSideMenus();
	void showFPS();

public: // settings
	struct Settings {
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
			bool enable_vsync;
			bool frame_limit;
			int  max_fps;
		} rendering;

		struct {
			bool show_bvh;
			bool show_chunks;
			bool show_fps;
			bool show_imgui_demo;
		} debug;

		int curr_settings_menu;
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

	std::vector<SchematicSheetPtr_t> sheets;
	std::vector<Window_SheetPtr_t>   window_sheets;

	SideMenu*     curr_menu;
	SideMenu*     curr_menu_hover;
	Window_Sheet* curr_window_sheet;

	timepoint_t last_time;
	
	bool project_opened;

public: // windows
	Window_Library  window_library;
	Window_History  window_history;
	Window_Explorer window_explorer;

public:
	CustomTitleBar titlebar;

	bool want_open_project;
	bool want_close_window;

	std::string new_project_path;

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