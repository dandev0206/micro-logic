#pragma once

#include "../gui/docking_window.h"

class Window_History : public DockingWindow {
public:
	Window_History();

	void showUI();

	bool show;
};