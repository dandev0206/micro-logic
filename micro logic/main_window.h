#pragma once

#include <vk2d/system/window.h>
#include <vk2d/graphics/texture.h>
#include <vk2d/system/font.h>
#include <chrono>
#include "window/window_settings.h"
#include "window/window_sheet.h"
#include "window/window_library.h"
#include "window/window_history.h"
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
	using SchematicSheet_t = std::unique_ptr<SchematicSheet>;
	using clock_t          = std::chrono::system_clock;
	using timepoint_t      = clock_t::time_point;

	MainWindow();
	~MainWindow();

	void show();
	void closeWindow();

	void loop(float dt);
	void eventProc(const vk2d::Event& e, float dt);

	void initializeProject();
	void closeProject();
	void openProjectDialog();
	void openProject(const std::string& dir);
	void saveProjectDialog();
	void saveProject();
	bool hasUnsaved() const;

	SideMenu& getCurrentSideMenu();
	void setCurrentSideMenu(int32_t menu_idx);
	void setCurrentSideMenu(const SideMenu& menu);

	Window_Sheet& getCurrentWindowSheet();
	void setCurrentWindowSheet(const Window_Sheet& window_sheet);

	void setStatusMessage(const std::string& str);
	void setInfoMessage(const std::string& str);

	void redo();
	void undo();
	bool isRedoable();
	bool isUndoable();

	void beginClipboardPaste();

	vk2d::Texture createThumbnail(const SchematicSheet& sheet) const;

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

	std::vector<std::unique_ptr<SideMenu>> side_menus;
	std::vector<vk2d::Texture>             textures;
	std::vector<LogicGate>                 logic_gates;
	std::vector<LogicUnit>                 logic_units;

	std::string status_message;
	std::string info_message;

public: // project
	std::string project_dir;
	std::string project_name;

	std::vector<SchematicSheet_t> sheets;
	std::vector<vk2d::Texture>    sheet_thumbnails;
	std::vector<Window_Sheet>     window_sheets;

	int32_t curr_menu;
	int32_t curr_menu_hover;
	int32_t curr_window_sheet;

	timepoint_t last_time;
	
	bool project_saved;

public: // windows
	Window_Settings           window_settings;
	Window_Library            window_library;
	Window_History            window_history;

public:
	TitleButton hovered_title_button;

	bool close_window;
	bool initialized;
};

inline MainWindow& MainWindow::get()
{
	return *main_window;
}